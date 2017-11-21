// x_message-tcp.cpp - Core socket functions 
#include "xbase/x_target.h"
#include "xbase/x_debug.h"
#include "xsocket/x_socket.h"
#include "xsocket/x_address.h"
#include "xsocket/x_netip.h"
#include "xsocket/private/x_message_tcp.h"

#ifdef PLATFORM_PC
    #include <winsock2.h>         // For socket(), connect(), send(), and recv()
    #include <ws2tcpip.h>
    #include <stdio.h>
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
    typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <errno.h>             // For errno

namespace xcore
{
	s32 is_error(s32 n)
	{
		return n == 0 || (n < 0 && errno != EINTR && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK
#ifdef _WIN32
			&& WSAGetLastError() != WSAEINTR && WSAGetLastError() != WSAEWOULDBLOCK
#endif
			);
	}


	s32				message_socket_writer::write(socket_t s)
	{
		s32 n;
		s32 bytes_written = m_bytes_written;
		while ((n = ::send(s, (const char*)(m_data + bytes_written), m_bytes_to_write - bytes_written, 0)) > 0)
		{
			bytes_written += n;
			if (bytes_written == m_bytes_to_write)
				break;
		}
		m_bytes_written = bytes_written;

		if (is_error(n))
			return -1;

		return m_bytes_to_write - bytes_written;
	}

	s32				message_socket_reader::read(socket_t s)
	{
		s32 n;
		s32 bytes_read = m_bytes_read;
		while ((n = ::recv(s, (char*)(m_data + bytes_read), m_bytes_to_read - bytes_read, 0)) > 0)
		{
			bytes_read += n;
			if (bytes_read == m_bytes_to_read)
				break;
		}

		if (is_error(n))
			return -1;

		m_bytes_read = bytes_read;

		if (m_state == STATE_READ_SIZE && m_bytes_read == m_bytes_to_read)
		{
			m_state = STATE_READ_BODY;
			m_bytes_to_read = ((u32 const*)m_data)[0];
			m_bytes_read = 0;
		}

		return m_bytes_to_read - m_bytes_read;
	}
}