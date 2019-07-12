#ifndef ASIO_HELPER_HPP_
#define ASIO_HELPER_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <thread>

#include <asio.hpp>

class SocketSync
{
private:
    std::thread thread_incoming_msg_;
    asio::io_context io_context_;
    asio::ip::tcp::socket socket_;
    std::function<void(const char *, std::size_t)> receive_func_;
    bool is_ok_;
    bool thread_must_run_;

public:
    SocketSync(const std::string &ip_address, std::uint16_t port);
    ~SocketSync();
    bool is_OK();
    void set_receive_callback(std::function<void(const char *, std::size_t)> receive_callback);
    void send(const char *ptr, std::size_t size);
    void close();

private:
    void establish_connection(asio::ip::tcp::endpoint& end_point);
    void stop_thread();
};

#endif //!ASIO_HELPER_HPP_