#include "ccore/c_target.h"
#include "cbase/c_allocator.h"
#include "cbase/c_runes.h"
#include "cbase/c_printf.h"
#include "cbase/c_va_list.h"

#include "csocket/c_netip.h"

namespace ncore
{
    s32 netip_t::to_string(runes_t& str, bool omit_port) const
    {
        s32 l = 0;
        if (m_type == NETIP_IPV4)
        {
            // Example: 10.0.0.22:port
            if (omit_port)
            {
                sprintf(str, make_crunes("%u.%u.%u.%u"), va_t(m_ip.ip8[0]), va_t(m_ip.ip8[1]), va_t(m_ip.ip8[2]), va_t(m_ip.ip8[3]));
                l = 1;
            }
            else
            {
                sprintf(str, make_crunes("%u.%u.%u.%u:%u"), va_t(m_ip.ip8[0]), va_t(m_ip.ip8[1]), va_t(m_ip.ip8[2]), va_t(m_ip.ip8[3]), va_t(m_port));
                l = 1;
            }
        }
        else if (m_type == NETIP_IPV6)
        {
            // Example: [2001:db8:85a3:8d3:1319:8a2e:370:7348]:443
            if (omit_port)
            {
                sprintf(str, make_crunes("%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x"), va_t(m_ip.ip16[0]), va_t(m_ip.ip16[1]), va_t(m_ip.ip16[2]), va_t(m_ip.ip16[3]), va_t(m_ip.ip16[4]), va_t(m_ip.ip16[5]), va_t(m_ip.ip16[6]), va_t(m_ip.ip16[7]));
                l = 1;
            }
            else
            {
                sprintf(str, make_crunes("[%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x]:%u"), va_t(m_ip.ip16[0]), va_t(m_ip.ip16[1]), va_t(m_ip.ip16[2]), va_t(m_ip.ip16[3]), va_t(m_ip.ip16[4]), va_t(m_ip.ip16[5]), va_t(m_ip.ip16[6]), va_t(m_ip.ip16[7]),
                        va_t(m_port));
                l = 1;
            }
        }
        return l;
    }

    void netip_t::serialize_to(buffer_t& dst)
    {
        binary_writer_t writer = dst.writer();

        writer.write((u8)((m_type >> 8) & 0xFF));
        writer.write((u8)((m_type >> 0) & 0xFF));
        writer.write((u8)((m_port >> 8) & 0xFF));
        writer.write((u8)((m_port >> 0) & 0xFF));

        cbuffer_t ip8(m_ip.ip8, m_ip.ip8 + sizeof(m_ip.ip8));
        writer.write_data(ip8);

        dst = writer.get_current_buffer();
    }

    void netip_t::deserialize_from(cbuffer_t& src)
    {
        binary_reader_t reader = src.reader();
        m_type = reader.read_u16();
        m_port = reader.read_u16();

        buffer_t ip8(m_ip.ip8, m_ip.ip8, m_ip.ip8 + sizeof(m_ip.ip8));
        reader.read_data(ip8);
    }

}  // namespace ncore
