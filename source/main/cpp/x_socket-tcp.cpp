// x_socket_tcp.cpp - Core socket functions 
#include "xbase/x_target.h"
#include "xbase/x_debug.h"
#include "xsocket/x_socket.h"
#include "xsocket/x_address.h"
#include "xsocket/x_netip.h"
#include "xsocket/private/x_message-tcp.h"

#ifdef PLATFORM_PC
    #include <winsock2.h>         // For socket(), connect(), send(), and recv()
    #include <ws2tcpip.h>
    #include <stdio.h>
	#include <time.h>
    typedef int socklen_t;
    typedef char raw_type;       // Type used for raw data on this platform
	#pragma comment(lib, "winmm.lib")
	#pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/types.h>       // For data types
    #include <sys/socket.h>      // For socket(), connect(), send(), and recv()
    #include <netdb.h>           // For gethostbyname()
    #include <arpa/inet.h>       // For inet_addr()
    #include <unistd.h>          // For close()
    #include <netinet/in.h>      // For sockaddr_in
    #include <cstring>	       // For memset()
    #include <stdio>
    #include <cstdlib>	       // For atoi()
	#include <time.h>
	typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <errno.h>             // For errno

namespace xcore
{
	// Forward declares
	class xsocket_tcp;

	static bool s_set_blocking_mode(socket_t sock, bool blocking)
	{
#ifdef WINVER
		u_long flag = !blocking;
		if (ioctlsocket(sock, FIONBIO, &flag) != 0)
			return false;
#else
		s32 flags = fcntl(sock, F_GETFL);
		if (blocking)
			flags &= ~O_NONBLOCK;
		else
			flags |= O_NONBLOCK;

		if (fcntl(sock, F_SETFL, flags) == -1)
			return false;
#endif
		return true;
	}

	static void s_read_sockaddr(const sockaddr_storage* addr, char* host, u16& port)
	{
		char buffer[INET6_ADDRSTRLEN];
		if (addr->ss_family == AF_INET)
		{
			sockaddr_in const* sin = reinterpret_cast<const struct sockaddr_in*>(addr);
			port = ntohs(sin->sin_port);
			inet_ntop(addr->ss_family, (void*)&(sin->sin_addr), buffer, sizeof(buffer));
		}
		else
		{
			sockaddr_in6 const* sin = reinterpret_cast<const struct sockaddr_in6*>(addr);
			port = ntohs(sin->sin6_port);
			inet_ntop(addr->ss_family, (void*)&(sin->sin6_addr), buffer, sizeof(buffer));
		}
		strcpy(host, buffer);
	}

	static addrinfo* s_get_socket_info(const char* host, unsigned int port, bool wildcardAddress)
	{
		struct addrinfo conf, *res;
		memset(&conf, 0, sizeof(conf));
		conf.ai_family = AF_INET;
		if (wildcardAddress)
			conf.ai_flags |= AI_PASSIVE;
		conf.ai_socktype = SOCK_STREAM;

		char portStr[10];
		snprintf(portStr, 10, "%u", port);

		s32 result = ::getaddrinfo(host, portStr, &conf, &res);
		if (result != 0)
			return NULL;	// ERROR_RESOLVING_ADDRESS
		return res;
	}

	union socket_address
	{
		sockaddr		sa;
		sockaddr_in		sin;
#ifdef NS_ENABLE_IPV6
		sockaddr_in6	sin6;
#else
		sockaddr		sin6;
#endif
	};

	static void s_set_close_on_exec(socket_t sock)
	{
		(void)SetHandleInformation((HANDLE)sock, HANDLE_FLAG_INHERIT, 0);
	}

	static void s_set_non_blocking_mode(socket_t sock)
	{
		unsigned long on = 1;
		ioctlsocket(sock, FIONBIO, &on);
	}

	const u16	STATUS_NONE = 0;
	const u16	STATUS_SECURE = 0x1000;
	const u16	STATUS_SECURE_RECV = 0x0001;
	const u16	STATUS_SECURE_SEND = 0x0002;
	const u16	STATUS_ACCEPT = 0x0004;
	const u16	STATUS_CONNECT = 0x0008;
	const u16	STATUS_ACCEPT_SECURE_RECV = STATUS_ACCEPT | STATUS_SECURE | STATUS_SECURE_RECV;
	const u16	STATUS_ACCEPT_SECURE_SEND = STATUS_ACCEPT | STATUS_SECURE | STATUS_SECURE_SEND;
	const u16	STATUS_CONNECT_SECURE_SEND = STATUS_CONNECT | STATUS_SECURE | STATUS_SECURE_SEND;
	const u16	STATUS_CONNECT_SECURE_RECV = STATUS_CONNECT | STATUS_SECURE | STATUS_SECURE_RECV;
	const u16	STATUS_CONNECTING = 0x20;
	const u16	STATUS_CONNECTED = 0x040;
	const u16	STATUS_DISCONNECTED = 0x80;
	const u16	STATUS_CLOSE = 0x100;
	const u16	STATUS_CLOSE_IMMEDIATELY = 0x200;

	static bool		status_is(u16 status, u16 check)
	{
		return (status & check) == check;
	}
	static bool		status_is_one_of(u16 status, u16 check)
	{
		return (status & check) != 0;
	}
	static u16		status_set(u16 status, u16 set)
	{
		return status | set;
	}
	static u16		status_clear(u16 status, u16 set)
	{
		return status & ~set;
	}

	struct xconnection
	{
		SOCKET					m_handle;
		time_t					m_last_io_time;
		u16						m_status;
		u16						m_ip_port;
		char					m_ip_str[INET6_ADDRSTRLEN];
		u32						m_sockaddr_len;
		socket_address			m_sockaddr;
		xaddress*				m_address;
		xsocket_tcp*			m_parent;
		xmessage_queue			m_message_queue;
		xmessage*				m_message_read;
		message_socket_reader	m_message_reader;
		message_socket_writer	m_message_writer;
	};

	static void s_init(xconnection* c, xsocket_tcp* parent)
	{
		c->m_handle = INVALID_SOCKET;
		c->m_last_io_time = time(NULL);
		c->m_status = STATUS_NONE;
		c->m_ip_port = 0;
		memset(c->m_ip_str, 0, sizeof(c->m_ip_str));
		c->m_sockaddr_len = 0;
		memset(&c->m_sockaddr, 0, sizeof(socket_address));
		c->m_address = NULL;
		c->m_parent = parent;
		c->m_message_queue.init();
		c->m_message_read = NULL;
		c->m_message_reader.init(INVALID_SOCKET);
		c->m_message_writer.init(INVALID_SOCKET, NULL);
	}
	
	static s32 s_create_socket(const char* host, u16 port, bool listen, bool blockingConnect, xconnection* socket)
	{
		socket->m_handle = NULL;
		socket->m_status = STATUS_NONE;

		addrinfo* info;
		if (host != NULL)
		{
			info = s_get_socket_info(host, port, false);
		}
		else
		{
			info = s_get_socket_info("localhost", port, true);
		}

		struct addrinfo* nextAddr = info;
		while (nextAddr)
		{
			if (nextAddr->ai_family == AF_INET6)
			{	// skip IPv6
				nextAddr = nextAddr->ai_next;
				continue;
			}

			socket->m_handle = ::socket(nextAddr->ai_family, nextAddr->ai_socktype, nextAddr->ai_protocol);
			if (socket->m_handle == -1)
			{
				nextAddr = nextAddr->ai_next;
				continue;
			}
			s_set_blocking_mode(socket->m_handle, blockingConnect);
#ifdef TARGET_PC
			char flag = 1;
#else
			int flag = 1;
			if (::setsockopt(socket->m_handle, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&flag), sizeof(flag)) == -1)
			{
				::closesocket(socket->m_handle);
				return -1;
			}
#endif
			if (::setsockopt(socket->m_handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&flag), sizeof(flag)) == -1)
			{
				::closesocket(socket->m_handle);
				socket->m_handle = -1;
				return -1;
			}
			if (!listen)
			{
				if (::connect(socket->m_handle, nextAddr->ai_addr, nextAddr->ai_addrlen) == -1 && blockingConnect)
				{
					::closesocket(socket->m_handle);
					socket->m_handle = -1;
				}
				else if (blockingConnect)
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
					::closesocket(socket->m_handle);
					socket->m_handle = -1;
				}
				else if (::listen(socket->m_handle, 16) == -1)
				{
					::closesocket(socket->m_handle);
					socket->m_handle = -1;
					return -1;
				}
			}
			if (socket->m_handle == -1)
			{
				nextAddr = nextAddr->ai_next;
				continue;
			}

			if (blockingConnect)
				s_set_blocking_mode(socket->m_handle, false);

			memcpy(&socket->m_sockaddr, nextAddr->ai_addr, nextAddr->ai_addrlen);
			socket->m_sockaddr_len = nextAddr->ai_addrlen;

			break;
		}

		if (socket->m_handle == -1)
		{
			::closesocket(socket->m_handle);
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
			::closesocket(socket->m_handle);
			return -1;
		}

		s_read_sockaddr(&localAddr, socket->m_ip_str, socket->m_ip_port);
		return 0;
	}

	struct xconnections
	{
		u32				m_len;
		u32				m_max;
		xconnection**	m_array;
	};

	void	push_connection(xconnections* self, xconnection* c)
	{
		self->m_array[self->m_len] = c;
		self->m_len += 1;
	}

	bool	pop_connection(xconnections* self, xconnection*& c)
	{
		if (self->m_len == 0)
			return false;
		self->m_len -= 1;
		c = self->m_array[self->m_len];
		return true;
	}

	// An address is a xnetip connected to an ID.
	// When it is actively used it has a valid pointer
	// to a connection.
	// The ID and netip are part of the 'secure' handshake
	// that is done when a connection is established.
	struct xaddress
	{
		xconnection*	m_conn;
		xsockid			m_sockid;
		xnetip			m_netip;
	};

	struct xaddresses
	{
		inline			xaddresses() : m_len(0), m_max(0), m_array(NULL) {}

		u32				m_len;
		u32				m_max;
		xaddress**		m_array;
	};

	void	push_address(xaddresses* self, xaddress* c)
	{
		self->m_array[self->m_len] = c;
		self->m_len += 1;
	}

	bool	pop_address(xaddresses* self, xaddress*& c)
	{
		if (self->m_len == 0)
			return false;
		self->m_len -= 1;
		c = self->m_array[self->m_len];
		return true;
	}


	static void add_to_set(socket_t sock, fd_set *set, socket_t *max_fd)
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

	class xsocket_tcp : public xsocket
	{
		x_iallocator*	m_allocator;
		u16				m_local_port;
		const char*		m_socket_name;	// e.g. Jurgen/CNSHAW1334/10.0.22.76:port/virtuosgames.com
		xsockid			m_sockid;
		xnetip			m_netip;
		xconnection		m_server_socket;

		u32				m_max_open;
		xconnections	m_free_connections;
		xconnections	m_secure_connections;
		xconnections	m_open_connections;

		xaddresses		m_to_connect;
		xaddresses		m_to_disconnect;

		xmessage_queue	m_received_messages;

		xconnection*	accept();

	public:
		inline			xsocket_tcp(x_iallocator* allocator) : m_allocator(allocator)	{ s_attach(); }
						~xsocket_tcp() { s_release(); }

		virtual void	open(u16 port, const char* socket_name, xsockid const& id, u32 max_open);
		virtual void	close();

		virtual void	process(xaddresses& open_conns, xaddresses& closed_conns, xaddresses& new_conns, xaddresses& failed_conns, xaddresses& pex_conns);

		virtual void	connect(xaddress *);
		virtual void	disconnect(xaddress *);

		virtual bool	alloc_msg(xmessage *& msg);
		virtual void	commit_msg(xmessage * msg);
		virtual void	free_msg(xmessage * msg);

		virtual bool	send_msg(xmessage * msg, xaddress * to);
		virtual bool	recv_msg(xmessage *& msg, xaddress *& from);

		XCORE_CLASS_PLACEMENT_NEW_DELETE
	};

	xsocket*		gCreateTcpBasedSocket(x_iallocator* allocator)
	{
		void* mem = allocator->allocate(sizeof(xsocket_tcp), sizeof(void*));
		xsocket_tcp* socket = new (mem) xsocket_tcp(allocator);
		return socket;
	}

	void	xsocket_tcp::open(u16 port, const char* name, xsockid const& id, u32 max_open)
	{
		// Open the server (bind/listen) socket
		m_sockid = id;
		s_create_socket(NULL, port, true, false, &m_server_socket);
	}

	void	xsocket_tcp::close()
	{
		// close all active sockets
		// close server socket
		// free all messages
	}

	xconnection*	xsocket_tcp::accept()
	{
		xconnection *c = NULL;

		socket_address sa;
		socklen_t len = sizeof(sa);

		// NOTE: on Windows, sock is always > FD_SETSIZE
		socket_t sock = ::accept(m_server_socket.m_handle, &sa.sa, &len);
		if (sock != INVALID_SOCKET)
		{
			pop_connection(&m_free_connections, c);
			if (c == NULL)
			{
				closesocket(sock);
				c = NULL;
			}
			else
			{
				s_init(c, this);
				s_set_close_on_exec(sock);
				s_set_non_blocking_mode(sock);
				c->m_parent = this;
				c->m_handle = sock;
				c->m_address = NULL;
				c->m_sockaddr_len = len;
				memcpy(&c->m_sockaddr, &sa, len);
				c->m_status = STATUS_ACCEPT_SECURE_RECV;
			}
		}
		return c;
	}

	void	xsocket_tcp::process(xaddresses& open_connections, xaddresses& closed_connections, xaddresses& new_connections, xaddresses& failed_connections, xaddresses& pex_connections)
	{
		time_t current_time = time(NULL);

		// Non-block connects to remote IP:Port sockets
		xaddress* remote_addr;
		while (pop_address(&m_to_connect, remote_addr))
		{
			xconnection* c = remote_addr->m_conn;
			if (c->m_address == NULL)
			{
				if (pop_connection(&m_free_connections, c))
				{
					s_init(c, this);

					char remote_str[128 + 1];
					remote_str[128] = '\0';
					remote_addr->m_netip.to_string(remote_str, 128, true);
					if (s_create_socket(remote_str, remote_addr->m_netip.get_port(), false, false, c) == 0)
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
		socket_t max_fd = INVALID_SOCKET;
		fd_set read_set, write_set, excp_set;
		FD_ZERO(&read_set);
		FD_ZERO(&write_set);
		FD_ZERO(&excp_set);
		add_to_set(m_server_socket.m_handle, &read_set, &max_fd);
		for (u32 i = 0; i<m_open_connections.m_len; )
		{
			xconnection * conn = m_open_connections.m_array[i];

			//DBG(("%p read_set", conn));
			add_to_set(conn->m_handle, &read_set, &max_fd);

			if (conn->m_message_queue.m_size > 0)
			{
				//DBG(("%p write_set", conn));
				add_to_set(conn->m_handle, &write_set, &max_fd);
			}
			++i;
		}

		// process to-secure sockets, send / receive messages on these sockets
		// process messages for the 'secure' sockets and see if they are now 'secured'
		// any 'secured' sockets add them to 'open' sockets and add their addresses to 'new_connections'
		for (u32 i = 0; i < m_secure_connections.m_len; ++i)
		{
			xconnection* sc = m_secure_connections.m_array[i];
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
		// add them to 'pex_connections'.
		// for any socket that gave an 'error/except-ion' mark them as 'closed', add their addresses to 'closed_connections' and free their messages
		
		// any other messages add them to the 'recv queue'
		s32 const wait_ms = 1;

		timeval tv;
		tv.tv_sec = wait_ms / 1000;
		tv.tv_usec = (wait_ms % 1000) * 1000;

		if (::select((s32)max_fd + 1, &read_set, &write_set, &excp_set, &tv) > 0)
		{
			// select() might have been waiting for a long time, reset current_time
			// now to prevent last_io_time being set to the past.
			current_time = time(NULL);

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
				xconnection * conn = accept();
				if (conn != NULL)
				{
					conn->m_last_io_time = current_time;
					push_connection(&m_secure_connections, conn);
				}
			}

			xmessage_queue rcvd_secure_messages;
			rcvd_secure_messages.init();

			for (s32 i = 0; i<m_open_connections.m_len; )
			{
				xconnection * conn = m_open_connections.m_array[i];

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
						xmessage* curr_msg = conn->m_message_read;
						if (curr_msg == NULL)
						{
							alloc_msg(conn->m_message_read);
							curr_msg = conn->m_message_read;
						}

						s32 status;
						xmessage* rcvd_msg;
						while ((status = conn->m_message_reader.read(curr_msg, rcvd_msg)) > 0)
						{
							if (rcvd_msg != NULL)
							{
								xmessage_hdr* rcvd_hdr = msg_to_hdr(rcvd_msg);
								rcvd_hdr->m_remote = conn->m_address;
								if (status_is(conn->m_status, STATUS_SECURE))
								{
									if (status_is(conn->m_status, STATUS_SECURE_RECV))
									{
										xmessage_reader msg_reader(rcvd_msg);
										xsockid sockid;
										msg_reader.read_data((xbyte*)sockid.m_id, (u32)xsockid::SIZE);

										// Search this ID in our database, if no address found create one
										// and add it to the database.
										// If found compare the netip in the entry with the netip received.
									}
									else
									{
										// Something is wrong
										conn->m_status = STATUS_CLOSE_IMMEDIATELY;
									}
								}
								else
								{
									m_received_messages.push(rcvd_hdr);
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
								// Create a message with our ID and Address details and send it
								xmessage* secure_msg;
								alloc_msg(secure_msg);

								xmessage_writer msg_writer(secure_msg);
								msg_writer.write_data(m_sockid.m_id, xsockid::SIZE);
								xbyte netip_data[xnetip::SERIALIZE_SIZE];
								m_netip.serialize_to(netip_data, xnetip::SERIALIZE_SIZE);
								msg_writer.write_data(netip_data, xnetip::SERIALIZE_SIZE);
								secure_msg->m_size = msg_writer.get_cursor();
								
								conn->m_message_queue.push(secure_msg);
							}
						}

						s32 status;
						xmessage* msg_that_was_send;
						while ((status = conn->m_message_writer.write(msg_that_was_send)) > 0)
						{
							if (msg_that_was_send != NULL)
								free_msg(msg_that_was_send);
						}

						if (status_is(conn->m_status, STATUS_SECURE_SEND))
						{
							if (conn->m_message_queue.empty())
							{
								conn->m_status = status_clear(conn->m_status, STATUS_SECURE_SEND);
								conn->m_status = status_set(conn->m_status, STATUS_SECURE_RECV);
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

		// for all open sockets add their addresses to 'open_connections'
		// Every X seconds build a pex message and send it to the next open connection
	}

	void	xsocket_tcp::connect(xaddress* a)
	{
		push_address(&m_to_connect, a);
	}

	void	xsocket_tcp::disconnect(xaddress* a)
	{
		push_address(&m_to_disconnect, a);
	}

	bool	xsocket_tcp::alloc_msg(xmessage*& msg)
	{
		msg = NULL;
		return false;
	}

	void	xsocket_tcp::commit_msg(xmessage* msg)
	{
		// commit the now actual used message size
	}

	void	xsocket_tcp::free_msg(xmessage* msg)
	{
		// free the message back to the message allocator
	}

	bool	xsocket_tcp::send_msg(xmessage* msg, xaddress* to)
	{
		if (to->m_conn != NULL)
		{
			if (status_is(to->m_conn->m_status, STATUS_CONNECTED))
			{
				// queue the message up in the to-send queue of the associated connection
				xmessage_hdr* msg_hdr = msg_to_hdr(msg);
				to->m_conn->m_message_queue.push(msg_hdr);
				return true;
			}
		}
		return false;
	}

	bool	xsocket_tcp::recv_msg(xmessage*& msg, xaddress*& from)
	{
		// pop any message from the 'recv-queue' until empty
		xmessage_hdr* msg_hdr = m_received_messages.pop();
		if (msg_hdr != NULL)
		{
			from = msg_hdr->m_remote;
			msg = hdr_to_msg(msg_hdr);
			return msg_hdr != NULL;
		}
		else
		{
			from = NULL;
			msg = NULL;
			return false;
		}
	}

}