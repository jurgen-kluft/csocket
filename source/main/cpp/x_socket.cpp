// x_socket_tcp.cpp - Core socket functions 
#include "xbase/x_target.h"
#include "xbase/x_debug.h"
#include "xsocket/x_socket.h"
#include "xsocket/x_address.h"

#ifdef PLATFORM_PC
    #include <winsock2.h>         // For socket(), connect(), send(), and recv()
    #include <ws2tcpip.h>
    #include <stdio.h>
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
	class xsock_init
	{
	public:
		static bool		attach();
		static bool		release();
	protected:
		static s32		s_initialized;
	};

	s32	xsock_init::s_initialized = 0;

	bool	xsock_init::attach()
	{
		bool result = true;
		if (s_initialized == 0)
		{
			s_initialized += 1;
#ifdef PLATFORM_PC
			WORD wVersionRequested;
			WSADATA wsaData;

			wVersionRequested = MAKEWORD(2, 0);              // Request WinSock v2.0
			if (WSAStartup(wVersionRequested, &wsaData) != 0)	// Load WinSock DLL
			{	// "Unable to load WinSock DLL");
				result = false;
				s_initialized = false;
			}
#endif
		}
		return result;
	}

	bool	xsock_init::release()
	{
		bool result = false;
		s_initialized -= 1;
		if (s_initialized == 0)
		{
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
			if (WSACleanup() == 0)
			{
				result = true;
			}
#endif
		}
		return result;
	}

	void	xsocket::s_attach()
	{
		xsock_init::attach();
	}
	void	xsocket::s_release()
	{
		xsock_init::release();
	}




}