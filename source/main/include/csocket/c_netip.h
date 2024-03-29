//==============================================================================
//  x_netip.h
//==============================================================================
#ifndef __XSOCKET_NETIP_H__
#define __XSOCKET_NETIP_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xbase/x_chars.h"

namespace ccore
{
	// ==============================================================================================================================
	// ==============================================================================================================================
	// ==============================================================================================================================
	struct xnetip
	{
		enum econf
		{
			SERIALIZE_SIZE = 20
		};

		enum etype
		{
			NETIP_NONE = 0,
			NETIP_IPV4 = 4,
			NETIP_IPV6 = 16,
		};

		inline xnetip()
		{
			xbyte ip[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
			init(NETIP_NONE, 0, xcbuffer(16, ip));
		}

		inline xnetip(etype _type, u16 _port, xcbuffer& _ip)
		{
			init(_type, _port, _ip);
		}
		inline xnetip(u16 _port, xbyte _ipv41, xbyte _ipv42, xbyte _ipv43, xbyte _ipv44)
		{
			xbyte ip[] = {_ipv41, _ipv42, _ipv43, _ipv44};
			init(NETIP_IPV4, _port, xcbuffer(4, ip));
		}

		void init(etype _type, u16 _port, xcbuffer const& _ip)
		{
			m_type = _type;
			m_port = _port;
			setip(_ip);
		}

		etype get_type() const
		{
			return (etype)m_type;
		}
		u16 get_port() const
		{
			return m_port;
		}

		void set_port(u16 p)
		{
			m_port = p;
		}

		bool is_ip4() const
		{
			return (etype)m_type == NETIP_IPV4;
		}
		bool is_ip6() const
		{
			return (etype)m_type == NETIP_IPV6;
		}

		bool operator==(const xnetip& _other) const
		{
			return is_equal(_other);
		}
		bool operator!=(const xnetip& _other) const
		{
			return !is_equal(_other);
		}

		xbyte operator[](s32 index) const
		{
			return m_ip.ip8[index];
		}

		s32 to_string(xuchars& str, bool omit_port = false) const;

		bool is_equal(const xnetip& ip) const
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

		u32 get_serialization_size() const
		{
			return 2 * sizeof(u16) + 8 * sizeof(u16);
		}
		void serialize_to(xbuffer& dst);
		void deserialize_from(xcbuffer& src);

	private:
		void setip(xcbuffer const& src)
		{
			u32 i = 0;
			for (; i < src.size(); ++i)
				m_ip.ip8[i] = src[i];
			for (; i < sizeof(m_ip); ++i)
				m_ip.ip8[i] = 0;
		}

		u16 m_type;
		u16 m_port;
		union ip {
			u8  ip8[16];
			u16 ip16[8];
		};
		ip m_ip;
	};
}    // namespace ccore

#endif    ///< __XSOCKET_NETIP_H__
