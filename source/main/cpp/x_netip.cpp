#include "xbase\x_target.h"
#include "xbase\x_allocator.h"
#include "xbase\x_string_ascii.h"

#include "xsocket\x_netip.h"

namespace xcore
{
	s32	xnetip::to_string(char* str, u32 max_len, bool omit_port) const
	{
		s32 l = 0;
		if (m_type == NETIP_IPV4)
		{
			// Example: 10.0.0.22:port
			if (omit_port)
			{
				l = ascii::sprintf(str, str + max_len, "%u.%u.%u.%u", NULL, x_va(m_ip.ip8[0]), x_va(m_ip.ip8[1]), x_va(m_ip.ip8[2]), x_va(m_ip.ip8[3]));
			}
			else
			{
				l = ascii::sprintf(str, str + max_len, "%u.%u.%u.%u:%u", NULL, x_va(m_ip.ip8[0]), x_va(m_ip.ip8[1]), x_va(m_ip.ip8[2]), x_va(m_ip.ip8[3]), x_va(m_port));
			}
		}
		else if (m_type == NETIP_IPV6)
		{
			// Example: [2001:db8:85a3:8d3:1319:8a2e:370:7348]:443
			if (omit_port)
			{
				l = ascii::sprintf(str, str + max_len, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", NULL, x_va(m_ip.ip16[0]), x_va(m_ip.ip16[1]), x_va(m_ip.ip16[2]), x_va(m_ip.ip16[3]), x_va(m_ip.ip16[4]), x_va(m_ip.ip16[5]), x_va(m_ip.ip16[6]), x_va(m_ip.ip16[7]));
			}
			else
			{
				l = ascii::sprintf(str, str + max_len, "[%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x]:%u", NULL, x_va(m_ip.ip16[0]), x_va(m_ip.ip16[1]), x_va(m_ip.ip16[2]), x_va(m_ip.ip16[3]), x_va(m_ip.ip16[4]), x_va(m_ip.ip16[5]), x_va(m_ip.ip16[6]), x_va(m_ip.ip16[7]), x_va(m_port));
			}
		}
		return l;
	}

	void	xnetip::serialize_to(xbyte* dst, u32 max_len)
	{
		u16 * d = (u16*)dst;
		d[0] = m_type;
		d[1] = m_port;
		d += 2;
		for (s32 i = 0; i < 8; ++i)
			d[i] = m_ip.ip16[i];
	}

	void	xnetip::deserialize_from(xbyte const* src, u32 len)
	{
		u16 const* s = (u16 const*)src;
		m_type = *s++;
		m_port = *s++;
		for (s32 i = 0; i < 8; ++i)
			m_ip.ip16[i] = *s++;
	}

}