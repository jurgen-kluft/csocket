//==============================================================================
//  x_netip.h
//==============================================================================
#ifndef __XSOCKET_NETIP_H__
#define __XSOCKET_NETIP_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

namespace xcore
{
	// ==============================================================================================================================
	// ==============================================================================================================================
	// ==============================================================================================================================
	struct xnetip
	{
		enum etype
		{
			NETIP_NONE = 0,
			NETIP_IPV4 = 4,
			NETIP_IPV6 = 16,
		};

		inline		xnetip()
		{ 
			xbyte	ip[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
			init(NETIP_NONE, 0, ip);
		}

		inline		xnetip(etype _type, u16 _port, xbyte* _ip)			{ init(_type, _port, _ip); }
		inline		xnetip(u16 _port, xbyte _ipv41, xbyte _ipv42, xbyte _ipv43, xbyte _ipv44)
		{
			xbyte	ip[] = { _ipv41, _ipv42, _ipv43, _ipv44 };
			init(NETIP_IPV4, _port, ip);
		}

		void		init(etype _type, u16 _port, xbyte* _ip)
		{
			m_type = _type;
			m_port = _port;
			setip(_ip, (u32)_type);
		}

		etype		get_type() const									{ return (etype)m_type; }
		u16			get_port() const									{ return m_port; }

		void		set_port(u16 p)										{ m_port = p; }

		bool		is_ip4() const										{ return (etype)m_type == NETIP_IPV4; }
		bool		is_ip6() const										{ return (etype)m_type == NETIP_IPV6; }

		bool		operator == (const xnetip& _other) const			{ return is_equal(_other); }
		bool		operator != (const xnetip& _other) const			{ return !is_equal(_other); }

		xbyte		operator [] (s32 index) const						{ return m_ip.ip8[index]; }

		s32			to_string(char* str, u32 max_len, bool omit_port=false) const;

		bool		is_equal(const xnetip& ip) const
		{
			if (m_type == ip.m_type)
			{
				if (m_port == ip.m_port)
				{
					for (s32 i = 0; i < m_type; ++i)
					{
						if (m_ip.ip8[i] != ip.m_ip.ip8[i])
							return false;
					}
					return true;
				}
			}
			return false;
		}

	private:
		void		setip(xbyte* _ip, u32 size)
		{
			u32 i = 0;
			for (; i < size; ++i)
				m_ip.ip8[i] = _ip[i];
			for (; i < sizeof(m_ip); ++i)
				m_ip.ip8[i] = 0;
		}

		u16			m_type;
		u16			m_port;
		union ip
		{
			u8			ip8[16];
			u16			ip16[8];
		};
		ip			m_ip;
	};
}

#endif	///< __XSOCKET_NETIP_H__
