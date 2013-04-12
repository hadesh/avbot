
#pragma once

#ifndef AV_ACCEPTOR_SERVER_H
#define AV_ACCEPTOR_SERVER_H

#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include "boost/coro/coro.hpp"
#include "boost/coro/yield.hpp"

namespace boost{

template<class ProtocolProcesser, typename Protocol = typename ProtocolProcesser::Protocol>
class acceptor_server :  boost::coro::coroutine
{
	boost::asio::io_service & io_service;
	boost::shared_ptr<boost::asio::basic_socket_acceptor<Protocol> > m_acceptor_socket;
	boost::shared_ptr<boost::asio::basic_stream_socket<Protocol> > m_client_socket;

	void start_accept(){
		io_service.post(boost::asio::detail::bind_handler(*this, boost::system::error_code()));
	}

public:
	typedef typename Protocol::endpoint endpoint_type;

	acceptor_server(boost::asio::io_service & io, const endpoint_type & endpoint)
		:io_service(io), m_acceptor_socket(new boost::asio::basic_socket_acceptor<Protocol>(io, endpoint))
	{
		start_accept();
	}

	acceptor_server(boost::shared_ptr<boost::asio::basic_socket_acceptor<Protocol> > acceptor_socket)
		:io_service(acceptor_socket->get_io_service()), m_acceptor_socket(acceptor_socket)
	{
		start_accept();
	}

	void operator()(const boost::system::error_code & ec)
	{
		CORO_REENTER(this)
		{
			do{
				m_client_socket.reset(new boost::asio::basic_stream_socket<Protocol>(io_service));
				_yield m_acceptor_socket->async_accept(*m_client_socket, *this);
				if (!ec)
					_fork acceptor_server<ProtocolProcesser, Protocol>(*this)(ec);
			}while (is_parent());
			std::cout <<  "child here" << std::endl;
			{ProtocolProcesser p(m_client_socket);}
		}
	}
};

}

#include "boost/coro/unyield.hpp"

#endif // AV_ACCEPTOR_SERVER_H
