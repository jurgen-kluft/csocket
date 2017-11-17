// x_socket.h - Core TCP socket functions 
#ifndef __XSOCKET_SOCKET_TCP_H__
#define __XSOCKET_SOCKET_TCP_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
	class x_iallocator;
	class xsock_address;
	class xsock_addresses;
	class xsock_tcp;

	class xsock_tcp_factory
	{
	public:
		virtual void	create_tcp(xsock_tcp*& ) = 0;
		virtual void	destroy_tcp(xsock_tcp* ) = 0;
	};

	// Base class representing basic communication endpoint
	class xsock_tcp
	{
	public:
		xsock_tcp();

		/**
		*   @return true if socket has valid descriptor
		*/
		bool valid() const;

		/**
		*   Construct a UDP socket with the given local port and address
		*   @param localAddress local address
		*   @param localPort local port
		*/
		void connect(xsock_address const * );

		/**
		*   Unset foreign address and port
		*   @return true if disassociation is successful
		*/
		void disconnect();

		/**
		*   Send the given buffer 
		*   @param buffer buffer to be written
		*   @param bufferLen number of bytes to write
		*   @return true if send is successful
		*/
		void send(const void *buffer, s32 bufferLen);

		/**
		*   Read read up to bufferLen bytes data from this socket.
		The given buffer is where the data will be placed.
		*   @param buffer buffer to receive data
		*   @param bufferLen maximum number of bytes to receive
		*   @return number of bytes received and -1 for error
		*/
		s32 recv(void *buffer, s32 bufferLen);

		/**
		*   Return the remote address
		*   @param multicastTTL multicast TTL
		*/
		bool getRemoteAddress(xsock_address const*&);


	private:
		// Prevent the user from trying to use value semantics on this object
				xsock_tcp(const xsock_tcp &sock);
				~xsock_tcp();

		void	operator=(const xsock_tcp &sock);

	protected:
		s32				m_socket_descriptor;

				xsock_tcp(s32 descriptor);

		bool open_socket(s32 type, s32 protocol);
	};

	// TCP 'listening' socket class for incoming connections
	class xsock_tcp_listen
	{
	public:
		/**
		 *   Construct a TCP socket for use with a server, accepting connections
		 *   on the specified port on the interface specified by the given address
		 *   @param localAddress local interface (address) of server socket
		 *   @param localPort local port of server socket
		 *   @param queueLen maximum queue length for outstanding
		 *                   connection requests (default 5)
		 *   @exception SocketException thrown if unable to create TCP server socket
		 */
		xsock_tcp_listen(xsock_addresses* addresses, xsock_tcp_factory* socket_factory);

		/**
		 *   Create the listening socket 
		 *   @return true if socket has been created succesfully
		 */
		bool	start(xsock_address const *, u32 queueLen = 5);

		/**
		 *   Blocks until a new connection is established on this socket or error
		 *   @return new connection socket
		 */
		xsock_tcp *	accept();

	private:
		u32					m_queue_len;
		s32					m_socket_descriptor;
		x_iallocator*		m_allocator;
		xsock_addresses*	m_addresses;
		xsock_tcp_factory*	m_socket_tcp_factory;
	};

}

#endif
