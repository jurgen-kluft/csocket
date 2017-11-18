// x_socket_tcp.cpp - Core socket functions 
#include "xbase/x_target.h"
#include "xbase/x_debug.h"
#include "xsocket/x_socket.h"
#include "xsocket/x_socket_tcp.h"
#include "xsocket/x_address.h"

#ifdef PLATFORM_PC
    #include <winsock2.h>         // For socket(), connect(), send(), and recv()
    #include <ws2tcpip.h>
    #include <stdio>
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
    #include <stdio>
    #include <cstdlib>	       // For atoi()
    typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <errno.h>             // For errno

namespace xcore
{
	bool	xsock_init::startUp()
	{
		bool result = true;
		if (!m_initialized)
		{
			m_initialized = true;
#ifdef PLATFORM_PC
			WORD wVersionRequested;
			WSADATA wsaData;

			wVersionRequested = MAKEWORD(2, 0);              // Request WinSock v2.0
			if (WSAStartup(wVersionRequested, &wsaData) != 0)	// Load WinSock DLL
			{	// "Unable to load WinSock DLL");
				result = false;
				m_initialized = false;
			}
#endif
		}
		return result;
	}

	bool	xsock_init::cleanUp()
	{
		bool result = true;
		if (m_initialized)
		{
			m_initialized = false;
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
			if (WSACleanup() != 0)
			{
				result = false;
			}
#endif
		}
		return result;
	}

	bool set_blocking_mode(s32 handle, bool blocking) 
	{
#ifdef WINVER
		u32 flag = !blocking;
		if(ioctlsocket(handle, FIONBIO, &flag) != 0)
			return false;
#else
		s32 flags = fcntl(handle, F_GETFL);
		if(blocking)
			flags &= ~O_NONBLOCK;
		else
			flags |= O_NONBLOCK;

		if(fcntl(handle, F_SETFL, flags) == -1)
			return false;
#endif
		return true;
	}

	static void read_sockaddr(const struct sockaddr_storage* addr, char* host, unsigned int& port) 
	{
		char buffer[INET6_ADDRSTRLEN];
		if(addr->ss_family == AF_INET) 
		{
			sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(addr);
			port = ntohs(sin->sin_port);
			inet_ntop(addr->ss_family, (void*)&(sin->sin_addr), buffer, sizeof(buffer));
		} 
		else 
		{
			sockaddr_in6* sin = reinterpret_cast<const struct sockaddr_in6*>(addr);
			port = ntohs(sin->sin6_port);
			inet_ntop(addr->ss_family, (void*)&(sin->sin6_addr), buffer, sizeof(buffer));
		}
		strcpy(host, buffer);
	}

	static addrinfo* get_socket_info(const char* host, unsigned int port, bool wildcardAddress) 
	{
		struct addrinfo conf, *res;
		memset(&conf, 0, sizeof(conf));
		conf.ai_flags = AI_V4MAPPED;
		conf.ai_family = AF_INET;
		if(wildcardAddress)
			conf.ai_flags |= AI_PASSIVE;
		conf.ai_socktype = SOCK_STREAM;

		char portStr[10];
		snprintf(portStr, 10, "%u", port);

		s32 result = getaddrinfo(host, portStr, &conf, &res);
		if(result != 0)
			return NULL;	// ERROR_RESOLVING_ADDRESS
		return res;
	}

	// create a socket and connect or bind
	static void create_socket(const char* host, u16 port, bool listen, bool blockingConnect) 
	{
		addrinfo* info;
		if(host != NULL)
		{
			info = get_socket_info(host, port, false);
		}
		else 
		{
			info = get_socket_info(host, port, true);
		}
		struct addrinfo* nextAddr = info.get();
		while(nextAddr) 
		{
			if(nextAddr->ai_family == AF_INET6) 
			{	// skip IPv6
				nextAddr = nextAddr->ai_next;
				continue;
			}

			handle = socket(nextAddr->ai_family, nextAddr->ai_socktype, nextAddr->ai_protocol);
			if(handle == -1) 
			{
				nextAddr = nextAddr->ai_next;
				continue;
			}
			set_blocking_mode(handle, blockingConnect);
		#ifdef TARGET_PC
			char flag = 1;
		#else
			int flag = 1;
			if(setsockopt(handle, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&flag), sizeof(flag)) == -1) 
			{
				disconnect();
				return -1;
			}
		#endif
			if(setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&flag), sizeof(flag)) == -1) 
			{
				disconnect();
				return -1;
			}
			if (listen)
			{
				if(connect(handle, nextAddr->ai_addr, nextAddr->ai_addrlen) == -1 && blockingConnect) 
				{
					closesocket(handle);
					handle = -1;
				} 
				else if(blockingConnect)
				{
					status = READY;
				}
				else
				{
					status = CONNECTING;
				}
			}
			else
			{
                if(bind(handle, nextAddr->ai_addr, nextAddr->ai_addrlen) == -1) 
				{
                    closesocket(handle);
                    handle = -1;
                }
                else if(listen(handle, status) == -1) 
				{
                    disconnect();
                    return -1;
                }				
			}
			if(handle == -1) 
			{
				nextAddr = nextAddr->ai_next;
				continue;
			}

			if(blockingConnect)
				set_blocking_mode(handle, false);

			break;
		}
		
		if(handle == -1) 
		{
			disconnect();
			return -1;
		}

		struct sockaddr_storage localAddr;
	#ifdef TARGET_PC
		s32 size = sizeof(localAddr);
	#else
		u32 size = sizeof(localAddr);
	#endif
		if(getsockname(handle, reinterpret_cast<struct sockaddr*>(&localAddr), &size) != 0) 
		{
			disconnect();
			return -1;
		}

		char final_ip_str[INET6_ADDRSTRLEN];
		u16 final_port;
		read_sockaddr(&localAddr, final_ip_str, final_port);
	}	



}