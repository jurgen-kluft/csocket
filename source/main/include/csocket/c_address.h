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

namespace ccore
{
	struct xaddress;

	class xaddress_registry
	{
	public:
		static bool create(x_iallocator* alloc, u32 max_addresses, xaddress_registry*& addr);
		static void destroy(xaddress_registry* addr);

		virtual bool add(xbytes32 const& addr_id, xbytes32 const& addr_ep) = 0;
		virtual bool get(xbytes32 const& addr_id, xbytes32& addr_ep)       = 0;
		virtual bool rem(xbytes32 const& addr_id)                          = 0;
	};
}    // namespace ccore

#endif    // __XSOCKET_ADDRESS_H__