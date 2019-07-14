#include <iostream>
#include <server_async.hpp>

int main(int argc, char *argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: server <port>\n";
            return 1;
        }

        asio::io_context io_context;
        tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[1]));

        ServerAsync server(io_context, endpoint);

        server.set_receive_func([&](std::shared_ptr<RawMessage> incoming_msg) {
            std::cout.write(incoming_msg->body(), incoming_msg->body_length());
            std::cout << "\n";

            auto msg_ptr = std::make_shared<RawMessage>();
            auto str = std::string("server received: ").append(std::string(incoming_msg->body(), incoming_msg->body() + incoming_msg->body_length()));
            msg_ptr->body_length(str.size());
            std::memcpy(msg_ptr->body(), str.data(), msg_ptr->body_length());
            msg_ptr->encode_header();
            server.send(msg_ptr);
        });

        {
            std::thread context_thread([&io_context]() { io_context.run(); });
             char line[RawMessage::max_body_length + 1];
            while (std::cin.getline(line, RawMessage::max_body_length + 1))
            {
                auto msg_ptr = std::make_shared<RawMessage>();
                msg_ptr->body_length(std::strlen(line));
                std::memcpy(msg_ptr->body(), line, msg_ptr->body_length());
                msg_ptr->encode_header();
                server.send(msg_ptr);
            }

            context_thread.join();
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}