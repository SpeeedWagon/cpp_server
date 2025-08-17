#pragma once
#include<iostream>
#include<boost/beast/core.hpp>
#include<boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include<boost/config.hpp>

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class Listener : public std::enable_shared_from_this<Listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint);

    void run();

private:
    void do_accept();

    void on_accept(beast::error_code ec, tcp::socket socket);
};