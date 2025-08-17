

#include <iostream>
#include <memory>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>


namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class WebSocketSession;

http::response<http::string_body> handle_request(http::request<http::string_body>&& req) {

    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        http::response<http::string_body> res{ http::status::bad_request, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "Unknown HTTP-method";
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> res{ http::status::ok, req.version() };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.body() = "<h1>Hello from C++ Server!</h1><p>Try connecting with a WebSocket client.</p>";
    res.prepare_payload();
    return res;
}

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;

public:
    explicit WebSocketSession(beast::tcp_stream&& stream) : ws_(std::move(stream)) {};
    void run() {
        net::dispatch(ws_.get_executor(), beast::bind_front_handler(&WebSocketSession::on_run, shared_from_this()));
    }
private:
    void on_run() {
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        ws_.async_accept(
            beast::bind_front_handler(&WebSocketSession::on_accept, shared_from_this())
        );
    }
    void on_accept(beast::error_code ec) {
        if (ec) return;
        do_read();
    }
    void do_read() {
        ws_.async_read(buffer_, beast::bind_front_handler(&WebSocketSession::on_read, shared_from_this()));
    }
    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec == websocket::error::closed) return;
        if (ec) std::cerr << "read: " << ec.message() << "\n";
        ws_.text(ws_.got_text());
        ws_.async_write(buffer_.data(), beast::bind_front_handler(&WebSocketSession::on_write, shared_from_this()));
    }
    void on_write(beast::error_code ec, std::size_t bytes_transfered) {
        boost::ignore_unused(bytes_transfered);
        if (ec) std::cerr << "write: " << ec.message() << "\n";
        buffer_.consume(buffer_.size());
        do_read();
    }
};


int main()
{
    std::cout << "Hello World!123\n";
}

