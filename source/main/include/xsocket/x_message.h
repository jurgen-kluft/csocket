//==============================================================================
//  x_message.h
//==============================================================================
#ifndef __XSOCKET_MESSAGE_H__
#define __XSOCKET_MESSAGE_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xbase/x_allocator.h"
#include "xbase/x_buffer.h"

namespace xcore
{
	struct xmessage
	{
		inline xmessage() : m_max(0), m_size(0), m_data(NULL)
		{
		}

		XCORE_CLASS_PLACEMENT_NEW_DELETE

		xbinary_reader get_reader() const;
		xbinary_writer get_writer() const;

		u32    m_max;
		u32    m_size;
		xbyte* m_data;
	};

}    // namespace xcore

#endif    ///< __XSOCKET_MESSAGE_H__
