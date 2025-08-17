#pragma once
#include<iostream>
#include<boost/beast/core.hpp>
#include<boost/beast/http.hpp>
#include<boost/asio/dispatch.hpp>
#include<boost/asio/ip/tcp.hpp>
#include<boost/beast/websocket.hpp>
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class HttpSession : public std::enable_shared_from_this<HttpSession>
{
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
public:
    HttpSession(tcp::socket&& socket) : stream_(std::move(socket)) {}
    void run();
private:
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void send_response(http::message_generator&& msg);

    void on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred);

    void do_close();
};
