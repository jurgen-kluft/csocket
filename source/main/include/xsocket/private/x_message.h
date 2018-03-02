//==============================================================================
//  private x_message.h
//==============================================================================
#ifndef __XSOCKET_MESSAGE_PRIVATE_H__
#define __XSOCKET_MESSAGE_PRIVATE_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xbase/x_allocator.h"
#include "xsocket/x_message.h"

namespace xcore
{
	struct xaddress;

	struct xmessage_node
	{
		xmessage_node()
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

		void			push_back(xmessage_node* msg)
		{
			m_next->m_prev = msg;
			m_prev->m_next = msg;
			msg->m_next = m_next;
			msg->m_prev = m_prev;
			m_next = msg;
		}

		xmessage_node*	peek_front()
		{
			xmessage_node* msg = m_prev;
			return msg;
		}

		xmessage_node*	pop_front()
		{
			xmessage_node* msg = m_prev;
			msg->m_prev->m_next = msg->m_next;
			msg->m_next->m_prev = msg->m_prev;
			return msg;
		}

		XCORE_CLASS_PLACEMENT_NEW_DELETE

		xaddress*			m_remote;
		xmessage_node*		m_next;
		xmessage_node*		m_prev;
	};

	struct xmessage_header
	{
		u32			m_msg_size;
		u32			m_msg_flags;
	};

	inline xmessage*		alloc_msg(x_iallocator* allocator, u32 size)
	{
		u32 size_align = 4;
		u32 total_size = sizeof(xmessage_node) + sizeof(xmessage_header) + sizeof(xmessage) + sizeof(void*) + ((size + (size_align - 1)) & ~(size_align - 1));

		void* mem = allocator->allocate(total_size, sizeof(void*));

		void* mem_hdr = mem;
		xmessage_node* hdr = new (mem_hdr) xmessage_node();
		void* mem_msg = (void*)((xbyte*)mem + sizeof(xmessage_node) + sizeof(xmessage_header));
		xmessage* msg = new (mem_msg) xmessage();

		return msg;
	}

	inline xmessage*		header_to_msg(xmessage_header* hdr)
	{
		xmessage* msg = (xmessage*)((xbyte*)hdr + sizeof(xmessage_header));
		return msg;
	}

	inline xmessage*		node_to_msg(xmessage_node* node)
	{
		xmessage* msg = (xmessage*)((xbyte*)node + sizeof(xmessage_node) + sizeof(xmessage_header));
		return msg;
	}

	inline xmessage_header*	msg_to_header(xmessage* msg)
	{
		xmessage_header* hdr = (xmessage_header*)((xbyte*)msg - sizeof(xmessage_header));
		return hdr;
	}

	inline xmessage_node*	msg_to_node(xmessage* msg)
	{
		xmessage_node* node = (xmessage_node*)((xbyte*)msg - sizeof(xmessage_header) - sizeof(xmessage_node));
		return node;
	}

	inline void			get_msg_payload(xmessage_node* node, xbyte*& payload, u32& payload_size)
	{
		if (node != NULL)
		{
			xmessage* msg = node_to_msg(node);
			payload = (xbyte*)((xbyte*)msg - sizeof(xmessage_header));
			u32* payload_hdr = (u32*)payload;
			payload_size = msg->m_size + sizeof(xmessage_header);
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
		xmessage_node	m_head;

		void			init()
		{
			m_size = 0;
			m_head.clear();
		}

		bool			empty() const { return m_size == 0; }

		void			push(xmessage* msg)
		{
			xmessage_node*  hdr = msg_to_node(msg);
			m_size += 1;
			m_head.push_back(hdr);
		}

		void			push(xmessage_node* msg)
		{
			m_size += 1;
			m_head.push_back(msg);
		}

		xmessage_node*	peek()
		{
			if (m_size == 0)
				return NULL;
			return m_head.peek_front();
		}

		xmessage_node*	pop()
		{
			if (m_size == 0)
				return NULL;
			m_size -= 1;
			return m_head.pop_front();
		}
	};

}

#endif	///< __XSOCKET_MESSAGE_PRIVATE_H__
