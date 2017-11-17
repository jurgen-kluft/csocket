// x_socket.h - Core socket functions 
#ifndef __XSOCKET_SOCKET_UDP_H__
#define __XSOCKET_SOCKET_UDP_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
	class xsock_address;
	class xsock_iaddrin2address;

	// Base class representing basic communication endpoint
	class xsock_udp
	{
	public:
		xsock_udp(xsock_iaddrin2address* );
		~xsock_udp();    // Close and deallocate this socket

		/**
		*   @return true if socket has valid descriptor
		*/
		bool valid() const;

		/**
		*   Construct a UDP socket with the given local port and address
		*   @param localAddress local address
		*   @param localPort local port
		*/
		bool open(xsock_address const * );

		/**
		*   Unset foreign address and port
		*   @return true if disassociation is successful
		*/
		void disconnect();

		/**
		*   Send the given buffer as a UDP datagram to the specified address/port
		*   @param buffer buffer to be written
		*   @param bufferLen number of bytes to write
		*   @param foreignAddress address (IP address or name) to send to
		*   @param foreignPort port number to send to
		*   @return true if send is successful
		*/
		s32 sendTo(const void *buffer, s32 bufferLen, xsock_address const * );

		/**
		*   Read read up to bufferLen bytes data from this socket.
		The given buffer is where the data will be placed.
		*   @param buffer buffer to receive data
		*   @param bufferLen maximum number of bytes to receive
		*   @param foreignAddress address of datagram source
		*   @param foreignPort port of data source
		*   @return number of bytes received and -1 for error
		*/
		s32 recvFrom(void *buffer, s32 bufferLen, xsock_address const*& );

		/**
		*   Set the multicast TTL
		*   @param multicastTTL multicast TTL
		*/
		void setMulticastTTL(u8 multicastTTL);

		/**
		*   Join the specified multicast group
		*   @param multicastGroup multicast group address to join
		*/
		void joinGroup(const char* multicastGroup);

		/**
		*   Leave the specified multicast group
		*   @param multicastGroup multicast group address to leave
		*/
		void leaveGroup(const char* multicastGroup);

	private:
		// Prevent the user from trying to use value semantics on this object
				xsock_udp(const xsock_udp &sock);
		void	operator=(const xsock_udp &sock);

	protected:
		xsock_iaddrin2address*	m_addr2address;
		xsock_address const*	m_socket_address;
		s32     m_socket_descriptor;

				xsock_udp(s32 descriptor);

		bool open_socket(s32 type, s32 protocol);
		void setBroadcast();
	};

}

#endif
