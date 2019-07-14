#include <iostream>
#include <string>
#include <asio.hpp>
#include <csignal>
#include <memory>
#include <client_sync.hpp>
#include <receive_message.hpp>

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
    enum 
    { 
        header_size = 4,
    };

    ClientSync<ReceiveMessage> socket("127.0.0.1", 45678);
    if (!socket.is_OK())
        std::cerr << "ERROR -> socket is no connected yet !!!" << std::endl;

    try
    {
        socket.set_receive_callback([](std::shared_ptr<ReceiveMessage> msg) {
            std::cout << "Header size: " << msg->header_ptr()
                      << ", Body size: " << msg->body_size()
                      << "Msg: " << std::string(msg->body_ptr(), msg->body_ptr() + msg->body_size()) << std::endl;
        });

        socket.set_receive_process(ReceiveProcess::AUTOMATED);

        char line[512];
        while (gSignalStatus == 0)
        {
            if (std::cin.getline(line, 512))
            {
                auto size = (int32_t)std::strlen(line);
                std::vector<char> data(size + 4);
                std::copy((const char*)&size, (const char*)&size + 4, &data[0]);
                std::copy( line, line + size, (char*)(&data[0]) + 4);
                
                socket.send(data.data(), data.size());
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
