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

		void			push_back(xmessage_hdr* msg)
		{
			m_next->m_prev = msg;
			m_prev->m_next = msg;
			msg->m_next = m_next;
			msg->m_prev = m_prev;
			m_next = msg;
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
		xmessage* msg = hdr_to_msg(hdr);
		payload = (xbyte*)((xbyte*)msg - sizeof(u32));
		u32* payload_hdr = (u32*)payload;
		payload_size = msg->m_size + 4;
		payload_hdr[0] = msg->m_size;
	}

	struct xmessage_queue
	{
		u32				m_size;
		xmessage_hdr	m_head;

		void			push(xmessage_hdr* msg)
		{
			m_size += 1;
			m_head.push_back(msg);
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
		xmessage_hdr*	m_hdr;
		xbyte*			m_data;
		u32				m_bytes_written;
		u32				m_bytes_to_write;

	public:
		void			reset(xmessage_hdr* hdr)
		{
			m_hdr = hdr;
			hdr_to_payload(hdr, m_data, m_bytes_to_write);
			m_bytes_written = 0;
		}

		bool			write(xmessage*& msg)
		{
			return false;
		}

		s32				write(socket_t s);
	};

	class message_socket_reader
	{
		xmessage_hdr*	m_hdr;
		xbyte*			m_data;
		u32				m_bytes_read;
		u32				m_bytes_to_read;

		enum EState { STATE_READ_SIZE = 0, STATE_READ_BODY = 1 };
		u32				m_state;

	public:
		message_socket_reader()
			: m_hdr(NULL)
			, m_data(NULL)
			, m_bytes_read(0)
			, m_bytes_to_read(0)
			, m_state(STATE_READ_SIZE)
		{

		}

		bool			read(xmessage*& msg)
		{
			return false;
		}

		void			reset(xmessage_hdr* hdr)
		{
			m_hdr = hdr;
			hdr_to_payload(hdr, m_data, m_bytes_to_read);
			m_bytes_to_read = 4;
			m_bytes_read = 0;
			m_state = STATE_READ_SIZE;
		}

		xmessage_hdr*	get_msg_hdr() { return m_hdr; }

		// @return
		//   > 0 = still data left to read
		//     0 = done reading
		//   < 0 = an error occured during reading

		s32		read(socket_t s);

	};
}

#endif	///< __XSOCKET_MESSAGE_READER_WRITER_TCP_H__
