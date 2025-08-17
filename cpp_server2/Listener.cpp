#include "Listener.h"
#include "HttpSession.h"

inline Listener::Listener(net::io_context& ioc, tcp::endpoint endpoint)
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

inline void Listener::run() {
    do_accept();
}

inline void Listener::do_accept() {
    acceptor_.async_accept(net::make_strand(ioc_),
        beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
}

inline void Listener::on_accept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        std::cerr << "accept: " << ec.message() << "\n";
    }
    else {
        std::make_shared<HttpSession>(std::move(socket))->run();
    }
    do_accept();
}