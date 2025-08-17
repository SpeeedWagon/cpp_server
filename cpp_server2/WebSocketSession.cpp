#include "WebSocketSession.h"

inline void WebSocketSession::run() {
    net::dispatch(ws_.get_executor(),
        beast::bind_front_handler(&WebSocketSession::on_run, shared_from_this()));
}

inline void WebSocketSession::on_run() {
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws_.async_accept(
        beast::bind_front_handler(&WebSocketSession::on_accept, shared_from_this()));
}

inline void WebSocketSession::on_accept(beast::error_code ec) {
    if (ec)  std::cerr << "accept: " << ec.message() << "\n"; return;
    do_read();
}

inline void WebSocketSession::do_read() {
    ws_.async_read(buffer_,
        beast::bind_front_handler(&WebSocketSession::on_read, shared_from_this()));
}

inline void WebSocketSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec == websocket::error::closed) return;
    if (ec) std::cerr << "read: " << ec.message() << "\n";

    ws_.text(ws_.got_text());
    ws_.async_write(buffer_.data(),
        beast::bind_front_handler(&WebSocketSession::on_write, shared_from_this()));
}

inline void WebSocketSession::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) std::cerr << "write: " << ec.message() << "\n"; return;
    buffer_.consume(buffer_.size());
    do_read();
}
