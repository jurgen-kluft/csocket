// x_message-tcp.cpp - Core socket functions 
#include "xbase/x_target.h"
#include "xbase/x_debug.h"
#include "xsocket/x_socket.h"
#include "xsocket/x_address.h"
#include "xsocket/x_netip.h"
#include "xsocket/private/x_message-tcp.h"

#ifdef PLATFORM_PC
    #include <winsock2.h>         // For socket(), connect(), send(), and recv()
    #include <ws2tcpip.h>
    #include <stdio.h>
    typedef int socklen_t;
    typedef char raw_type;       // Type used for raw data on this platform
	#pragma comment(lib, "winmm.lib")
	#pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/types.h>       // For data types
    #include <sys/socket.h>      // For socket(), connect(), send(), and recv()
    #include <netdb.h>           // For gethostbyname()
    #include <arpa/inet.h>       // For inet_addr()
    #include <unistd.h>          // For close()
    #include <netinet/in.h>      // For sockaddr_in
    #include <cstring>	       // For memset()
    #include <stdio.h>
    #include <cstdlib>	       // For atoi()
    typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <errno.h>             // For errno

namespace xcore
{
	s32 is_error(s32 n)
	{
		return n == 0 || (n < 0 && errno != EINTR && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK
#ifdef _WIN32
			&& WSAGetLastError() != WSAEINTR && WSAGetLastError() != WSAEWOULDBLOCK
#endif
			);
	}


	s32				message_socket_writer::write(xmessage*& msg)
	{
		msg = NULL;
			
		if (m_current_msg == NULL)
		{
			m_current_msg = m_send_queue->peek();
			if (m_current_msg != NULL)
			{
				hdr_to_payload(m_current_msg, m_data, m_bytes_to_write);
				m_bytes_written = 0;
			}
		}

		if (m_current_msg != NULL)
		{
			s32 n;
			s32 bytes_written = m_bytes_written;
			while ((n = ::send(m_socket, (const char*)(m_data + bytes_written), m_bytes_to_write - bytes_written, 0)) > 0)
			{
				bytes_written += n;
				if (bytes_written == m_bytes_to_write)
					break;
			}
			m_bytes_written = bytes_written;

			if (is_error(n))
				return -1;

			if (bytes_written == m_bytes_to_write)
			{
				m_current_msg = m_send_queue->pop();
				msg = hdr_to_msg(m_current_msg);
				return m_send_queue->empty() ? 0 : 1;
			}
			else
			{
				return 1;
			}
		}
		else
		{
			return 0;
		}
	}

	s32				message_socket_reader::read(xmessage*& msg, xmessage*& rcvd)
	{
		rcvd = NULL;

		if (m_msg == NULL)
		{
			m_msg = msg;
			m_hdr = msg_to_hdr(msg);
			hdr_to_payload(m_hdr, m_data, m_data_size);
			m_bytes_read = 0;
			m_bytes_to_read = 4;
			m_state = STATE_READ_SIZE;
		}

		s32 n;
		s32 bytes_read = m_bytes_read;
		while ((n = ::recv(m_socket, (char*)(m_data + bytes_read), m_bytes_to_read - bytes_read, 0)) > 0)
		{
			bytes_read += n;
			if (bytes_read == m_bytes_to_read)
				break;
		}

		if (is_error(n))
			return -1;

		m_bytes_read = bytes_read;

		if (m_state == STATE_READ_SIZE && m_bytes_read == m_bytes_to_read)
		{
			m_state = STATE_READ_BODY;
			m_bytes_to_read = ((u32 const*)m_data)[0];
			m_bytes_read = 0;
		}

		if (m_bytes_read == m_bytes_to_read)
		{
			rcvd = msg;
			msg = NULL;
		}

		return n == 0 ? 0 : 1;
	}


	/// @DESIGN NOTE:
	///  The p2p message here has direct knowledge of ns_message and actually the
	///  structures at this layer are just wrappers around ns_message.

	/// ---------------------------------------------------------------------------------------
	/// Unaligned Data Reading/Writing
	/// ---------------------------------------------------------------------------------------
	inline u8			read_u8(xbyte const* ptr) { return (u8)*ptr; }
	inline s8			read_s8(xbyte const* ptr) { return (s8)*ptr; }
	inline u16			read_u16(xbyte const* ptr) { u16 b0 = *ptr++; u16 b1 = *ptr++; return (u16)((b1 << 8) | b0); }
	inline s16			read_s16(xbyte const* ptr) { u16 b0 = *ptr++; u16 b1 = *ptr++; return (s16)((b1 << 8) | b0); }
	inline u32			read_u32(xbyte const* ptr) { u32 b0 = *ptr++; u32 b1 = *ptr++; u32 b2 = *ptr++; u32 b3 = *ptr++; return (u32)((b3 << 24) | (b2 << 16) | (b1 << 8) | (b0)); }
	inline s32			read_s32(xbyte const* ptr) { u32 b0 = *ptr++; u32 b1 = *ptr++; u32 b2 = *ptr++; u32 b3 = *ptr++; return (s32)((b3 << 24) | (b2 << 16) | (b1 << 8) | (b0)); }
	inline f32			read_f32(xbyte const* ptr) { u32 b0 = *ptr++; u32 b1 = *ptr++; u32 b2 = *ptr++; u32 b3 = *ptr++; u32 u = ((b3 << 24) | (b2 << 16) | (b1 << 8) | (b0)); return *((f32*)&u); }
	inline u64			read_u64(xbyte const* ptr) { u64 l0 = read_u32(ptr); u64 l1 = read_u32(ptr + 4); return (u64)((l1 << 32) | l0); }
	inline s64			read_s64(xbyte const* ptr) { u64 l0 = read_u32(ptr); u64 l1 = read_u32(ptr + 4); return (s64)((l1 << 32) | l0); }
	inline f64			read_f64(xbyte const* ptr) { u64 l0 = read_u32(ptr); u64 l1 = read_u32(ptr + 4); u64 ll = (u64)((l1 << 32) | l0); return *((f64*)ll); }

	inline void			write_u8(xbyte * ptr, u8  b) { *ptr = b; }
	inline void			write_s8(xbyte * ptr, s8  b) { *ptr = b; }
	inline void			write_u16(xbyte * ptr, u16 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_s16(xbyte * ptr, s16 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_u32(xbyte * ptr, u32 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_s32(xbyte * ptr, s32 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_f32(xbyte * ptr, f32 f) { u32 b = *((u32*)&f); for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_u64(xbyte * ptr, u64 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_s64(xbyte * ptr, s64 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_f64(xbyte * ptr, f64 f) { u64 b = *((u64*)&f); for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }


	/// ---------------------------------------------------------------------------------------
	/// Message Reader
	/// ---------------------------------------------------------------------------------------
	inline bool			_can_read(xmessage* _msg, u32 _cursor, u32 _num_bytes)
	{
		if (_msg == NULL) return false;
		else return (_cursor + _num_bytes) <= _msg->m_size;
	}

	void				xmessage_reader::set_cursor(u32 c)
	{
		cursor_ = c;
		if (cursor_ >= msg_->m_size)
			cursor_ = msg_->m_size;
	}
	u32					xmessage_reader::get_cursor() const
	{
		return cursor_;
	}

	u32					xmessage_reader::get_size() const
	{
		if (msg_ == NULL) return 0;
		return msg_->m_size - cursor_;
	}

	bool				xmessage_reader::can_read(u32 number_of_bytes) const
	{
		return _can_read(msg_, cursor_, number_of_bytes);
	}


	u32					xmessage_reader::read(bool& b)
	{
		if (_can_read(msg_, cursor_, 1))
		{
			u8 const* ptr = (u8 const*)msg_->m_data + cursor_;
			cursor_ += 1;
			b = read_u8(ptr) != 0;
			return 1;
		}
		return 0;
	}

	u32					xmessage_reader::read(u8  & b)
	{
		if (_can_read(msg_, cursor_, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)msg_->m_data + cursor_;
			cursor_ += sizeof(b);
			b = read_u8(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_reader::read(s8  & b)
	{
		if (_can_read(msg_, cursor_, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)msg_->m_data + cursor_;
			cursor_ += sizeof(b);
			b = read_s8(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_reader::read(u16 & b)
	{
		if (_can_read(msg_, cursor_, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)msg_->m_data + cursor_;
			cursor_ += sizeof(b);
			b = read_u16(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_reader::read(s16 & b)
	{
		if (_can_read(msg_, cursor_, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)msg_->m_data + cursor_;
			cursor_ += sizeof(b);
			b = read_s16(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_reader::read(u32 & b)
	{
		if (_can_read(msg_, cursor_, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)msg_->m_data + cursor_;
			cursor_ += sizeof(b);
			b = read_u32(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_reader::read(s32 & b)
	{
		if (_can_read(msg_, cursor_, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)msg_->m_data + cursor_;
			cursor_ += sizeof(b);
			b = read_s32(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_reader::read(u64 & b)
	{
		if (_can_read(msg_, cursor_, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)msg_->m_data + cursor_;
			cursor_ += sizeof(b);
			b = read_u64(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_reader::read(s64 & b)
	{
		if (_can_read(msg_, cursor_, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)msg_->m_data + cursor_;
			cursor_ += sizeof(b);
			b = read_s64(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_reader::read(f32 & b)
	{
		if (_can_read(msg_, cursor_, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)msg_->m_data + cursor_;
			cursor_ += sizeof(b);
			b = read_f32(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_reader::read(f64 & b)
	{
		if (_can_read(msg_, cursor_, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)msg_->m_data + cursor_;
			cursor_ += sizeof(b);
			b = read_f64(ptr);
			return sizeof(b);
		}
		return 0;
	}

	bool				xmessage_reader::read_data(xbyte* data, u32 size)
	{
		if (!_can_read(msg_, cursor_, size))
		{
			return false;
		}
		xbyte const* src = (xbyte const*)msg_->m_data + cursor_;
		memcpy(data, src, size);
		cursor_ += size;
		return true;
	}

	bool				xmessage_reader::view_data(xbyte const*& data, u32 size)
	{
		if (!_can_read(msg_, cursor_, size))
		{
			data = NULL;
			return false;
		}
		data = (xbyte const*)msg_->m_data + cursor_;
		cursor_ += size;
		return true;
	}

	bool				xmessage_reader::view_string(char const*& str, u32& strlen)
	{
		if (msg_ == NULL)
			return false;

		strlen = 0;
		str = (char const*)msg_->m_data + cursor_;
		while (cursor_ < msg_->m_size)
		{
			if (str[strlen] == '\0')
				break;
			++strlen;
		}

		if (str[strlen] != '\0')
		{
			str = NULL;
			strlen = 0;
			return false;
		}

		cursor_ += strlen;
		return true;
	}

	/// ---------------------------------------------------------------------------------------
	/// Message Writer
	/// ---------------------------------------------------------------------------------------
	inline bool			_can_write(xmessage* _msg, u32 _cursor, u32 _num_bytes)
	{
		if (_msg == NULL) return false;
		else return (_cursor + _num_bytes) <= _msg->m_size;
	}

	void				xmessage_writer::set_cursor(u32 c)
	{
		cursor_ = c;
		if (cursor_ > msg_->m_size)
			cursor_ = msg_->m_size;
	}

	u32					xmessage_writer::get_cursor() const
	{
		return cursor_;
	}


	bool				xmessage_writer::can_write(u32 num_bytes) const
	{
		return _can_write(msg_, cursor_, num_bytes);
	}


	u32					xmessage_writer::write(bool b)
	{
		if (_can_write(msg_, cursor_, 1))
		{
			write_u8((xbyte*)msg_->m_data + cursor_, b ? 1 : 0);
			cursor_ += 1;
			return 1;
		}
		return 0;
	}

	u32					xmessage_writer::write(u8   b)
	{
		if (_can_write(msg_, cursor_, sizeof(b)))
		{
			write_u8((xbyte*)msg_->m_data + cursor_, b);
			cursor_ += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_writer::write(s8   b)
	{
		if (_can_write(msg_, cursor_, sizeof(b)))
		{
			write_u8((xbyte*)msg_->m_data + cursor_, b);
			cursor_ += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_writer::write(u16  b)
	{
		if (_can_write(msg_, cursor_, sizeof(b)))
		{
			write_u16((xbyte*)msg_->m_data + cursor_, b);
			cursor_ += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_writer::write(s16  b)
	{
		if (_can_write(msg_, cursor_, sizeof(b)))
		{
			write_s16((xbyte*)msg_->m_data + cursor_, b);
			cursor_ += sizeof(b);
		}
		return sizeof(b);
	}

	u32					xmessage_writer::write(u32  b)
	{
		if (_can_write(msg_, cursor_, sizeof(b)))
		{
			write_u32((xbyte*)msg_->m_data + cursor_, b);
			cursor_ += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_writer::write(s32  b)
	{
		if (_can_write(msg_, cursor_, sizeof(b)))
		{
			write_s32((xbyte*)msg_->m_data + cursor_, b);
			cursor_ += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_writer::write(u64  b)
	{
		if (_can_write(msg_, cursor_, sizeof(b)))
		{
			write_u64((xbyte*)msg_->m_data + cursor_, b);
			cursor_ += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_writer::write(s64  b)
	{
		if (_can_write(msg_, cursor_, sizeof(b)))
		{
			write_s64((xbyte*)msg_->m_data + cursor_, b);
			cursor_ += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_writer::write(f32  b)
	{
		if (_can_write(msg_, cursor_, sizeof(b)))
		{
			write_f32((xbyte*)msg_->m_data + cursor_, b);
			cursor_ += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					xmessage_writer::write(f64  b)
	{
		if (_can_write(msg_, cursor_, sizeof(b)))
		{
			write_f64((xbyte*)msg_->m_data + cursor_, b);
			cursor_ += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}


	u32					xmessage_writer::write_data(const xbyte* _data, u32 _size)
	{
		if (_can_write(msg_, cursor_, _size))
		{
			xbyte* dst = (xbyte*)msg_->m_data + cursor_;
			for (u32 i = 0; i<_size; i++)
				*dst++ = *_data++;
			cursor_ += _size;
			return _size;
		}
		return 0;
	}

	u32					xmessage_writer::write_string(const char* _str, u32 _len)
	{
		if (_can_write(msg_, cursor_, _len))
		{
			char* dst = (char*)msg_->m_data + cursor_;
			for (u32 i = 0; i<_len; i++)
				*dst++ = *_str++;
			cursor_ += _len;
			return _len;
		}
		return 0;
	}


}