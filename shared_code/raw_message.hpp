#ifndef NET_MESSAGE_HPP
#define NET_MESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

class RawMessage : std::enable_shared_from_this<RawMessage>
{
  public:
    enum
    {
        header_length = 4,
        max_body_length = 512
    };

    RawMessage()
        : body_length_(0)
    {
    }

    const char *data() const
    {
        return data_;
    }

    char *data()
    {
        return data_;
    }

    std::size_t length() const
    {
        return header_length + body_length_;
    }

    const char *body() const
    {
        return data_ + header_length;
    }

    char *body()
    {
        return data_ + header_length;
    }

    std::size_t body_length() const
    {
        return body_length_;
    }

    void body_length(std::size_t new_length)
    {
        body_length_ = new_length;
        if (body_length_ > max_body_length)
            body_length_ = max_body_length;
    }

    bool decode_header()
    {
        std::copy((const char*)data_, (const char*)data_ + 4, (char*)&body_length_);
        if (body_length_ > max_body_length)
        {
            body_length_ = 0;
            return false;
        }
        return true;
    }

    void encode_header()
    {
        auto size = static_cast<int>(body_length_);
        std::copy((const char*)&size, (const char*)(&size) + header_length, data_);
    }

  private:
    char data_[header_length + max_body_length];
    std::size_t body_length_;
};

#endif // NET_MESSAGE_HPP