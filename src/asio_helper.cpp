#include "asio_helper.hpp"

SocketSync::SocketSync(const std::string &ip_address, std::uint16_t port)
    : socket_(io_context_),
      is_ok_(false),
      thread_must_run_(true),
      receive_func_([](const char *ptr, std::size_t size) { std::cout << size << " bytes rceived"; })
{
    establish_connection(asio::ip::tcp::endpoint(asio::ip::address::from_string(ip_address), port));
}

SocketSync::~SocketSync()
{
    stop_thread();
    socket_.close();
}

bool SocketSync::is_OK()
{
    return is_ok_;
}

void SocketSync::set_receive_callback(std::function<void(const char *, std::size_t)> receive_callback)
{
    receive_func_ = receive_callback;
}

void SocketSync::send(const char *ptr, std::size_t size)
{
    if (is_ok_ && size > 0)
    {
        asio::error_code ec;
        socket_.send(asio::buffer(ptr, size), 0, ec);
        if (ec)
        {
            stop_thread();
            is_ok_ = false;
            socket_.close();
            std::cout << "Asio error:" << '\n'
                      << "\n\tCode: " << ec.value()
                      << "\n\tMessage: " << ec.message() << std::endl;
        }
    }
}

void SocketSync::close()
{
    stop_thread();
    socket_.close();
}

void SocketSync::establish_connection(asio::ip::tcp::endpoint &end_point)
{
    asio::error_code ec;
    socket_.connect(end_point, ec);
    if (ec)
    {
        socket_.close();
        std::cout << "Asio error:" << '\n'
                  << "\n\tCode: " << ec.value()
                  << "\n\tMessage: " << ec.message() << std::endl;
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        socket_.non_blocking(true);
        is_ok_ = true;
        thread_incoming_msg_ = std::thread([&]() {
            while (thread_must_run_)
            {
                asio::error_code ec = asio::error::would_block;
                std::vector<char> msg(1024);
                auto len = socket_.receive(asio::buffer(msg), 0, ec);
                if (ec && ec.value() != 10035)
                {
                    std::cout << "Asio error:" << '\n'
                              << "\n\tCode: " << ec.value()
                              << "\n\tMessage: " << ec.message() << std::endl;
                    thread_must_run_ = false;
                    is_ok_ = false;
                    socket_.close();
                    break;
                }

                if (len > 0)
                {
                    receive_func_(msg.data(), len);
                }
            }
        });
    }
}

void SocketSync::stop_thread()
{
    thread_must_run_ = false;
    if (thread_incoming_msg_.joinable())
        thread_incoming_msg_.join();
}