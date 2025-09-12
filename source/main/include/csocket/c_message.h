#ifndef __CSOCKET_MESSAGE_H__
#define __CSOCKET_MESSAGE_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cbase/c_allocator.h"
#include "cbase/c_buffer.h"

namespace ncore
{
    struct message_t
    {
        inline message_t()
            : m_max(0)
            , m_size(0)
            , m_data(NULL)
        {
        }

        DCORE_CLASS_PLACEMENT_NEW_DELETE

        binary_reader_t get_reader() const;
        binary_writer_t get_writer() const;

        u32   m_max;
        u32   m_size;
        byte *m_data;
    };

}  // namespace ncore

#endif  ///< __CSOCKET_MESSAGE_H__
