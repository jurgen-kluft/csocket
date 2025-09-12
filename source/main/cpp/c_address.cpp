#include "ccore/c_target.h"
#include "cbase/c_allocator.h"
#include "cbase/c_memory.h"
#include "cbase/c_runes.h"
#include "chash/c_hash.h"

#include "cbase/c_buffer.h"
#include "csocket/c_address.h"

#ifdef PLATFORM_PC
#    include <cstdio>
#    include <winsock2.h>  // For socket(), connect(), send(), and recv()
#    include <ws2tcpip.h>
typedef int  socklen_t;
typedef char raw_type;  // Type used for raw data on this platform
#else
#    include <arpa/inet.h>   // For inet_addr()
#    include <cstdio>        // For sprintf()
#    include <cstdlib>       // For atoi()
#    include <cstring>       // For memset()
#    include <netdb.h>       // For gethostbyname()
#    include <netinet/in.h>  // For sockaddr_in
#    include <sys/socket.h>  // For socket(), connect(), send(), and recv()
#    include <sys/types.h>   // For data types
#    include <unistd.h>      // For close()
typedef void raw_type;  // Type used for raw data on this platform
#endif

#include <errno.h>  // For errno

namespace ncore
{
    typedef nhash::skein256 addr_hasher_t;

    struct address_t
    {
        data_t<32> m_id;  // Connection ID
        data_t<32> m_ep;  // End-Point information (e.g. 88.128.64.32:5488, IP:Port)
        address_t *m_next;
    };

    class address_registry_imp_t : public address_registry_t
    {
    protected:
        friend class address_registry_t;

        alloc_t    *m_allocator;
        u32         m_count;
        u32         m_size;
        u32         m_mask;
        address_t **m_hashtable;

    public:
        address_registry_imp_t()
            : m_allocator(nullptr)
            , m_count(0)
            , m_size(0)
            , m_mask(0)
            , m_hashtable(nullptr)
        {
        }

        DCORE_CLASS_PLACEMENT_NEW_DELETE

        void init(alloc_t *allocator)
        {
            m_allocator = allocator;
            m_count     = 0;
            m_size      = 4096;
            m_mask      = (m_size - 1);
            m_hashtable = (address_t **)m_allocator->allocate(m_size * sizeof(void *), sizeof(void *));
        }

        virtual bool add(address_id_t const &addr_id, address_id_t const &addr_ep)
        {
            address_t *h = find_by_hash(addr_id);
            if (h == nullptr)
            {
                // We don't have this ID registered
                h       = (address_t *)m_allocator->allocate(sizeof(address_t), sizeof(void *));
                h->m_id = addr_id;
                h->m_ep = addr_ep;
                add(h);
                return true;
            }
            return false;
        }

        virtual bool get(address_id_t const &addr_id, address_id_t &addr_ep)
        {
            address_t *h = find_by_hash(addr_id);
            if (h != nullptr)
            {
                addr_ep = h->m_ep;
            }
            return (h != nullptr);
        }

        virtual bool rem(address_id_t const &addr_id)
        {
            address_t *h = find_by_hash(addr_id);
            if (h != nullptr)
            {
                remove(h);
                destroy(h);
                return true;
            }
            return false;
        }

    protected:
        u32 hash_to_index(address_id_t const &hash) const
        {
            u32 i = (u32)(hash[8]) | (u32)(hash[9] << 8) | (u32)(hash[10] << 16) | (u32)(hash[11] << 24);
            i     = i & m_mask;
            return i;
        }

        address_t *find_by_hash(address_id_t const &hash)
        {
            u32 const  i = hash_to_index(hash);
            address_t *e = m_hashtable[i];
            while (e != nullptr)
            {
                if (e->m_id.size() == hash.size() && e->m_id.compare(hash) == 0)
                    break;
                e = e->m_next;
            }
            return e;
        }

        void add(address_t *h)
        {
            u32 const   i = hash_to_index(h->m_id);
            address_t **p = &m_hashtable[i];
            address_t  *e = m_hashtable[i];
            while (e != nullptr)
            {
                s32 r = e->m_id.compare(h->m_id);
                if (r == -1)
                {
                    break;
                }
                p = &e->m_next;
                e = e->m_next;
            }
            *p        = h;
            h->m_next = e;
        }

        void remove(address_t *h)
        {
            u32 const   i = hash_to_index(h->m_id);
            address_t **p = &m_hashtable[i];
            address_t  *e = m_hashtable[i];
            while (e != nullptr)
            {
                s32 r = e->m_id.compare(h->m_id);
                if (r == 0)
                {
                    *p = e->m_next;
                    e  = e->m_next;
                    break;
                }
                p = &e->m_next;
                e = e->m_next;
            }
            h->m_next = nullptr;
        }

        void destroy(address_t *h) { m_allocator->deallocate(h); }
    };

    bool address_registry_t::create(alloc_t *alloc, u32 max_addresses, address_registry_t *&addresses)
    {
        address_registry_imp_t *addr_imp = g_allocate<address_registry_imp_t>(alloc);
        addr_imp->init(alloc);
        addresses = addr_imp;
        return true;
    }

    void address_registry_t::destroy(address_registry_t *addr)
    {
        address_registry_imp_t *imp = (address_registry_imp_t *)addr;
        for (u32 i = 0; i < imp->m_size; i++)
        {
            address_t *e = imp->m_hashtable[i];
            while (e != nullptr)
            {
                address_t *n = e->m_next;
                imp->m_allocator->deallocate(e);
                e = n;
            }
        }
        imp->m_allocator->deallocate(imp->m_hashtable);
        imp->m_allocator->deallocate(imp);
    };

}  // namespace ncore
