//==============================================================================
//  x_address.h
//==============================================================================
#ifndef __XSOCKET_ADDRESS_H__
#define __XSOCKET_ADDRESS_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xbase/x_buffer.h"

namespace xcore
{
	struct xaddress;

	class xaddresses
	{
	public:
		static bool		create(x_iallocator* alloc, u32 max_addresses, xaddresses*& addr);
		static void		destroy(xaddresses* addr);

		virtual bool	add(xbuffer32 const& addr_id, xbuffer32 const& addr_ep) = 0;
		virtual bool	get(xbuffer32 const& addr_id, xbuffer32& addr_ep) = 0;
		virtual bool	rem(xbuffer32 const& addr_id) = 0;
	};
}

#endif	// __XSOCKET_ADDRESS_H__