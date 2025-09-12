#include "ccore/c_target.h"
#include "cbase/c_bit_field.h"
#include "cbase/c_debug.h"
#include "cbase/c_memory.h"
#include "cbase/c_runes.h"
#include "cbase/c_printf.h"
#include "cbase/c_va_list.h"

#include "csocket/private/c_message-tcp.h"
#include "csocket/private/c_message.h"
#include "csocket/c_address.h"
#include "csocket/c_message.h"
#include "csocket/c_netip.h"
#include "csocket/c_socket.h"
#include "ctime/c_time.h"

#ifdef TARGET_PC
#    include <stdio.h>
#    include <winsock2.h>  // For socket(), connect(), send(), and recv()
#    include <ws2tcpip.h>
typedef int  socklen_t;
typedef char raw_type;  // Type used for raw data on this platform
#    pragma comment(lib, "winmm.lib")
#    pragma comment(lib, "Ws2_32.lib")
#else
#    include <arpa/inet.h>   // For inet_addr()
#    include <cstdlib>       // For atoi()
#    include <cstring>       // For memset()
#    include <netdb.h>       // For gethostbyname()
#    include <netinet/in.h>  // For sockaddr_in
// #include <stdio>
#ifdef TARGET_MAC
#    include <fcntl.h>  // For socket(), connect(), send(), and recv()
#endif
#    include <sys/socket.h>  // For socket(), connect(), send(), and recv()
#    include <sys/types.h>   // For data types
#    include <unistd.h>      // For close()
typedef void raw_type;  // Type used for raw data on this platform
#endif

#include <errno.h>  // For errno

namespace ncore
{
    // Forward declares
    class socket_tcp_t;

    static bool s_set_blocking_mode(sd_t sock, bool noblock)
    {
#ifdef WINVER
        u_long flag = noblock;
        if (ioctlsocket(sock, FIONBIO, &flag) != 0)
            return false;
#else
        s32 flags = ::fcntl(sock, F_GETFL);
        if (noblock)
            flags |= O_NONBLOCK;
        else
            flags &= ~O_NONBLOCK;

        if (::fcntl(sock, F_SETFL, flags) == -1)
            return false;
#endif
        return true;
    }

    static void s_read_sockaddr(const sockaddr_storage* addr, char* host, u16& port)
    {
        if (addr->ss_family == AF_INET)
        {
            sockaddr_in const* sin = reinterpret_cast<const struct sockaddr_in*>(addr);
            port                   = ntohs(sin->sin_port);
            inet_ntop(addr->ss_family, (void*)&(sin->sin_addr), host, sizeof(sin->sin_addr));
        }
        else
        {
            sockaddr_in6 const* sin = reinterpret_cast<const struct sockaddr_in6*>(addr);
            port                    = ntohs(sin->sin6_port);
            inet_ntop(addr->ss_family, (void*)&(sin->sin6_addr), host, sizeof(sin->sin6_addr));
        }
    }

    static addrinfo* s_get_socket_info(crunes_t const& host, u16 port, bool wildcardAddress)
    {
        struct addrinfo conf, *res;
        g_memset(&conf, 0, sizeof(conf));
        conf.ai_family = AF_INET;
        if (wildcardAddress)
            conf.ai_flags |= AI_PASSIVE;
        conf.ai_socktype = SOCK_STREAM;

        char portStrBuffer[20];
        portStrBuffer[sizeof(portStrBuffer) - 1] = '\0';
        runes_t portstr                          = make_runes(portStrBuffer, portStrBuffer + sizeof(portStrBuffer) - 1);
        sprintf(portstr, make_crunes("%u"), va_t(port));

        s32 result = ::getaddrinfo(host.m_ascii, portstr.m_ascii, &conf, &res);
        if (result != 0)
            return NULL;  // ERROR_RESOLVING_ADDRESS
        return res;
    }

    union socket_address
    {
        sockaddr    sa;
        sockaddr_in sin;
#ifdef NS_ENABLE_IPV6
        sockaddr_in6 sin6;
#else
        sockaddr sin6;
#endif
        void clear()
        {
            memset(&sa, 0, sizeof(sa));
            memset(&sin, 0, sizeof(sin));
            memset(&sin6, 0, sizeof(sin6));
        }
    };

//    static void s_set_close_on_exec(sd_t sock) { (void)::SetHandleInformation((HANDLE)sock, HANDLE_FLAG_INHERIT, 0); }

    const u16 STATUS_NONE                = 0;
    const u16 STATUS_SECURE              = 0x1000;
    const u16 STATUS_SECURE_RECV         = 0x0001;
    const u16 STATUS_SECURE_SEND         = 0x0002;
    const u16 STATUS_ACCEPT              = 0x0004;
    const u16 STATUS_CONNECT             = 0x0008;
    const u16 STATUS_ACCEPT_SECURE_RECV  = STATUS_ACCEPT | STATUS_SECURE | STATUS_SECURE_RECV;
    const u16 STATUS_ACCEPT_SECURE_SEND  = STATUS_ACCEPT | STATUS_SECURE | STATUS_SECURE_SEND;
    const u16 STATUS_CONNECT_SECURE_SEND = STATUS_CONNECT | STATUS_SECURE | STATUS_SECURE_SEND;
    const u16 STATUS_CONNECT_SECURE_RECV = STATUS_CONNECT | STATUS_SECURE | STATUS_SECURE_RECV;
    const u16 STATUS_CONNECTING          = 0x20;
    const u16 STATUS_CONNECTED           = 0x040;
    const u16 STATUS_DISCONNECTED        = 0x80;
    const u16 STATUS_CLOSE               = 0x100;
    const u16 STATUS_CLOSE_IMMEDIATELY   = 0x200;

    static bool status_is(u16 status, u16 check) { return (status & check) == check; }
    static bool status_is_one_of(u16 status, u16 check) { return (status & check) != 0; }
    static u16  status_set(u16 status, u16 set) { return status | set; }
    static u16  status_clear(u16 status, u16 set) { return status & ~set; }

    struct connection_t
    {
        sd_t                  m_handle;
        tick_t                m_last_io_time;
        u16                   m_status;
        u16                   m_ip_port;
        char                  m_ip_str[20];
        u32                   m_sockaddr_len;
        socket_address        m_sockaddr;
        address_t*            m_address;
        socket_tcp_t*         m_parent;
        message_queue_t       m_message_queue;
        message_t*            m_message_read;
        message_socket_reader m_message_reader;
        message_socket_writer m_message_writer;
    };

    const int INVALID_SOCKET = -1;

    static void s_init(connection_t* c, socket_tcp_t* parent)
    {
        c->m_handle       = INVALID_SOCKET;
        c->m_last_io_time = 0;
        c->m_status       = STATUS_NONE;
        c->m_ip_port      = 0;
        g_memset(c->m_ip_str, 0, sizeof(c->m_ip_str));
        c->m_sockaddr_len = 0;
        c->m_sockaddr.clear();
        c->m_address = NULL;
        c->m_parent  = parent;
        c->m_message_queue.init();
        c->m_message_read = NULL;
        c->m_message_reader.init(INVALID_SOCKET);
        c->m_message_writer.init(INVALID_SOCKET, NULL);
    }

    enum e_create_socket
    {
        CS_OPTION_LISTEN  = 1,
        CS_OPTION_NOBLOCK = 2,
    };

    static s32 s_create_socket(crunes_t const& host, u16 port, u32 flags, connection_t* socket)
    {
        socket->m_handle = NULL;
        socket->m_status = STATUS_NONE;

        addrinfo* info;
        if (is_empty(host) == false)
        {
            info = s_get_socket_info(host, port, false);
        }
        else
        {
            crunes_t localhost = make_crunes("localhost");
            info               = s_get_socket_info(localhost, port, true);
        }

        struct addrinfo* nextAddr = info;
        while (nextAddr)
        {
            if (nextAddr->ai_family == AF_INET6)
            {  // skip IPv6
                nextAddr = nextAddr->ai_next;
                continue;
            }

            socket->m_handle = ::socket(nextAddr->ai_family, nextAddr->ai_socktype, nextAddr->ai_protocol);
            if (socket->m_handle == -1)
            {
                nextAddr = nextAddr->ai_next;
                continue;
            }
            s_set_blocking_mode(socket->m_handle, nflags::is_set(flags, CS_OPTION_NOBLOCK));

#ifdef TARGET_PC
            byte flag = 1;
#else
            int flag = 1;
            if (::setsockopt(socket->m_handle, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&flag), sizeof(flag)) == -1)
            {
                ::close(socket->m_handle);
                return -1;
            }
#endif
            if (::setsockopt(socket->m_handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&flag), sizeof(flag)) == -1)
            {
                ::close(socket->m_handle);
                socket->m_handle = -1;
                return -1;
            }
            if (nflags::is_set(flags, CS_OPTION_LISTEN))
            {
                if (::connect(socket->m_handle, nextAddr->ai_addr, nextAddr->ai_addrlen) == -1 && !nflags::is_set(flags, CS_OPTION_NOBLOCK))
                {
                    ::close(socket->m_handle);
                    socket->m_handle = -1;
                }
                else if (!nflags::is_set(flags, CS_OPTION_NOBLOCK))
                {
                    socket->m_status = STATUS_CONNECTED;
                }
                else
                {
                    socket->m_status = STATUS_CONNECTING;
                }
            }
            else
            {
                if (::bind(socket->m_handle, nextAddr->ai_addr, nextAddr->ai_addrlen) == -1)
                {
                    ::close(socket->m_handle);
                    socket->m_handle = -1;
                }
                else if (::listen(socket->m_handle, 16) == -1)
                {
                    ::close(socket->m_handle);
                    socket->m_handle = -1;
                    return -1;
                }
            }
            if (socket->m_handle == -1)
            {
                nextAddr = nextAddr->ai_next;
                continue;
            }

            if (!nflags::is_set(flags, CS_OPTION_NOBLOCK))
                s_set_blocking_mode(socket->m_handle, false);

            memcpy(&socket->m_sockaddr, nextAddr->ai_addr, nextAddr->ai_addrlen);
            socket->m_sockaddr_len = nextAddr->ai_addrlen;

            break;
        }

        if (socket->m_handle == -1)
        {
            ::close(socket->m_handle);
            socket->m_handle = -1;
            return -1;
        }

        struct sockaddr_storage localAddr;
#ifdef TARGET_PC
        s32 size = sizeof(localAddr);
#else
        u32 size = sizeof(localAddr);
#endif
        if (::getsockname(socket->m_handle, reinterpret_cast<struct sockaddr*>(&localAddr), &size) != 0)
        {
            ::close(socket->m_handle);
            return -1;
        }

        s_read_sockaddr(&localAddr, socket->m_ip_str, socket->m_ip_port);
        return 0;
    }

    struct connections_t
    {
        u32            m_len;
        u32            m_max;
        connection_t** m_array;
    };

    void push_connection(connections_t* self, connection_t* c)
    {
        self->m_array[self->m_len] = c;
        self->m_len += 1;
    }

    bool pop_connection(connections_t* self, connection_t*& c)
    {
        if (self->m_len == 0)
            return false;
        self->m_len -= 1;
        c = self->m_array[self->m_len];
        return true;
    }

    // An address is a netip_t connected to an ID.
    // When it is actively used it has a valid pointer
    // to a connection.
    // The ID and netip are part of the 'secure' handshake
    // that is done when a connection is established.
    struct address_t
    {
        connection_t* m_conn;
        sockid_t      m_sockid;
        netip_t       m_netip;
    };

    struct addresses_t
    {
        inline addresses_t()
            : m_len(0)
            , m_max(0)
            , m_array(NULL)
        {
        }

        u32         m_len;
        u32         m_max;
        address_t** m_array;
    };

    void push_address(addresses_t* self, address_t* c)
    {
        self->m_array[self->m_len] = c;
        self->m_len += 1;
    }

    bool pop_address(addresses_t* self, address_t*& c)
    {
        if (self->m_len == 0)
            return false;
        self->m_len -= 1;
        c = self->m_array[self->m_len];
        return true;
    }

    static void add_to_set(sd_t sock, fd_set* set, sd_t* max_fd)
    {
        if (sock != INVALID_SOCKET)
        {
            FD_SET(sock, set);
            if (*max_fd == INVALID_SOCKET || sock > *max_fd)
            {
                *max_fd = sock;
            }
        }
    }

    class socket_tcp_t : public socket_t
    {
    public:
        alloc_t*     m_allocator;
        u16          m_local_port;
        const char*  m_socket_name;  // e.g. Jurgen/CNSHAW1334/10.0.22.76:port/virtuosgames.com
        sockid_t     m_sockid;
        netip_t      m_netip;
        connection_t m_server_socket;

        u32           m_max_open;
        connections_t m_free_connections;
        connections_t m_secure_connections;
        connections_t m_open_connections;

        addresses_t m_to_connect;
        addresses_t m_to_disconnect;

        message_queue_t m_received_messages;

        connection_t* accept();
        void          send_secure_msg(connection_t* conn);

    public:
        inline socket_tcp_t()
            : m_allocator(nullptr)
        {
            s_attach();
        }
        ~socket_tcp_t() { s_release(); }

        void init(alloc_t* allocator) { m_allocator = allocator; }

        virtual void open(u16 port, crunes_t const& socket_name, sockid_t const& id, u32 max_open);
        virtual void close();

        virtual void process(addresses_t& open_conns, addresses_t& closed_conns, addresses_t& new_conns, addresses_t& failed_conns, addresses_t& pex_conns);

        virtual void connect(address_t*);
        virtual void disconnect(address_t*);

        virtual bool alloc_msg(message_t*& msg);
        virtual void commit_msg(message_t* msg);
        virtual void free_msg(message_t* msg);

        virtual bool send_msg(message_t* msg, address_t* to);
        virtual bool recv_msg(message_t*& msg, address_t*& from);

        DCORE_CLASS_PLACEMENT_NEW_DELETE
    };

    socket_t* gCreateTcpBasedSocket(alloc_t* allocator)
    {
        socket_tcp_t* socket = g_allocate<socket_tcp_t>(allocator);
        socket->init(allocator);
        return socket;
    }

    void gDestroyTcpBasedSocket(socket_t* socket)
    {
        socket_tcp_t* s = static_cast<socket_tcp_t*>(socket);
        s->close();
        g_deallocate(s->m_allocator, s);
    }

    void socket_tcp_t::open(u16 port, crunes_t const& name, sockid_t const& id, u32 max_open)
    {
        // Open the server (bind/listen) socket
        m_sockid = id;
        s_create_socket(crunes_t(), port, CS_OPTION_LISTEN | CS_OPTION_NOBLOCK, &m_server_socket);
    }

    void socket_tcp_t::close()
    {
        // close all active sockets
        // close server socket
        // free all messages
    }

    connection_t* socket_tcp_t::accept()
    {
        connection_t* c = NULL;

        socket_address sa;
        socklen_t      len = sizeof(sa);

        // NOTE: on Windows, sock is always > FD_SETSIZE
        sd_t sock = ::accept(m_server_socket.m_handle, &sa.sa, &len);
        if (sock != INVALID_SOCKET)
        {
            pop_connection(&m_free_connections, c);
            if (c == NULL)
            {
                ::close(sock);
                c = NULL;
            }
            else
            {
                s_init(c, this);
                s_set_blocking_mode(sock, false);
                c->m_parent       = this;
                c->m_handle       = sock;
                c->m_address      = NULL;
                c->m_sockaddr_len = len;
                memcpy(&c->m_sockaddr, &sa, len);
                c->m_status = STATUS_ACCEPT_SECURE_RECV;
            }
        }
        return c;
    }

    void socket_tcp_t::send_secure_msg(connection_t* conn)
    {
        // Create a message with our ID and Address details and send it
        message_t* secure_msg;
        alloc_msg(secure_msg);

        binary_writer_t msg_writer = secure_msg->get_writer();
        msg_writer.write_data(m_sockid.buffer());
        byte     netip_data[netip_t::SERIALIZE_SIZE];
        buffer_t netip(netip_data, netip_data + netip_t::SERIALIZE_SIZE);
        m_netip.serialize_to(netip);
        msg_writer.write_data(netip);
        secure_msg->m_size = msg_writer.size();

        conn->m_message_queue.push(secure_msg);
    }

    void socket_tcp_t::process(addresses_t& open_connections, addresses_t& closed_connections, addresses_t& new_connections, addresses_t& failed_connections, addresses_t& pex_connections)
    {
        // Non-block connects to remote IP:Port sockets
        address_t* remote_addr;
        while (pop_address(&m_to_connect, remote_addr))
        {
            connection_t* c = remote_addr->m_conn;
            if (c->m_address == NULL)
            {
                if (pop_connection(&m_free_connections, c))
                {
                    s_init(c, this);

                    char    remote_str_chars[128];
                    runes_t remote_str = make_runes(remote_str_chars, remote_str_chars + sizeof(remote_str_chars) - 1);
                    remote_addr->m_netip.to_string(remote_str, true);
                    if (s_create_socket(make_crunes(remote_str), remote_addr->m_netip.get_port(), CS_OPTION_NOBLOCK, c) == 0)
                    {
                        // This connection needs to be secured first
                        push_connection(&m_secure_connections, c);
                        c->m_status = STATUS_CONNECT_SECURE_SEND | STATUS_CONNECTING;
                    }
                    else
                    {
                        // Failed to connect to remote, reuse the connection object
                        push_address(&failed_connections, remote_addr);
                        push_connection(&m_free_connections, c);
                    }
                }
            }
        }

        // build the set of 'reading' sockets
        // build the set of 'writing' sockets
        sd_t   max_fd = INVALID_SOCKET;
        fd_set read_set, write_set, excp_set;
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_ZERO(&excp_set);
        add_to_set(m_server_socket.m_handle, &read_set, &max_fd);
        for (u32 i = 0; i < m_open_connections.m_len;)
        {
            connection_t* conn = m_open_connections.m_array[i];

            // DBG(("%p read_set", conn));
            add_to_set(conn->m_handle, &read_set, &max_fd);

            if (conn->m_message_queue.m_size > 0)
            {
                // DBG(("%p write_set", conn));
                add_to_set(conn->m_handle, &write_set, &max_fd);
            }
            ++i;
        }

        // process to-secure sockets, send / receive messages on these sockets
        // process messages for the 'secure' sockets and see if they are now 'secured'
        // any 'secured' sockets add them to 'open' sockets and add their addresses to 'new_connections'
        for (u32 i = 0; i < m_secure_connections.m_len; ++i)
        {
            connection_t* sc = m_secure_connections.m_array[i];
            if (status_is(sc->m_status, STATUS_SECURE_RECV))
            {
                add_to_set(sc->m_handle, &read_set, &max_fd);
            }
            else if (status_is(sc->m_status, STATUS_SECURE_SEND))
            {
                add_to_set(sc->m_handle, &write_set, &max_fd);
            }
        }

        // process messages for all other sockets and filter out any 'pex' messages, process them and register the addresses, for any 'new' address
        // add them to 'connections'.
        // for any socket that gave an 'error/except-ion' mark them as 'closed', add their addresses to 'closed_connections' and free their messages

        // any other messages add them to the 'recv queue'
        s32 const wait_ms = 1;

        timeval tv;
        tv.tv_sec  = wait_ms / 1000;
        tv.tv_usec = (wait_ms % 1000) * 1000;

        if (::select((s32)max_fd + 1, &read_set, &write_set, &excp_set, &tv) > 0)
        {
            // select() might have been waiting for a long time, reset current_time
            // now to prevent last_io_time being set to the past.
            tick_t current_time = getTime();

            // @TODO: Handle exceptions of the listening socket, we basically
            //        have to restart the server when this happens and the user needs
            //        to know this.
            //        Restarting should have a time-guard so that we don't try and restart
            //        every call.

            // Accept new connections
            if (m_server_socket.m_handle != INVALID_SOCKET && FD_ISSET(m_server_socket.m_handle, &read_set))
            {
                // We're not looping here, and accepting just one connection at
                // a time. The reason is that eCos does not respect non-blocking
                // flag on a listening socket and hangs in a loop.
                connection_t* conn = accept();
                if (conn != NULL)
                {
                    conn->m_last_io_time = current_time;
                    conn->m_status       = STATUS_ACCEPT_SECURE_RECV;
                    push_connection(&m_secure_connections, conn);
                }
            }

            message_queue_t rcvd_secure_messages;
            rcvd_secure_messages.init();

            for (u32 i = 0; i < m_open_connections.m_len;)
            {
                connection_t* conn = m_open_connections.m_array[i];

                // Why only checking connections that are in the CONNECTING state
                // for exceptions ?
                // if (status_is(conn->m_status, STATUS_CONNECTING))
                {
                    // The socket was in the state of connecting and was added to the write set.
                    // If it appears in the exception set we will close and remove it.
                    if (FD_ISSET(conn->m_handle, &excp_set))
                    {
                        conn->m_status = STATUS_CLOSE_IMMEDIATELY;
                    }
                }

                if (!status_is(conn->m_status, STATUS_CLOSE_IMMEDIATELY))
                {
                    if (FD_ISSET(conn->m_handle, &read_set))
                    {
                        conn->m_last_io_time = current_time;

                        s32 status = 1;
                        while (status > 0)
                        {
                            if (conn->m_message_read == NULL)
                            {
                                alloc_msg(conn->m_message_read);
                            }

                            message_t* rcvd_msg = NULL;
                            status              = conn->m_message_reader.read(conn->m_message_read, rcvd_msg);

                            if (rcvd_msg != NULL)
                            {
                                message_node_t* rcvd_node = msg_to_node(rcvd_msg);
                                rcvd_node->m_remote       = conn->m_address;
                                if (status_is(conn->m_status, STATUS_SECURE))
                                {
                                    if (status_is(conn->m_status, STATUS_SECURE_RECV))
                                    {
                                        binary_reader_t msg_reader = rcvd_msg->get_reader();
                                        sockid_t        sockid;
                                        buffer_t        sockid_buffer = sockid.buffer();
                                        msg_reader.read_data(sockid_buffer);

                                        // Search this ID in our database, if no address found create one
                                        // and add it to the database.
                                        // If found compare the netip in the entry with the netip received.

                                        if (status_is(conn->m_status, STATUS_ACCEPT_SECURE_RECV))
                                        {
                                            // This is an incoming connection and we have received a secure
                                            // message. Send one back with our own information to conclude
                                            // the secure handshake.
                                            send_secure_msg(conn);
                                            conn->m_status = status_clear(conn->m_status, STATUS_SECURE_RECV);
                                            conn->m_status = status_set(conn->m_status, STATUS_SECURE_SEND);
                                        }
                                    }
                                    else
                                    {
                                        // Something is wrong
                                        conn->m_status = STATUS_CLOSE_IMMEDIATELY;
                                    }
                                }
                                else
                                {
                                    m_received_messages.push(rcvd_node);
                                }
                            }
                        }
                    }

                    if (FD_ISSET(conn->m_handle, &write_set))
                    {
                        conn->m_last_io_time = current_time;

                        if (status_is(conn->m_status, STATUS_CONNECTING))
                        {
                            // Connected !
                            conn->m_status = status_clear(conn->m_status, STATUS_CONNECTING);
                            conn->m_status = status_set(conn->m_status, STATUS_CONNECTED);

                            // Is this a SECURE connection
                            if (status_is(conn->m_status, STATUS_SECURE))
                            {
                                send_secure_msg(conn);
                            }
                        }

                        s32        status;
                        message_t* msg_that_was_send;
                        while ((status = conn->m_message_writer.write(msg_that_was_send)) > 0)
                        {
                            if (msg_that_was_send != NULL)
                                free_msg(msg_that_was_send);
                        }

                        if (status_is(conn->m_status, STATUS_SECURE_SEND))
                        {
                            if (conn->m_message_queue.empty())
                            {
                                if (status_is(conn->m_status, STATUS_SECURE | STATUS_CONNECT))
                                {
                                    conn->m_status = status_clear(conn->m_status, STATUS_SECURE_SEND);
                                    conn->m_status = status_set(conn->m_status, STATUS_SECURE_RECV);
                                }
                                else if (status_is(conn->m_status, STATUS_SECURE | STATUS_ACCEPT))
                                {
                                    // Connection has been secured
                                    conn->m_status = status_clear(conn->m_status, STATUS_SECURE_SEND);
                                    conn->m_status = status_set(conn->m_status, STATUS_CONNECTED);
                                }
                            }
                        }

                        if (status < 0)
                        {
                            conn->m_status = status_set(conn->m_status, STATUS_CLOSE_IMMEDIATELY);
                        }
                    }
                }
            }
        }

        // Check connection states that are in-active or in a non connected state
        tick_t current_time = getTime();
        for (u32 i = 0; i < m_open_connections.m_len;)
        {
            connection_t* conn = m_open_connections.m_array[i];

            tick_t timeout = millisecondsToTicks(1000);
            if (!status_is(conn->m_status, STATUS_CONNECTED) && ((conn->m_last_io_time + timeout) > current_time))
            {
                // This connection seems 'frozen', close it
                conn->m_status = STATUS_CLOSE_IMMEDIATELY;
            }
        }

        // for all open sockets add their addresses to 'open_connections'
        // Every X seconds build a pex message and send it to the next open connection
    }

    void socket_tcp_t::connect(address_t* a) { push_address(&m_to_connect, a); }
    void socket_tcp_t::disconnect(address_t* a) { push_address(&m_to_disconnect, a); }

    bool socket_tcp_t::alloc_msg(message_t*& msg)
    {
        msg = NULL;
        return false;
    }

    void socket_tcp_t::commit_msg(message_t* msg)
    {
        // commit the now actual used message size
    }

    void socket_tcp_t::free_msg(message_t* msg)
    {
        // free the message back to the message allocator
    }

    bool socket_tcp_t::send_msg(message_t* msg, address_t* to)
    {
        if (to->m_conn != NULL)
        {
            if (status_is(to->m_conn->m_status, STATUS_CONNECTED))
            {
                // queue the message up in the to-send queue of the associated connection
                message_node_t* msg_hdr = msg_to_node(msg);
                to->m_conn->m_message_queue.push(msg_hdr);
                return true;
            }
        }
        return false;
    }

    bool socket_tcp_t::recv_msg(message_t*& msg, address_t*& from)
    {
        // pop any message from the 'recv-queue' until empty
        message_node_t* msg_node = m_received_messages.pop();
        if (msg_node != NULL)
        {
            from = msg_node->m_remote;
            msg  = node_to_msg(msg_node);
            return msg_node != NULL;
        }
        else
        {
            from = NULL;
            msg  = NULL;
            return false;
        }
    }

}  // namespace ncore
