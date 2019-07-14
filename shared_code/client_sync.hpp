#ifndef CLIENT_SYNC_HPP_
#define CLIENT_SYNC_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <thread>

#include <asio.hpp>
#include "interface_receivable.hpp"

enum class ReceiveProcess : std::uint8_t
{
    MANUAL,
    AUTOMATED
};

template<typename T>
class ClientSync
{

private:
    std::thread thread_incoming_msg_;
    asio::io_context io_context_;
    asio::ip::tcp::socket socket_;
    std::function<void(std::shared_ptr<T>)> receive_func_;
    std::function<void(asio::error_code er)> error_func_;
    bool is_ok_;
    bool thread_must_run_;
    ReceiveProcess receive_process_flag_;

public:
    ClientSync(const std::string &ip_address, std::uint16_t port);
    ~ClientSync();
    bool is_OK();
    void set_receive_callback(std::function<void(std::shared_ptr<T>)> receive_callback);
    void set_error_callback(std::function<void(asio::error_code er)> error_callback);
    void send(const char *ptr, std::size_t size);
    std::size_t receive(char* ptr, std::size_t expected_size);
    void close();
    void set_receive_process(ReceiveProcess receive_process);
    ReceiveProcess get_receive_process();

private:
    void establish_connection(asio::ip::tcp::endpoint& end_point);
    void stop_thread();
};

template<typename T>
ClientSync<T>::ClientSync(const std::string &ip_address, std::uint16_t port)
    : socket_(io_context_),
      is_ok_(false),
      thread_must_run_(true),
      receive_process_flag_(ReceiveProcess::MANUAL),
      receive_func_([](std::shared_ptr<T> msg) { std::cout << "Header size: " << msg->header_ptr() << ", Body size: " << msg->body_size()<< std::endl; }),
      error_func_([](asio::error_code er) { std::cerr << "Asio ERROR [" << er.value() << "]: " << er.message() << std::endl; })
{
    static_assert(std::is_base_of<IReceivable, T>::value, "The receive object must be base on IReceivable.");
    establish_connection(asio::ip::tcp::endpoint(asio::ip::address::from_string(ip_address), port));
}

template<typename T>
ClientSync<T>::~ClientSync()
{
    stop_thread();
    if (socket_.is_open())
        socket_.close();
}

template<typename T>
bool ClientSync<T>::is_OK()
{
    return is_ok_;
}

template<typename T>
void ClientSync<T>::set_receive_callback(std::function<void(std::shared_ptr<T>)> receive_callback)
{
    receive_func_ = receive_callback;
}

template<typename T>
void ClientSync<T>::set_error_callback(std::function<void(asio::error_code er)> error_callback)
{
    error_func_ = error_callback;
}

template<typename T>
void ClientSync<T>::send(const char *ptr, std::size_t size)
{
    if (is_ok_ && ptr && size > 0)
    {
        asio::error_code ec;
        socket_.send(asio::buffer(ptr, size), 0, ec);
        if (ec)
        {
            stop_thread();
            is_ok_ = false;
            socket_.close();
            error_func_(ec);
        }
    }
}

template<typename T>
void ClientSync<T>::close()
{
    stop_thread();
    socket_.close();
}

template<typename T>
std::size_t ClientSync<T>::receive(char *ptr, std::size_t expected_size)
{
    if (receive_process_flag_ == ReceiveProcess::MANUAL && is_ok_ && ptr && expected_size > 0)
    {
        asio::error_code ec = asio::error::would_block;
        auto len = socket_.receive(asio::buffer(ptr, expected_size), 0, ec);
        if (ec && ec.value() != asio::error::would_block)
        {
            error_func_(ec);
            is_ok_ = false;
            socket_.close();
            return 0;
        }

        return len;
    }
    else
    {
        return 0;
    }
}

template<typename T>
void ClientSync<T>::set_receive_process(ReceiveProcess receive_process)
{
    switch (receive_process)
    {
        case ReceiveProcess::MANUAL:
        {
            if (receive_process_flag_ == ReceiveProcess::AUTOMATED)
            {
                receive_process_flag_ = ReceiveProcess::MANUAL;
                stop_thread();
            }
            break;
        }

        case ReceiveProcess::AUTOMATED:
        {
            if (receive_process_flag_ == ReceiveProcess::MANUAL)
            {
                receive_process_flag_ = ReceiveProcess::AUTOMATED;
                thread_must_run_ = true;
                thread_incoming_msg_ = std::thread([&]() {
                    while (thread_must_run_)
                    {
                        asio::error_code ec = asio::error::would_block;
                        auto msg = std::make_shared<T>();
                        if(msg->header_ptr() == nullptr || msg->header_size() == 0)
                            continue;

                        auto len = socket_.receive(asio::buffer(msg->header_ptr(), msg->header_size()), 0, ec);
                        if (ec && ec.value() != asio::error::would_block)
                        {
                            error_func_(ec);
                            thread_must_run_ = false;
                            is_ok_ = false;
                            socket_.close();
                            break;
                        }

                        if (len == msg->header_size())
                        {
                            msg->prepare_body();
                            if(msg->body_ptr() == nullptr || msg->body_size() == 0)
                                continue;
                            asio::error_code ec = asio::error::would_block;
                            len = socket_.receive(asio::buffer(msg->body_ptr(), msg->body_size()), 0, ec);
                            if (ec && ec.value() != asio::error::would_block)
                            {
                                error_func_(ec);
                                thread_must_run_ = false;
                                is_ok_ = false;
                                socket_.close();
                                break;
                            }

                            if (len == msg->body_size())
                                receive_func_(msg);
                        }
                    }
                });
            }
            break;
        }

        default:
            break;
    }
}

template <typename T>
ReceiveProcess ClientSync<T>::get_receive_process()
{
    return receive_process_flag_;
}

template<typename T>
void ClientSync<T>::establish_connection(asio::ip::tcp::endpoint &end_point)
{
    asio::error_code ec;
    socket_.connect(end_point, ec);
    if (ec)
    {
        socket_.close();
        error_func_(ec);
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    socket_.non_blocking(true);
    is_ok_ = true;
}

template<typename T>
void ClientSync<T>::stop_thread()
{
    if (receive_process_flag_ == ReceiveProcess::AUTOMATED)
    {
        thread_must_run_ = false;
        if (thread_incoming_msg_.joinable())
            thread_incoming_msg_.join();
    }
}

#endif //!CLIENT_SYNC_HPP_