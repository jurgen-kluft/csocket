// x_socket.h - Core TCP socket functions 
#ifndef __XSOCKET_SOCKET_TCP_H__
#define __XSOCKET_SOCKET_TCP_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
	class x_iallocator;
	class xsock_address;
	class xsock_addresses;
	class xsock_tcp;

	// ------------------------------------------------------------------------------------------
	// TCP API - IPv4
	// ------------------------------------------------------------------------------------------
	typedef		void*		xsscontext;
	typedef		void*		xssaddress;
	typedef		void*		xsssocket;
	typedef		void*		xsslisten;
	typedef		void*		xsspoller;
	typedef		u32			xssipv4;
	typedef		u16			xssport;
	typedef		s32			xssstate;
	
	xsscontext	context_create_ipv4(x_iallocator*, u32 max_sockets);
	void		context_destroy(xsscontext);

	xssaddress	to_address(const char* ip, xssport port);
	xssaddress	to_address(xssipv4 ip, xssport port);

	xsssocket	connect(xsscontext, xssaddress);
	xssaddress	get_address(xsssocket);
	void		disconnect(xsssocket);
	void		close(xsssocket);

	void		set_non_blocking(xsssocket);

	bool		is_connecting(xsssocket);
	bool		is_connected(xsssocket);

	s32			send(xsssocket, xbyte* buffer, u32 len);
	s32			recv(xsssocket, xbyte* buffer, u32& len);

	xsslisten	listen(xsscontext, xssport);
	xsssocket	accept(xsslisten, u32 timeout_us);
	void		close(xsslisten);

	xsspoller	poller_create(xsscontext, u32 max_sockets);
	void		poller_destroy(xsspoller);
	void		poller_poll(xsspoller* p, xsssocket* readers, u32 num_readers, xsssocket* writers, u32 num_writers, u32 timeout_us);
	bool		poller_read(xsspoller* p, xsssocket, bool& can_read, bool& can_write, bool& has_error);

}

#endif
