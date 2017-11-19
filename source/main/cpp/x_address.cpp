#include "xbase/x_target.h"
#include "xbase/x_allocator.h"
#include "xbase/x_string_ascii.h"
#include "xbase/x_memory_std.h"
#include "xhash/x_skein.h"

#include "xsocket/x_address.h"

#ifdef PLATFORM_PC
	#include <winsock2.h>         // For socket(), connect(), send(), and recv()
	#include <ws2tcpip.h>
	#include <cstdio>
	typedef int socklen_t;
	typedef char raw_type;       // Type used for raw data on this platform
#else
	#include <sys/types.h>       // For data types
	#include <sys/socket.h>      // For socket(), connect(), send(), and recv()
	#include <netdb.h>           // For gethostbyname()
	#include <arpa/inet.h>       // For inet_addr()
	#include <unistd.h>          // For close()
	#include <netinet/in.h>      // For sockaddr_in
	#include <cstring>	         // For memset()
	#include <cstdio>	         // For sprintf()
	#include <cstdlib>	         // For atoi()
	typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <errno.h>             // For errno

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PRIVATE] API
	class xsock_haddress : public xsock_address
	{
	public:
		virtual bool				construct(char* address, u16 port, u8 family, u8 socktype, u8 protocol);
		virtual s32					to_string(char* str, u32 maxstrlen) const;
		virtual xsock_addrin const&	get_addrin() const;

		xsock_haddress*				m_next;
		u32							m_index;
		xsock_addrin				m_addrin;
		xsock_hash					m_hash;
	};

	bool	xsock_haddress::construct(char* address, u16 port, u8 family, u8 socktype, u8 protocol)
	{
		s32 result = m_addrin.construct(address, port, family, socktype, protocol);
		return result == 0;
	}

	s32		xsock_haddress::to_string(char* str, u32 maxstrlen) const
	{
		// @TODO
		return 0;
	}

	xsock_addrin const&	xsock_haddress::get_addrin() const
	{
		return m_addrin;
	}


	class xsock_address_factory_imp : public xsock_iaddress_factory
	{
		xdigest_engine_skein256		m_hasher;

	public:
		void					init()
		{
			m_count = 0;
			m_size = 4096;
			m_mask = (m_size - 1);
		}

		virtual xsock_hash		compute_hash(void const* data, u32 len)
		{
			xsock_hash hash;
			m_hasher.reset();
			m_hasher.update(data, len);
			m_hasher.digest((xbyte*)hash.m_hash);
			return hash;
		}

		virtual xsock_address*	create(xsock_addrin const& a)
		{
			xsock_hash hash = compute_hash((const u8*)&a, sizeof(xsock_addrin));
			xsock_haddress* h = find_by_hash(hash);
			if (h == nullptr)
			{
				h = (xsock_haddress*)m_allocator->allocate(sizeof(xsock_haddress), sizeof(void*));
				h->m_hash = hash;
				h->m_addrin.m_len = a.m_len;
				x_memcpy(h->m_addrin.m_data, a.m_data, sizeof(a.m_data));
				add(h);
			}
			return h;
		}

		virtual void			destroy(xsock_address* addr)
		{
			m_allocator->deallocate(addr);
		}

		virtual bool			get_assoc(void* addrin, u32 addrinlen, xsock_address*& assoc)
		{
			xsock_hash hash = compute_hash(addrin, addrinlen);
			assoc = find_by_hash(hash);
			return assoc != nullptr;
		}

		virtual	void			set_assoc(void* addrin, u32 addrinlen, xsock_address* addr)
		{
			xsock_hash hash = compute_hash(addrin, addrinlen);
			xsock_haddress* h = find_by_hash(hash);
			if (h == nullptr)
			{
				add(h);
			}
		}

		virtual	void			del_assoc(void* addrin, u32 addrinlen)
		{
			xsock_hash hash = compute_hash(addrin, addrinlen);
			xsock_haddress* h = find_by_hash(hash);
			if (h != nullptr)
			{
				remove(h);
				destroy(h);
			}
		}

		virtual bool			get_assoc(xsock_address* adr, u32& assoc)
		{
			xsock_haddress* hadr = (xsock_haddress*)adr;
			assoc = hadr->m_index;
			return true;
		}

		virtual	void			set_assoc(xsock_address* addr, u32 idx)
		{
			xsock_haddress* haddr = (xsock_haddress*)addr;
			haddr->m_index = idx;
		}

		virtual void			del_assoc(xsock_address* adr)
		{

		}

	protected:
		u32					hash_to_index(xsock_hash const & hash)
		{
			u32 i = (u32)(hash.m_hash[8]) | (u32)(hash.m_hash[9] << 8) | (u32)(hash.m_hash[10] << 16) | (u32)(hash.m_hash[11] << 24);
			i = i & m_mask;
			return i;
		}

		xsock_haddress*		find_by_hash(xsock_hash const & hash)
		{
			u32 const i = hash_to_index(hash);
			xsock_haddress* e = m_hashtable[i];
			while (e != nullptr)
			{
				if (e->m_hash.m_len == hash.m_len && memcmp(e->m_hash.m_hash, hash.m_hash, sizeof(hash)) == 0)
					break;
				e = e->m_next;
			}
			return e;
		}

		void				add(xsock_haddress* h)
		{
			u32 const i = hash_to_index(h->m_hash);
			xsock_haddress** p = &m_hashtable[i];
			xsock_haddress* e = m_hashtable[i];
			while (e != nullptr)
			{
				s32 r = memcmp(e->m_hash.m_hash, h->m_hash.m_hash, sizeof(xsock_haddress::m_hash));
				if (r == -1)
				{
					break;
				}
				p = &e->m_next;
				e = e->m_next;
			}
			*p = h;
			h->m_next = e;
		}

		void				remove(xsock_haddress* h)
		{
			u32 const i = hash_to_index(h->m_hash);
			xsock_haddress** p = &m_hashtable[i];
			xsock_haddress* e = m_hashtable[i];
			while (e != nullptr)
			{
				s32 r = memcmp(e->m_hash.m_hash, h->m_hash.m_hash, sizeof(xsock_haddress::m_hash));
				if (r == 0)
				{
					*p = e->m_next;
					e = e->m_next;
					break;
				}
				p = &e->m_next;
				e = e->m_next;
			}
			h->m_next = nullptr;
		}

		x_iallocator*		m_allocator;

		u32					m_count;
		u32					m_size;
		u32					m_mask;
		xsock_haddress**		m_hashtable;
	};

#if defined(PLATFORM_PC) || defined(PLATFORM_OSX)

	bool		xsock_addrin::resolve(char const* address, u16 port, u8 family, u8 socktype, u8 protocol)
	{
		struct addrinfo hints;
		struct addrinfo *result = NULL;
		struct addrinfo *ptr = NULL;

		//--------------------------------
		// Setup the hints address info structure which is passed to the getaddrinfo() function
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = family;
		hints.ai_socktype = socktype;
		hints.ai_protocol = protocol;

		//--------------------------------
		m_len = 0;
		ZeroMemory(m_data, sizeof(m_data));

		//--------------------------------
		// Call getaddrinfo(). If the call succeeds, the result variable will hold a linked list
		// of addrinfo structures containing response information
		DWORD dwRetval = getaddrinfo(address, NULL, &hints, &result);
		if (dwRetval != 0)
		{
			//printf("getaddrinfo failed with error: %d\n", dwRetval);
			return false;
		}

		// For each address find and pick the one we need
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
		{
			if (ptr->ai_socktype == socktype && ptr->ai_protocol == protocol)
			{
				m_len = ptr->ai_addrlen;
				u8 const* ai_addr = (u8 const*)ptr->ai_addr;
				for (s32 i = 0; i < m_len; i++)
				{
					m_data[i] = ai_addr[i];
				}
				m_data[m_len + 0] = ptr->ai_family;
				m_data[m_len + 1] = ptr->ai_socktype;
				m_data[m_len + 2] = ptr->ai_protocol;
				break;
			}
		}

		freeaddrinfo(result);
		return true;
	}

	bool	xsock_addrin::construct_udp(char const* address, u16 port)
	{
		return construct(address, port, (u8)AF_UNSPEC, (u8)SOCK_DGRAM, (u8)IPPROTO_UDP);
	}

	bool	xsock_addrin::construct_tcp(char const* address, u16 port)
	{
		return construct(address, port, (u8)AF_UNSPEC, (u8)SOCK_STREAM, (u8)IPPROTO_TCP);
	}

	s32		xsock_addrin::to_string(char* str, u32 maxstrlen) const
	{
		struct sockaddr_in  *sockaddr_ipv4;
		LPSOCKADDR sockaddr_ip;

		char ipstringbuffer[46];
		DWORD ipbufferlength = 46;

		s32 ret;

		u8 const family = m_data[m_len + 0];
		switch (family)
		{
		case AF_UNSPEC:
			sprintf(str, "null://");
			break;
		case AF_INET:
			sockaddr_ipv4 = (struct sockaddr_in *)m_data;
			sprintf(str, "IPv4://%s", inet_ntoa(sockaddr_ipv4->sin_addr));
			break;
		case AF_INET6:
			sockaddr_ip = (LPSOCKADDR)m_data;
			// The buffer length is changed by each call to WSAAddresstoString
			// So we need to set it for each iteration through the loop for safety
			ret = WSAAddressToString(sockaddr_ip, m_len, NULL, ipstringbuffer, &ipbufferlength);
			if (ret)
				sprintf(str, "IPv6://error(%u)", WSAGetLastError());
			else
				sprintf(str, "IPv6://%s", ipstringbuffer);
			break;
		case AF_NETBIOS:
			sprintf(str, "netbios://");
			break;
		default:
			sprintf(str, "null://%ld", family);
			break;
		}

		s32 strlength = strlen(str);
		char* family_str = str + strlength;

		u8 const socktype = m_data[m_len + 1];
		switch (socktype)
		{
		case 0:
			sprintf(family_str, ":?");
			break;
		case SOCK_STREAM:
			sprintf(family_str, ":stream");
			break;
		case SOCK_DGRAM:
			sprintf(family_str, ":datagram");
			break;
		case SOCK_RAW:
			sprintf(family_str, ":raw");
			break;
		case SOCK_RDM:
			sprintf(family_str, ":rdm");
			break;
		case SOCK_SEQPACKET:
			sprintf(family_str, ":seq");
			break;
		default:
			sprintf(family_str, ":%ld", socktype);
			break;
		}

		s32 strlength = strlen(str);
		char* protocol_str = str + strlength;

		u8 const protocol = m_data[m_len + 2];
		switch (protocol)
		{
		case 0:
			sprintf(protocol_str, ":?");
			break;
		case IPPROTO_TCP:
			sprintf(protocol_str, ":TCP");
			break;
		case IPPROTO_UDP:
			sprintf(protocol_str, ":UDP");
			break;
		default:
			sprintf(protocol_str, ":%ld", protocol);
			break;
		}

		return true;
	}


#else
	s32		xsock_addrin::from_string(const char* addr)
	{
		return -1;
	}

	s32		xsock_addrin::to_string(char* str, u32 maxstrlen) const
	{
		sprintf(str, "null://");
		return -1;
	}

#endif

}
