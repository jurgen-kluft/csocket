//==============================================================================
//  x_message_reader_writer_tcp.h
//==============================================================================
#ifndef __XSOCKET_MESSAGE_READER_WRITER_TCP_H__
#define __XSOCKET_MESSAGE_READER_WRITER_TCP_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xbase/x_allocator.h"

namespace ccore
{
	typedef u32		socket_t;
	
	struct xmessage_node;
	struct xmessage_queue;


	class message_socket_writer
	{
		xmessage_queue*	m_send_queue;
		xmessage_node*	m_current_msg;
		xbyte*			m_data;
		u32				m_bytes_written;
		u32				m_bytes_to_write;
		socket_t 		m_socket;

	public:
		void			init(socket_t sock, xmessage_queue* send_queue)
		{
			m_socket = sock;
			m_send_queue = send_queue;
			m_bytes_written = 0;
		}

		// 
		// Supply a pointer to a message in @msg, when this becomes
		// NULL it means that the message has been fully written to
		// the socket. That is ofcourse if no '-1' has been returned.
		// 
		// return:
		//   -1 -> an error occured, better close this socket
		//    0 -> socket cannot be written to anymore
		//    1 -> still need to write data
		s32				write(xmessage*& msg);
	};

	class message_socket_reader
	{
		xmessage*		m_msg;
		xmessage_node*	m_hdr;
		xbyte*			m_data;
		u32				m_data_size;
		u32				m_bytes_read;
		u32				m_bytes_to_read;
		socket_t		m_socket;
		enum EState { STATE_READ_SIZE = 0, STATE_READ_BODY = 1 };
		u32				m_state;

	public:
		message_socket_reader()
			: m_msg(NULL)
			, m_hdr(NULL)
			, m_data(NULL)
			, m_bytes_read(0)
			, m_bytes_to_read(0)
			, m_socket(0)
			, m_state(STATE_READ_SIZE)
		{
		}

		void			init(socket_t sock)
		{
			m_socket = sock;
			m_msg = NULL;
			m_hdr = NULL;
			m_data = NULL;
			m_bytes_to_read = 0;
			m_bytes_read = 0;
			m_state = STATE_READ_SIZE;
		}

		// 
		// supply a pointer to a message in @msg, when this
		// becomes NULL @rcvd will contain a pointer to the
		// message that is now complete.
		// 
		// return:
		//   -1 -> an error occured, better close this socket
		//    0 -> socket has no more data
		//    1 -> socket still has data to read
		s32				read(xmessage*& msg, xmessage*& rcvd);
	};

}

#endif	///< __XSOCKET_MESSAGE_READER_WRITER_TCP_H__
