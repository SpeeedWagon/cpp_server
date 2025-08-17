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
    explicit WebSocketSession(beast::tcp_stream&& stream) : ws_(std::move(stream)) {}

    void run() {
        net::dispatch(ws_.get_executor(),
            beast::bind_front_handler(&WebSocketSession::on_run, shared_from_this()));
    }

private:
    void on_run() {
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        ws_.async_accept(
            beast::bind_front_handler(&WebSocketSession::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec) {
        if (ec)  std::cerr << "accept: " << ec.message() << "\n"; return;
        do_read();
    }

    void do_read() {
        ws_.async_read(buffer_,
            beast::bind_front_handler(&WebSocketSession::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec == websocket::error::closed) return;
        if (ec) std::cerr << "read: " << ec.message() << "\n";

        ws_.text(ws_.got_text());
        ws_.async_write(buffer_.data(),
            beast::bind_front_handler(&WebSocketSession::on_write, shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) std::cerr << "write: " << ec.message() << "\n"; return;
        buffer_.consume(buffer_.size());
        do_read();
    }
};

class HttpSession : public std::enable_shared_from_this<HttpSession> 
{
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
public:
    HttpSession(tcp::socket&& socket) : stream_(std::move(socket)) {}
    void run() {
        net::dispatch(stream_.get_executor(), beast::bind_front_handler(&HttpSession::do_read, shared_from_this()));
    }
private:
    void do_read() {
        req_ = {};
        stream_.expires_after(std::chrono::seconds(30));
        http::async_read(stream_, buffer_, req_, beast::bind_front_handler(&HttpSession::on_read, shared_from_this()));
    }
    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec == http::error::end_of_stream) {
            do_close();
            return;
        }  
        if (ec) {
            std::cerr << "read: " << ec.message() << "\n"; return;
        }  

        if (websocket::is_upgrade(req_)) {
            std::make_shared<WebSocketSession>(std::move(stream_))->run();
            return;
        }

        send_response(handle_request(std::move(req_)));
    }
    void send_response(http::message_generator&& msg) {
        bool keep_alive = msg.keep_alive();
        beast::async_write(stream_, std::move(msg),
            beast::bind_front_handler(&HttpSession::on_write, shared_from_this(), keep_alive));
    }

    void on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) {
        std::cerr << "write: " << ec.message() << "\n";
        return;
        }  
    if(keep_alive) {
        do_read();
    }
    else {
        do_close();
        } 
    }

    void do_close() {
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    }
};

class Listener : public std::enable_shared_from_this<Listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint)
        : ioc_(ioc), acceptor_(ioc) {
        beast::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) { std::cerr << "open: " << ec.message() << "\n"; return; }
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) { std::cerr << "set_option: " << ec.message() << "\n"; return; }
        acceptor_.bind(endpoint, ec);
        if (ec) { std::cerr << "bind: " << ec.message() << "\n"; return; }
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) { std::cerr << "listen: " << ec.message() << "\n"; return; }
    }

    void run() {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(net::make_strand(ioc_),
            beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            std::cerr << "accept: " << ec.message() << "\n";
        }
        else {
            std::make_shared<HttpSession>(std::move(socket))->run();
        }
        do_accept();
    }
};

int main()
{
    auto const address = net::ip::make_address("0.0.0.0");
    auto const port = static_cast<unsigned short>(8000);
    auto const threads = 4;

    std::cout << "Server starting on http://" << address << ":" << port << std::endl;

    net::io_context ioc{ threads };

    std::make_shared<Listener>(ioc, tcp::endpoint{ address, port })->run();

    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i) {
        v.emplace_back([&ioc] { ioc.run(); });
    }
    ioc.run();

    return EXIT_SUCCESS;
}

