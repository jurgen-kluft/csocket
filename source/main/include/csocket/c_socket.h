// x_socket.h - Core socket functions
#ifndef __XSOCKET_SOCKET_H__
#define __XSOCKET_SOCKET_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xbase/x_buffer.h"
#include "xbase/x_chars.h"

namespace ccore
{
	class x_iallocator;

	struct xaddress;
	struct xaddresses;
	struct xmessage;

	typedef xbytes32 xsockid;

	class xsocket
	{
	protected:
		static void s_attach();
		static void s_release();

	public:
		virtual ~xsocket()
		{
		}

		virtual void open(u16 port, xcuchars const& name, xsockid const& id, u32 max_open) = 0;
		virtual void close()                                                               = 0;

		virtual void process(xaddresses& open_conns, xaddresses& closed_conns, xaddresses& new_conns, xaddresses& failed_conns, xaddresses& pex_conns) = 0;

		virtual void connect(xaddress*)    = 0;
		virtual void disconnect(xaddress*) = 0;

		virtual bool alloc_msg(xmessage*& msg) = 0;
		virtual void commit_msg(xmessage* msg) = 0;
		virtual void free_msg(xmessage* msg)   = 0;

		virtual bool send_msg(xmessage* msg, xaddress* to)     = 0;
		virtual bool recv_msg(xmessage*& msg, xaddress*& from) = 0;
	};

	xsocket* gCreateTcpBasedSocket(x_iallocator*);
}    // namespace ccore

#endif
