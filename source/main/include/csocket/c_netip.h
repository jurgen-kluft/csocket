#ifndef __CSOCKET_NETIP_H__
#define __CSOCKET_NETIP_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "cbase/c_buffer.h"
#include "cbase/c_runes.h"

namespace ncore
{
	// ==============================================================================================================================
	// ==============================================================================================================================
	// ==============================================================================================================================
	struct netip_t
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

		inline netip_t()
		{
			byte ip[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            cbuffer_t b(ip, ip+16);
			init(NETIP_NONE, 0, b);
		}

		inline netip_t(etype _type, u16 _port, cbuffer_t& _ip)
		{
			init(_type, _port, _ip);
		}
		inline netip_t(u16 _port, byte _ipv41, byte _ipv42, byte _ipv43, byte _ipv44)
		{
			byte ip[] = {_ipv41, _ipv42, _ipv43, _ipv44};
			init(NETIP_IPV4, _port, cbuffer_t(ip, ip+4));
		}

		void init(etype _type, u16 _port, cbuffer_t const& _ip)
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

		bool operator==(const netip_t& _other) const
		{
			return is_equal(_other);
		}
		bool operator!=(const netip_t& _other) const
		{
			return !is_equal(_other);
		}

		byte operator[](s32 index) const
		{
			return m_ip.ip8[index];
		}

		s32 to_string(runes_t& str, bool omit_port = false) const;

		bool is_equal(const netip_t& ip) const
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
		void serialize_to(buffer_t& dst);
		void deserialize_from(cbuffer_t& src);

	private:
		void setip(cbuffer_t const& src)
		{
            buffer_t ipb(m_ip.ip8, m_ip.ip8 + sizeof(m_ip));
            ipb.copy_from(src);
            ipb.fill_rest(0);
		}

		u16 m_type;
		u16 m_port;
		union ip {
			u8  ip8[16];
			u16 ip16[8];
		};
		ip m_ip;
	};
}    // namespace ncore

#endif    ///< __CSOCKET_NETIP_H__
