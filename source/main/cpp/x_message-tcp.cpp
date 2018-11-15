// x_message-tcp.cpp - Core socket functions
#include "xsocket/private/x_message-tcp.h"
#include "xbase/x_debug.h"
#include "xbase/x_target.h"
#include "xsocket/private/x_message.h"
#include "xsocket/x_address.h"
#include "xsocket/x_netip.h"
#include "xsocket/x_socket.h"


#ifdef PLATFORM_PC
#include <stdio.h>
#include <winsock2.h>    // For socket(), connect(), send(), and recv()
#include <ws2tcpip.h>
typedef int  socklen_t;
typedef char raw_type;    // Type used for raw data on this platform
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>     // For inet_addr()
#include <cstdlib>         // For atoi()
#include <cstring>         // For memset()
#include <netdb.h>         // For gethostbyname()
#include <netinet/in.h>    // For sockaddr_in
#include <stdio.h>
#include <sys/socket.h>    // For socket(), connect(), send(), and recv()
#include <sys/types.h>     // For data types
#include <unistd.h>        // For close()
typedef void raw_type;    // Type used for raw data on this platform
#endif

#include <errno.h>    // For errno

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

	s32 message_socket_writer::write(xmessage*& msg)
	{
		msg = NULL;

		if (m_current_msg == NULL)
		{
			m_current_msg = m_send_queue->peek();
			if (m_current_msg != NULL)
			{
				get_msg_payload(m_current_msg, m_data, m_bytes_to_write);
				m_bytes_written = 0;
			}
		}

		if (m_current_msg != NULL)
		{
			s32 n;
			s32 bytes_written = m_bytes_written;
			while ((n = ::send(m_socket, (const char*)(m_data + bytes_written), m_bytes_to_write - bytes_written, 0)) > 0)
			{
				bytes_written += n;
				if (bytes_written == m_bytes_to_write)
					break;
			}
			m_bytes_written = bytes_written;

			if (is_error(n))
				return -1;

			if (bytes_written == m_bytes_to_write)
			{
				m_current_msg = m_send_queue->pop();
				msg           = node_to_msg(m_current_msg);
				return m_send_queue->empty() ? 0 : 1;
			}
			else
			{
				return 1;
			}
		}
		else
		{
			return 0;
		}
	}

	s32 message_socket_reader::read(xmessage*& msg, xmessage*& rcvd)
	{
		rcvd = NULL;

		if (m_msg == NULL)
		{
			m_msg = msg;
			m_hdr = msg_to_node(msg);
			get_msg_payload(m_hdr, m_data, m_data_size);
			m_bytes_read    = 0;
			m_bytes_to_read = 4;
			m_state         = STATE_READ_SIZE;
		}

		s32 n;
		s32 bytes_read = m_bytes_read;
		while ((n = ::recv(m_socket, (char*)(m_data + bytes_read), m_bytes_to_read - bytes_read, 0)) > 0)
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
			m_state         = STATE_READ_BODY;
			m_bytes_to_read = ((u32 const*)m_data)[0];
			m_bytes_read    = 0;
		}

		if (m_bytes_read == m_bytes_to_read)
		{
			rcvd = msg;
			msg  = NULL;
		}

		return n == 0 ? 0 : 1;
	}

}    // namespace xcore