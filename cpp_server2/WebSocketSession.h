#pragma once
#include<boost/beast/core.hpp>
#include<boost/beast/websocket.hpp>
#include<iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace net = boost::asio;

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;

public:
    explicit WebSocketSession(beast::tcp_stream&& stream, http::request<http::string_body> req) : ws_(std::move(stream)), req_(std::move(req)) {}

    void run();

private:
    void on_run();

    void on_accept(beast::error_code ec);

    void do_read();

    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    void on_write(beast::error_code ec, std::size_t bytes_transferred);
};
