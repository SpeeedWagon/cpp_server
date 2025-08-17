#include <string>

#include "WebSocketSession.h"
#include "Listener.h"
#include "HttpSession.h"

//namespace beast = boost::beast;
//namespace http = beast::http;
//namespace websocket = beast::websocket;
//namespace net = boost::asio;
//using tcp = boost::asio::ip::tcp;
//
//class WebSocketSession;

//http::response<http::string_body> handle_request(http::request<http::string_body>&& req) {
//
//    if (req.method() != http::verb::get && req.method() != http::verb::head) {
//        http::response<http::string_body> res{ http::status::bad_request, req.version() };
//        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
//        res.set(http::field::content_type, "text/html");
//        res.keep_alive(req.keep_alive());
//        res.body() = "Unknown HTTP-method";
//        res.prepare_payload();
//        return res;
//    }
//
//    http::response<http::string_body> res{ http::status::ok, req.version() };
//    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
//    res.set(http::field::content_type, "text/html");
//    res.body() = "<h1>Hello from C++ Server!</h1><p>Try connecting with a WebSocket client.</p>";
//    res.prepare_payload();
//    return res;
//}

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

