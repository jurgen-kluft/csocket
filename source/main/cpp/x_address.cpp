//
#include "xbase/x_allocator.h"
#include "xbase/x_memory_std.h"
#include "xbase/x_string_ascii.h"
#include "xbase/x_target.h"
#include "xhash/x_skein.h"

#include "xbase/x_buffer.h"
#include "xsocket/x_address.h"

#ifdef PLATFORM_PC
#include <cstdio>
#include <winsock2.h>    // For socket(), connect(), send(), and recv()
#include <ws2tcpip.h>
typedef int  socklen_t;
typedef char raw_type;    // Type used for raw data on this platform
#else
#include <arpa/inet.h>     // For inet_addr()
#include <cstdio>          // For sprintf()
#include <cstdlib>         // For atoi()
#include <cstring>         // For memset()
#include <netdb.h>         // For gethostbyname()
#include <netinet/in.h>    // For sockaddr_in
#include <sys/socket.h>    // For socket(), connect(), send(), and recv()
#include <sys/types.h>     // For data types
#include <unistd.h>        // For close()
typedef void raw_type;    // Type used for raw data on this platform
#endif

#include <errno.h>    // For errno

namespace xcore
{
	typedef xdigest_engine_skein256 xaddr_hasher;

	struct xaddress
	{
		xbytes<32> m_id;    // Connection ID
		xbytes<32> m_ep;    // End-Point information (e.g. 88.128.64.32:5488, IP:Port)
		xaddress*  m_next;
	};

	class xaddress_registry_imp : public xaddress_registry
	{
	protected:
		friend class xaddress_registry;

		x_iallocator* m_allocator;
		u32           m_count;
		u32           m_size;
		u32           m_mask;
		xaddress**    m_hashtable;

	public:
		xaddress_registry_imp(x_iallocator* alloc) : m_allocator(alloc), m_count(0), m_size(0), m_mask(0), m_hashtable(NULL)
		{
		}

		XCORE_CLASS_PLACEMENT_NEW_DELETE

		void init()
		{
			m_count     = 0;
			m_size      = 4096;
			m_mask      = (m_size - 1);
			m_hashtable = (xaddress**)m_allocator->allocate(m_size * sizeof(void*), sizeof(void*));
		}

		virtual bool add(xbytes<32> const& addr_id, xbytes<32> const& addr_ep)
		{
			xaddress* h = find_by_hash(addr_id);
			if (h == nullptr)
			{
				// We don't have this ID registered
				h       = (xaddress*)m_allocator->allocate(sizeof(xaddress), sizeof(void*));
				h->m_id = addr_id;
				h->m_ep = addr_ep;
				add(h);
				return true;
			}
			return false;
		}

		virtual bool get(xbytes<32> const& addr_id, xbytes<32>& addr_ep)
		{
			xaddress* h = find_by_hash(addr_id);
			if (h != nullptr)
			{
				addr_ep = h->m_ep;
			}
			return (h != nullptr);
		}

		virtual bool rem(xbytes<32> const& addr_id)
		{
			xaddress* h = find_by_hash(addr_id);
			if (h != nullptr)
			{
				remove(h);
				destroy(h);
				return true;
			}
			return false;
		}

	protected:
		u32 hash_to_index(xbytes<32> const& hash) const
		{
			u32 i = (u32)(hash[8]) | (u32)(hash[9] << 8) | (u32)(hash[10] << 16) | (u32)(hash[11] << 24);
			i     = i & m_mask;
			return i;
		}

		xaddress* find_by_hash(xbytes<32> const& hash)
		{
			u32 const i = hash_to_index(hash);
			xaddress* e = m_hashtable[i];
			while (e != nullptr)
			{
				if (e->m_id.size() == hash.size() && e->m_id.compare(hash) == 0)
					break;
				e = e->m_next;
			}
			return e;
		}

		void add(xaddress* h)
		{
			u32 const  i = hash_to_index(h->m_id);
			xaddress** p = &m_hashtable[i];
			xaddress*  e = m_hashtable[i];
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

		void remove(xaddress* h)
		{
			u32 const  i = hash_to_index(h->m_id);
			xaddress** p = &m_hashtable[i];
			xaddress*  e = m_hashtable[i];
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

		void destroy(xaddress* h)
		{
			m_allocator->deallocate(h);
		}
	};

	bool xaddress_registry::create(x_iallocator* alloc, u32 max_addresses, xaddress_registry*& addresses)
	{
		xaddress_registry_imp* addr_imp = xnew<xaddress_registry_imp>(xheap(alloc), alloc);
		addr_imp->init();
		addresses = addr_imp;
		return true;
	}

	void xaddress_registry::destroy(xaddress_registry* addr)
	{
		xaddress_registry_imp* imp = (xaddress_registry_imp*)addr;
		for (u32 i = 0; i < imp->m_size; i++)
		{
			xaddress* e = imp->m_hashtable[i];
			while (e != nullptr)
			{
				xaddress* n = e->m_next;
				imp->m_allocator->deallocate(e);
				e = n;
			}
		}
		imp->m_allocator->deallocate(imp->m_hashtable);
		imp->m_allocator->deallocate(imp);
	};

}    // namespace xcore
