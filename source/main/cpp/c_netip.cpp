#include "xbase\x_allocator.h"
#include "xbase\x_string_ascii.h"
#include "xbase\x_target.h"

#include "xsocket\x_netip.h"

namespace ccore
{
	s32 xnetip::to_string(xuchars& str, bool omit_port) const
	{
		s32 l = 0;
		if (m_type == NETIP_IPV4)
		{
			// Example: 10.0.0.22:port
			if (omit_port)
			{
				ascii::sprintf(str, xcuchars("%u.%u.%u.%u"), x_va(m_ip.ip8[0]), x_va(m_ip.ip8[1]), x_va(m_ip.ip8[2]), x_va(m_ip.ip8[3]));
				l = 1;
			}
			else
			{
				ascii::sprintf(str, xcuchars("%u.%u.%u.%u:%u"), x_va(m_ip.ip8[0]), x_va(m_ip.ip8[1]), x_va(m_ip.ip8[2]), x_va(m_ip.ip8[3]), x_va(m_port));
				l = 1;
			}
		}
		else if (m_type == NETIP_IPV6)
		{
			// Example: [2001:db8:85a3:8d3:1319:8a2e:370:7348]:443
			if (omit_port)
			{
				ascii::sprintf(str, xcuchars("%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x"), x_va(m_ip.ip16[0]), x_va(m_ip.ip16[1]), x_va(m_ip.ip16[2]),
				               x_va(m_ip.ip16[3]), x_va(m_ip.ip16[4]), x_va(m_ip.ip16[5]), x_va(m_ip.ip16[6]), x_va(m_ip.ip16[7]));
				l = 1;
			}
			else
			{
				ascii::sprintf(str, xcuchars("[%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x]:%u"), x_va(m_ip.ip16[0]), x_va(m_ip.ip16[1]), x_va(m_ip.ip16[2]),
				               x_va(m_ip.ip16[3]), x_va(m_ip.ip16[4]), x_va(m_ip.ip16[5]), x_va(m_ip.ip16[6]), x_va(m_ip.ip16[7]), x_va(m_port));
				l = 1;
			}
		}
		return l;
	}

	void xnetip::serialize_to(xbuffer& dst)
	{
		dst[0] = (m_type >> 8) & 0xFF;
		dst[1] = (m_type >> 0) & 0xFF;

		dst[2] = (m_port >> 8) & 0xFF;
		dst[3] = (m_port >> 0) & 0xFF;

		for (s32 i = 0; i < 16; ++i)
			dst[4 + i] = m_ip.ip8[i];
	}

	void xnetip::deserialize_from(xcbuffer& src)
	{
		m_type = (src[0] << 8) | src[1];
		m_port = (src[2] << 8) | src[3];
		for (s32 i = 0; i < 16; ++i)
			m_ip.ip8[i] = src[4 + i];
	}

}    // namespace ccore