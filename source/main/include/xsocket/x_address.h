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
	struct xsock_addrin
	{
		u32		m_len;
		u8		m_data[64];
		u8		m_family;
		u8		m_socktype;
		u8		m_protocol;
		u8		m_dummy1;

		bool	resolve_udp(char* address, u16 port);
		bool	resolve_tcp(char* address, u16 port);

		s32		to_string(char* str, u32 maxstrlen) const;
	};

	struct xsock_hash
	{
		u32		m_len;
		u8		m_hash[32];
	};


	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class xsock_address
	{
	public:
		virtual bool				construct(char* address, u16 port, u8 family, u8 socktype, u8 protocol) = 0;
		virtual xsock_addrin const&	get_addrin() const = 0;
		virtual s32					to_string(char* str, u32 maxstrlen) const = 0;
	};

	class xsock_iaddress_factory
	{
	public:
		virtual xsock_address*	create(xsock_address const& manual_address) = 0;
		virtual void			destroy(xsock_address*) = 0;
	};

	class xsock_iaddrin2address
	{
	public:
		virtual	bool			get_assoc(void* addrin, u32 addrinlen, xsock_address*& assoc) = 0;
		virtual	void			set_assoc(void* addrin, u32 addrinlen, xsock_address* addr) = 0;
		virtual	void			del_assoc(void* addrin, u32 addrinlen) = 0;
	};

	class xsock_iaddress2idx
	{
	public:
		virtual	bool			get_assoc(xsock_address* address, u32& assoc) = 0;
		virtual	void			set_assoc(xsock_address* address, u32 idx) = 0;
		virtual	void			del_assoc(xsock_address* address) = 0;
	};

	class xsock_ihashing
	{
	public:
		virtual xsock_hash		compute_hash(void* data, u32 len) = 0;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class xsock_addresses : public xsock_iaddress_factory, public xsock_iaddrin2address, public xsock_iaddress2idx, public xsock_ihashing
	{
	public:
		virtual xsock_address*	create(xsock_address const& manual_address);
		virtual void			destroy(xsock_address*);

		virtual	bool			get_assoc(void* addrin, u32 addrinlen, xsock_address*& assoc);
		virtual	void			set_assoc(void* addrin, u32 addrinlen, xsock_address* addr);
		virtual	void			del_assoc(void* addrin, u32 addrinlen);

		virtual	bool			get_assoc(xsock_address* address, u32& assoc);
		virtual	void			set_assoc(xsock_address* address, u32 idx);
		virtual	void			del_assoc(xsock_address* address);

		void*					operator new(xcore::xsize_t num_bytes, void* mem) { return mem; }
		void					operator delete(void* mem, void*) { }
		void*					operator new(xcore::xsize_t num_bytes) { return NULL; }
		void					operator delete(void* mem) { }

	private:
		virtual xsock_hash		compute_hash(void* data, u32 len);
	};

}

#endif	// __XSOCKET_ADDRESS_H__