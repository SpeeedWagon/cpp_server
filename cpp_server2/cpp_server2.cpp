#include <string>
#include "WebSocketSession.h"
#include "Listener.h"
#include "HttpSession.h"

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

