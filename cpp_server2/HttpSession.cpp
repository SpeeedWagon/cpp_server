#include "HttpSession.h"
#include "WebSocketSession.h"

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

inline void HttpSession::run() {
    net::dispatch(stream_.get_executor(), beast::bind_front_handler(&HttpSession::do_read, shared_from_this()));
}

inline void HttpSession::do_read() {
    req_ = {};
    stream_.expires_after(std::chrono::seconds(30));
    http::async_read(stream_, buffer_, req_, beast::bind_front_handler(&HttpSession::on_read, shared_from_this()));
}

inline void HttpSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
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

inline void HttpSession::send_response(http::message_generator&& msg) {
    bool keep_alive = msg.keep_alive();
    beast::async_write(stream_, std::move(msg),
        beast::bind_front_handler(&HttpSession::on_write, shared_from_this(), keep_alive));
}

inline void HttpSession::on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        std::cerr << "write: " << ec.message() << "\n";
        return;
    }
    if (keep_alive) {
        do_read();
    }
    else {
        do_close();
    }
}

inline void HttpSession::do_close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}