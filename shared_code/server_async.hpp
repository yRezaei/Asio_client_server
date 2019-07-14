#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <functional>
#include <string>
#include <set>
#include <utility>
#include <asio.hpp>
#include "raw_message.hpp"

using asio::ip::tcp;

class ServerAsync
{
    typedef std::deque<std::shared_ptr<RawMessage>> RawMessage_queue;

private:
    class Session
    {
    public:
        std::function<void(std::shared_ptr<RawMessage>)> receive_callback;

    public:
        Session(tcp::socket socket, std::function<void(void)> handle_error)
            : socket_(std::move(socket)),
              handle_error_(handle_error)
        {
        }

        void start()
        {
            do_read_header();
        }

        void deliver(std::shared_ptr<RawMessage> msg_data)
        {
            bool write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(msg_data);
            if (!write_in_progress)
            {
                do_write();
            }
        }

    private:
        void do_read_header()
        {
            incoming_msg = std::make_shared<RawMessage>();
            asio::async_read(socket_,
                             asio::buffer(incoming_msg->data(), RawMessage::header_length),
                             [this](std::error_code ec, std::size_t /*length*/) {
                                 if (!ec && incoming_msg->decode_header())
                                 {
                                     do_read_body();
                                 }
                                 else
                                 {
                                     handle_error_();
                                 }
                             });
        }

        void do_read_body()
        {
            asio::async_read(socket_,
                             asio::buffer(incoming_msg->body(), incoming_msg->body_length()),
                             [this](std::error_code ec, std::size_t /*length*/) {
                                 if (!ec)
                                 {
                                     receive_callback(incoming_msg);
                                     do_read_header();
                                 }
                                 else
                                 {
                                     handle_error_();
                                 }
                             });
        }

        void do_write()
        {
            asio::async_write(socket_,
                              asio::buffer(write_msgs_.front()->data(),
                                           write_msgs_.front()->length()),
                              [this](std::error_code ec, std::size_t /*length*/) {
                                  if (!ec)
                                  {
                                      write_msgs_.pop_front();
                                      if (!write_msgs_.empty())
                                      {
                                          do_write();
                                      }
                                  }
                                  else
                                  {
                                      handle_error_();
                                  }
                              });
        }

        tcp::socket socket_;
        std::shared_ptr<RawMessage> incoming_msg;
        RawMessage_queue write_msgs_;
        std::function<void(void)> handle_error_;
    };

public:
    ServerAsync(asio::io_context &io_context, const tcp::endpoint &endpoint)
        : acceptor_(io_context, endpoint), handl_error_([&]() { close_session(); })
    {
        do_accept();
    }

    ~ServerAsync()
    {
        close_session();
    }

    void send(std::shared_ptr<RawMessage> msg_ptr)
    {
        session_->deliver(msg_ptr);
    }

    void set_receive_func(std::function<void(std::shared_ptr<RawMessage>)> receive_func)
    {
        receive_func_ = receive_func;
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(
            [this](std::error_code ec, tcp::socket socket) {
                if (!ec)
                {
                    session_ = std::make_unique<Session>(std::move(socket), handl_error_);
                    session_->receive_callback = receive_func_;
                    session_->start();
                }
            });
    }

    void close_session()
    {
        if (session_)
        {
            session_.reset();
            session_ = nullptr;
        }
    }

    tcp::acceptor acceptor_;
    std::unique_ptr<Session> session_;
    std::function<void(void)> handl_error_;
    std::function<void(std::shared_ptr<RawMessage>)> receive_func_;
};
