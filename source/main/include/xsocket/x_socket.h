// x_socket.h - Core socket functions 
#ifndef __XSOCKET_SOCKET_H__
#define __XSOCKET_SOCKET_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
	class xsock_init
	{
	public:
		inline		xsock_init() : m_initialized(false) {}

		bool		startUp();
		bool		cleanUp();

	protected:
		bool		m_initialized;
	};

    enum ESocket
    {
        INVALID_SOCKET_DESCRIPTOR = -1,
    };

}

#endif
