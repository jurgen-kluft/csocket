// x_socket.cpp - Core socket functions 
#include "xbase/x_target.h"
#include "xbase/x_debug.h"
#include "xsocket/x_socket.h"
#include "xsocket/x_socket_udp.h"
#include "xsocket/x_socket_tcp.h"
#include "xsocket/x_address.h"

#ifdef PLATFORM_PC
    #include <winsock2.h>         // For socket(), connect(), send(), and recv()
    #include <ws2tcpip.h>
    typedef int socklen_t;
    typedef char raw_type;       // Type used for raw data on this platform
#else
    #include <sys/types.h>       // For data types
    #include <sys/socket.h>      // For socket(), connect(), send(), and recv()
    #include <netdb.h>           // For gethostbyname()
    #include <arpa/inet.h>       // For inet_addr()
    #include <unistd.h>          // For close()
    #include <netinet/in.h>      // For sockaddr_in
    #include <cstring>	       // For memset()
    #include <cstdlib>	       // For atoi()
    typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <errno.h>             // For errno

namespace xcore
{
	bool	xsock_init::startUp()
	{
		bool result = true;
#ifdef PLATFORM_PC
		if (!m_initialized)
		{
			WORD wVersionRequested;
			WSADATA wsaData;

			wVersionRequested = MAKEWORD(2, 0);              // Request WinSock v2.0
			if (WSAStartup(wVersionRequested, &wsaData) != 0)	// Load WinSock DLL
			{	// "Unable to load WinSock DLL");
				result = false;
				m_initialized = false;
			}
			else
			{
				m_initialized = true;
			}
		}
#endif
		return result;
	}

	bool	xsock_init::cleanUp()
	{
		bool result = true;
#ifdef PLATFORM_PC
		/**
		*   If WinSock, unload the WinSock DLLs; otherwise do nothing.  We ignore
		*   this in our sample client code but include it in the library for
		*   completeness.  If you are running on Windows and you are concerned
		*   about DLL resource consumption, call this after you are done with all
		*   Socket instances.  If you execute this on Windows while some instance of
		*   Socket exists, you are toast.  For portability of client code, this is
		*   an empty function on non-Windows platforms so you can always include it.
		*   @param buffer buffer to receive the data
		*   @param bufferLen maximum number of bytes to read into buffer
		*   @return number of bytes read, 0 for EOF, and -1 for error
		*/
		if (m_initialized)
		{
			if (WSACleanup() != 0)
			{
				result = false;
			}
#endif
			m_initialized = false;
		}
		return result;
	}


	// Function to fill in address structure given an address and port
	static s32 fillAddr(const char* address, u16 port, sockaddr_in &addr)
	{
		memset(&addr, 0, sizeof(addr));  // Zero out address structure
		addr.sin_family = AF_INET;       // Internet address

		hostent *host;  // Resolve name
		if ((host = gethostbyname(address)) == NULL)
		{
			// strerror() will not work for gethostbyname() and hstrerror() 
			// is supposedly obsolete
			// "Failed to resolve name (gethostbyname())"
			return -1;
		}
		addr.sin_addr.s_addr = *((unsigned long *)host->h_addr_list[0]);
		addr.sin_port = htons(port);     // Assign port in network byte order
		return 0;
	}
		
	// socket_base Code

	xsock_udp::xsock_udp(xsock_iaddrin2address * iaddr_to_address)
		: m_addr2address(iaddr_to_address)
		, m_socket_descriptor(INVALID_SOCKET_DESCRIPTOR)
	{
	}

	xsock_udp::~xsock_udp()
	{
#ifdef PLATFORM_PC
		::closesocket(m_socket_descriptor);
#else
		::close(m_socket_descriptor);
#endif
		m_socket_descriptor = -1;
	}

	bool xsock_udp::valid() const
	{
		return m_socket_descriptor != INVALID_SOCKET_DESCRIPTOR;
	}

	bool xsock_udp::open_socket(s32 type, s32 protocol)
	{
		bool result = false;

		{
			if (m_socket_descriptor == INVALID_SOCKET_DESCRIPTOR)
			{
				// Make a new socket
				if ((m_socket_descriptor = ::socket(PF_INET, type, protocol)) < 0)
				{
					// socket_base creation failed (socket())
					m_socket_descriptor = INVALID_SOCKET_DESCRIPTOR;
				}
			}
			result = m_socket_descriptor != INVALID_SOCKET_DESCRIPTOR;
		}
		return result;
	}

	bool xsock_udp::open(xsock_address const* local)
	{
		m_socket_address = local;
		return open_socket(SOCK_DGRAM, IPPROTO_UDP);
	}

	void xsock_udp::setBroadcast()
	{
		// If this fails, we'll hear about it when we try to send.  This will allow 
		// system that cannot broadcast to continue if they don't plan to broadcast
		s32 broadcastPermission = 1;
		setsockopt(m_socket_descriptor, SOL_SOCKET, SO_BROADCAST, (raw_type *)&broadcastPermission, sizeof(broadcastPermission));
	}

	void xsock_udp::disconnect()
	{
		sockaddr_in nullAddr;
		memset(&nullAddr, 0, sizeof(nullAddr));
		nullAddr.sin_family = AF_UNSPEC;

		// Try to disconnect
		if (::connect(m_socket_descriptor, (sockaddr *)&nullAddr, sizeof(nullAddr)) < 0)
		{
#ifdef PLATFORM_PC
			if (errno != WSAEAFNOSUPPORT)
			{
#else
			if (errno != EAFNOSUPPORT)
			{
#endif
				// "Disconnect failed (connect())", true);
			}
		}
		m_socket_descriptor = INVALID_SOCKET_DESCRIPTOR;
	}

	s32 xsock_udp::sendTo(const void *buffer, s32 bufferLen, xsock_address const* remote)
	{
		sockaddr_in destAddr;
			
		// Write out the whole buffer as a single message.
		s32 const actually_send = ::sendto(m_socket_descriptor, (raw_type *)buffer, bufferLen, 0, (sockaddr *)&destAddr, sizeof(destAddr));
		if (actually_send != bufferLen)
		{
			// "Send failed (sendto())", true);
			return -actually_send;
		}
		return bufferLen;
	}

	s32 xsock_udp::recvFrom(void *buffer, s32 bufferLen, xsock_address const*& remote)
	{
		sockaddr_in clntAddr;
		socklen_t addrLen = sizeof(clntAddr);
		s32 rtn;
		if ((rtn = recvfrom(m_socket_descriptor, (raw_type *)buffer, bufferLen, 0, (sockaddr *)&clntAddr, (socklen_t *)&addrLen)) < 0)
		{
			// "Receive failed (recvfrom())", true);
		}
		//sourceAddress = inet_ntoa(clntAddr.sin_addr);
		//sourcePort = ntohs(clntAddr.sin_port);

		return rtn;
	}

	void xsock_udp::setMulticastTTL(unsigned char multicastTTL)
	{
		if (setsockopt(m_socket_descriptor, IPPROTO_IP, IP_MULTICAST_TTL, (raw_type *)&multicastTTL, sizeof(multicastTTL)) < 0)
		{
			// "Multicast TTL set failed (setsockopt())", true);
		}
	}

	void xsock_udp::joinGroup(const char* multicastGroup)
	{
		ip_mreq multicastRequest;

		multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup);
		multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(m_socket_descriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (raw_type *)&multicastRequest, sizeof(multicastRequest)) < 0)
		{
			// "Multicast group join failed (setsockopt())", true);
		}
	}

	void xsock_udp::leaveGroup(const char* multicastGroup)
	{
		ip_mreq multicastRequest;

		multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup);
		multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(m_socket_descriptor, IPPROTO_IP, IP_DROP_MEMBERSHIP, (raw_type *)&multicastRequest, sizeof(multicastRequest)) < 0)
		{
			// "Multicast group leave failed (setsockopt())", true);
		}
	}

}