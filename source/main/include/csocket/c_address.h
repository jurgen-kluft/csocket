#ifndef __CSOCKET_ADDRESS_H__
#define __CSOCKET_ADDRESS_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cbase/c_buffer.h"

namespace ncore
{
    struct address_t;
    typedef data_t<32> address_id_t;

    class address_registry_t
    {
    public:
        static bool create(alloc_t* alloc, u32 max_addresses, address_registry_t*& addr);
        static void destroy(address_registry_t* addr);

        virtual bool add(address_id_t const& addr_id, address_id_t const& addr_ep) = 0;
        virtual bool get(address_id_t const& addr_id, address_id_t& addr_ep)       = 0;
        virtual bool rem(address_id_t const& addr_id)                              = 0;
    };
}  // namespace ncore

#endif  // __CSOCKET_ADDRESS_H__
