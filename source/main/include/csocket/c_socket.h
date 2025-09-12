#ifndef __CSOCKET_SOCKET_H__
#define __CSOCKET_SOCKET_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cbase/c_buffer.h"
#include "cbase/c_runes.h"

namespace ncore
{
    class alloc_t;

    struct address_t;
    struct addresses_t;
    struct message_t;

    typedef data_t<32> sockid_t;

    class socket_t
    {
    protected:
        static void s_attach();
        static void s_release();

    public:
        virtual ~socket_t() {}

        virtual void open(u16 port, crunes_t const& name, sockid_t const& id, u32 max_open) = 0;
        virtual void close()                                                                = 0;

        virtual void process(addresses_t& open_conns, addresses_t& closed_conns, addresses_t& new_conns, addresses_t& failed_conns, addresses_t& pex_conns) = 0;

        virtual void connect(address_t*)    = 0;
        virtual void disconnect(address_t*) = 0;

        virtual bool alloc_msg(message_t*& msg) = 0;
        virtual void commit_msg(message_t* msg) = 0;
        virtual void free_msg(message_t* msg)   = 0;

        virtual bool send_msg(message_t* msg, address_t* to)     = 0;
        virtual bool recv_msg(message_t*& msg, address_t*& from) = 0;
    };

    socket_t* gCreateTcpBasedSocket(alloc_t*);
    void      gDestroyTcpBasedSocket(socket_t*);
}  // namespace ncore

#endif
