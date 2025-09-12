#ifndef __CSOCKET_MESSAGE_PRIVATE_H__
#define __CSOCKET_MESSAGE_PRIVATE_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cbase/c_allocator.h"
#include "csocket/c_message.h"

namespace ncore
{
    struct address_t;

    struct message_node_t
    {
        message_node_t()
            : m_remote(NULL)
            , m_next(this)
            , m_prev(this)
        {
        }

        void clear()
        {
            m_remote = NULL;
            m_next   = this;
            m_prev   = this;
        }

        void push_back(message_node_t *msg)
        {
            m_next->m_prev = msg;
            m_prev->m_next = msg;
            msg->m_next    = m_next;
            msg->m_prev    = m_prev;
            m_next         = msg;
        }

        message_node_t *peek_front()
        {
            message_node_t *msg = m_prev;
            return msg;
        }

        message_node_t *pop_front()
        {
            message_node_t *msg = m_prev;
            msg->m_prev->m_next = msg->m_next;
            msg->m_next->m_prev = msg->m_prev;
            return msg;
        }

        DCORE_CLASS_PLACEMENT_NEW_DELETE

        address_t      *m_remote;
        message_node_t *m_next;
        message_node_t *m_prev;
    };

    struct message_header_t
    {
        u32 m_msg_size;
        u32 m_msg_flags;
    };

    inline message_t *alloc_msg(alloc_t *allocator, u32 size)
    {
        u32 size_align = 4;
        u32 total_size = sizeof(message_node_t) + sizeof(message_header_t) + sizeof(message_t) + sizeof(void *) + ((size + (size_align - 1)) & ~(size_align - 1));

        void *mem = allocator->allocate(total_size, sizeof(void *));

        void           *mem_hdr = mem;
        message_node_t *hdr     = new (mem_hdr) message_node_t();
        void           *mem_msg = (void *)((byte *)mem + sizeof(message_node_t) + sizeof(message_header_t));
        message_t      *msg     = new (mem_msg) message_t();

        return msg;
    }

    inline message_t *header_to_msg(message_header_t *hdr)
    {
        message_t *msg = (message_t *)((byte *)hdr + sizeof(message_header_t));
        return msg;
    }

    inline message_t *node_to_msg(message_node_t *node)
    {
        message_t *msg = (message_t *)((byte *)node + sizeof(message_node_t) + sizeof(message_header_t));
        return msg;
    }

    inline message_header_t *msg_to_header(message_t *msg)
    {
        message_header_t *hdr = (message_header_t *)((byte *)msg - sizeof(message_header_t));
        return hdr;
    }

    inline message_node_t *msg_to_node(message_t *msg)
    {
        message_node_t *node = (message_node_t *)((byte *)msg - sizeof(message_header_t) - sizeof(message_node_t));
        return node;
    }

    inline void get_msg_payload(message_node_t *node, byte *&payload, u32 &payload_size)
    {
        if (node != NULL)
        {
            message_t *msg   = node_to_msg(node);
            payload          = (byte *)((byte *)msg - sizeof(message_header_t));
            u32 *payload_hdr = (u32 *)payload;
            payload_size     = msg->m_size + sizeof(message_header_t);
            payload_hdr[0]   = msg->m_size;
        }
        else
        {
            payload      = NULL;
            payload_size = 0;
        }
    }

    struct message_queue_t
    {
        u32            m_size;
        message_node_t m_head;

        void init()
        {
            m_size = 0;
            m_head.clear();
        }

        bool empty() const { return m_size == 0; }

        void push(message_t *msg)
        {
            message_node_t *hdr = msg_to_node(msg);
            m_size += 1;
            m_head.push_back(hdr);
        }

        void push(message_node_t *msg)
        {
            m_size += 1;
            m_head.push_back(msg);
        }

        message_node_t *peek()
        {
            if (m_size == 0)
                return NULL;
            return m_head.peek_front();
        }

        message_node_t *pop()
        {
            if (m_size == 0)
                return NULL;
            m_size -= 1;
            return m_head.pop_front();
        }
    };
}  // namespace ncore

#endif  ///< __CSOCKET_MESSAGE_PRIVATE_H__
