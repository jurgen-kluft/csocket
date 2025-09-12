#ifndef __CSOCKET_MESSAGE_READER_WRITER_TCP_H__
#define __CSOCKET_MESSAGE_READER_WRITER_TCP_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cbase/c_allocator.h"

namespace ncore
{
    typedef s32 sd_t;  // socket descriptor type

    struct message_t;
    struct message_node_t;
    struct message_queue_t;

    class message_socket_writer
    {
        message_queue_t* m_send_queue;
        message_node_t*  m_current_msg;
        byte*            m_data;
        u32              m_bytes_written;
        u32              m_bytes_to_write;
        sd_t             m_socket;

    public:
        void init(sd_t sock, message_queue_t* send_queue)
        {
            m_socket        = sock;
            m_send_queue    = send_queue;
            m_bytes_written = 0;
        }

        //
        // Supply a pointer to a message in @msg, when this becomes
        // nullptr it means that the message has been fully written to
        // the socket. That is ofcourse if no '-1' has been returned.
        //
        // return:
        //   -1 -> an error occured, better close this socket
        //    0 -> socket cannot be written to anymore
        //    1 -> still need to write data
        s32 write(message_t*& msg);
    };

    class message_socket_reader
    {
        message_t*      m_msg;
        message_node_t* m_hdr;
        byte*           m_data;
        u32             m_data_size;
        u32             m_bytes_read;
        u32             m_bytes_to_read;
        sd_t            m_socket;
        enum EState
        {
            STATE_READ_SIZE = 0,
            STATE_READ_BODY = 1
        };
        u32 m_state;

    public:
        message_socket_reader()
            : m_msg(nullptr)
            , m_hdr(nullptr)
            , m_data(nullptr)
            , m_bytes_read(0)
            , m_bytes_to_read(0)
            , m_socket(0)
            , m_state(STATE_READ_SIZE)
        {
        }

        void init(sd_t sock)
        {
            m_socket        = sock;
            m_msg           = nullptr;
            m_hdr           = nullptr;
            m_data          = nullptr;
            m_bytes_to_read = 0;
            m_bytes_read    = 0;
            m_state         = STATE_READ_SIZE;
        }

        //
        // supply a pointer to a message in @msg, when this
        // becomes nullptr @rcvd will contain a pointer to the
        // message that is now complete.
        //
        // return:
        //   -1 -> an error occured, better close this socket
        //    0 -> socket has no more data
        //    1 -> socket still has data to read
        s32 read(message_t*& msg, message_t*& rcvd);
    };

}  // namespace ncore

#endif  ///< __CSOCKET_MESSAGE_READER_WRITER_TCP_H__
