#include <iostream>
#include <string>
#include <asio.hpp>
#include <csignal>
#include <memory>
#include "raw_message.hpp"
#include "asio_helper.hpp"

namespace
{
volatile std::sig_atomic_t gSignalStatus;
}

void signal_handler(int signal)
{
    gSignalStatus = signal;
}

int main(int argc, char const *argv[])
{
    std::signal(SIGINT, signal_handler);

    SocketSync socket("127.0.0.1", 45678);
    if (!socket.is_OK())
        std::cerr << "ERROR -> socket is no connected yet !!!" << std::endl;

    try
    {
        socket.set_receive_callback([](const char *ptr, std::size_t size) {
            std::cout << size << " bytes rceived from server: ";
            std::cout.write(ptr + 4, size);
            std::cout << std::endl;
        });

        char line[512];
        while (gSignalStatus == 0)
        {
            if (std::cin.getline(line, 512))
            {
                auto msg_ptr = std::make_shared<RawMessage>();
                msg_ptr->body_length(std::strlen(line));
                std::memcpy(msg_ptr->body(), line, msg_ptr->body_length());
                msg_ptr->encode_header();
                asio::error_code ec;
                socket.send(msg_ptr->data(), msg_ptr->length());
            }
        }
    }
    catch (const std::exception &e)
    {
        socket.close();
        std::cerr << e.what() << '\n';
    }

    socket.close();

    return 0;
}
