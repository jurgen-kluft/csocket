//==============================================================================
//  x_address.h
//==============================================================================
#ifndef __XSOCKET_ADDRESS_H__
#define __XSOCKET_ADDRESS_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
	typedef		void*	xaddresses;
	typedef		void*	xaddress;
	typedef		void*	xhandshake;

	struct xaddress_data
	{
		u8		id[64];		// Connection ID
		u8		ep[64];		// End-Point information (e.g. 88.128.64.32:5488, IP:Port)
	};

	bool		create(x_iallocator*, u32 max_addresses, xaddresses&);
	void		destroy(xaddresses);

	bool		add_address(xaddresses, xaddress_data*, xaddress& );
	bool		rem_address(xaddresses, xaddress_data*, xaddress& );
	bool		get_address(xaddresses, xaddress_data*, xaddress& );

	class xhandshake
	{
	public:
		virtual void	process(xsssocket, bool& finished, bool& error) = 0;
	};

	// New connections need a handshake to validate/secure the connection
	bool		create(x_iallocator*, xhandshake*&);
	bool		destroy(xhandshake*);

}

#endif	// __XSOCKET_ADDRESS_H__