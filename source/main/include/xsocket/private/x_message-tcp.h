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

namespace xcore
{
	struct xaddress;

	typedef u32		socket_t;

	struct xmessage
	{
		inline			xmessage() : m_max(0), m_size(0), m_data(NULL) {}

		XCORE_CLASS_PLACEMENT_NEW_DELETE

		u32				m_max;
		u32				m_size;
		xbyte*			m_data;
	};

	struct xmessage_hdr
	{
		xmessage_hdr()
			: m_remote(NULL)
			, m_next(this)
			, m_prev(this)
		{
		}

		void			clear()
		{
			m_remote = NULL;
			m_next = this;
			m_prev = this;
		}

		void			push_back(xmessage_hdr* msg)
		{
			m_next->m_prev = msg;
			m_prev->m_next = msg;
			msg->m_next = m_next;
			msg->m_prev = m_prev;
			m_next = msg;
		}

		xmessage_hdr*	peek_front()
		{
			xmessage_hdr* msg = m_prev;
			return msg;
		}

		xmessage_hdr*	pop_front()
		{
			xmessage_hdr* msg = m_prev;
			msg->m_prev->m_next = msg->m_next;
			msg->m_next->m_prev = msg->m_prev;
			return msg;
		}

		XCORE_CLASS_PLACEMENT_NEW_DELETE

		xaddress*			m_remote;
		xmessage_hdr*		m_next;
		xmessage_hdr*		m_prev;
	};

	inline xmessage*		alloc_msg(x_iallocator* allocator, u32 size)
	{
		u32 size_align = 4;
		u32 total_size = sizeof(xmessage_hdr) + 2 * sizeof(u32) + sizeof(xmessage) + ((size + (size_align - 1)) & ~(size_align - 1));

		void* mem = allocator->allocate(total_size, sizeof(void*));

		void* mem_hdr = mem;
		xmessage_hdr* hdr = new (mem_hdr) xmessage_hdr();
		void* mem_msg = (void*)((xbyte*)mem + sizeof(xmessage_hdr) + 2 * sizeof(u32));
		xmessage* msg = new (mem_msg) xmessage();

		return msg;
	}

	inline xmessage*		hdr_to_msg(xmessage_hdr* hdr)
	{
		xmessage* msg = (xmessage*)((xbyte*)hdr + sizeof(xmessage_hdr) + 2 * sizeof(u32));
		return msg;
	}

	inline xmessage_hdr*	msg_to_hdr(xmessage* msg)
	{
		xmessage_hdr* hdr = (xmessage_hdr*)((xbyte*)msg - sizeof(xmessage_hdr) - 2 * sizeof(u32));
		return hdr;
	}

	inline void			hdr_to_payload(xmessage_hdr* hdr, xbyte*& payload, u32& payload_size)
	{
		if (hdr != NULL)
		{
			xmessage* msg = hdr_to_msg(hdr);
			payload = (xbyte*)((xbyte*)msg - sizeof(u32));
			u32* payload_hdr = (u32*)payload;
			payload_size = msg->m_size + 4;
			payload_hdr[0] = msg->m_size;
		}
		else
		{
			payload = NULL;
			payload_size = 0;
		}
	}

	struct xmessage_queue
	{
		u32				m_size;
		xmessage_hdr	m_head;

		void			init()
		{
			m_size = 0;
			m_head.clear();
		}

		bool			empty() const { return m_size == 0; }

		void			push(xmessage* msg)
		{
			xmessage_hdr*  hdr = msg_to_hdr(msg);
			m_size += 1;
			m_head.push_back(hdr);
		}

		void			push(xmessage_hdr* msg)
		{
			m_size += 1;
			m_head.push_back(msg);
		}

		xmessage_hdr*	peek()
		{
			if (m_size == 0)
				return NULL;
			return m_head.peek_front();
		}

		xmessage_hdr*	pop()
		{
			if (m_size == 0)
				return NULL;
			m_size -= 1;
			return m_head.pop_front();
		}
	};

	class message_socket_writer
	{
		xmessage_queue*	m_send_queue;
		xmessage_hdr*	m_current_msg;
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
		xmessage_hdr*	m_hdr;
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

	class xmessage_reader
	{
	public:
		inline				xmessage_reader(xmessage* _msg = NULL) : cursor_(0), msg_(_msg) {}

		void				set_cursor(u32);
		u32					get_cursor() const;

		u32					get_size() const;

		bool				can_read(u32 number_of_bytes) const;		// check if we still can read n number of bytes

		u32					read(bool&);
		u32					read(u8  &);
		u32					read(s8  &);
		u32					read(u16 &);
		u32					read(s16 &);
		u32					read(u32 &);
		u32					read(s32 &);
		u32					read(u64 &);
		u32					read(s64 &);
		u32					read(f32 &);
		u32					read(f64 &);

		bool				read_data(xbyte * _data, u32 _size);
		bool				view_data(xbyte const*& _data, u32 _size);
		bool				view_string(char const*& _str, u32& _out_len);

	protected:
		u32					cursor_;
		xmessage*			msg_;
	};

	class xmessage_writer
	{
	public:
		inline				xmessage_writer(xmessage* _msg) : cursor_(0), msg_(_msg) { }

		void				set_cursor(u32);
		u32					get_cursor() const;

		bool				can_write(u32 num_bytes = 0) const;

		u32					write(bool);
		u32					write(u8);
		u32					write(s8);
		u32					write(u16);
		u32					write(s16);
		u32					write(u32);
		u32					write(s32);
		u32					write(u64);
		u32					write(s64);
		u32					write(f32);
		u32					write(f64);

		u32					write_data(const xbyte*, u32);
		u32					write_string(const char*, u32 _len = 0);

	protected:
		u32					cursor_;
		xmessage*			msg_;
	};
}

#endif	///< __XSOCKET_MESSAGE_READER_WRITER_TCP_H__