#ifndef body_MESSAGE_HPP_
#define body_MESSAGE_HPP_

#include <vector>
#include "interface_receivable.hpp"

class ReceiveMessage : public IReceivable
{
private:
    std::vector<char> body_;
    std::int32_t body_size_;

public:
    ReceiveMessage()
    {
    }

    ~ReceiveMessage()
    {
    }

    char* header_ptr() override
    {
        return (char*)&body_size_;
    }

    char *body_ptr() override
    {
        body_size_ = (int)body_.size();
        if (body_size_ > 0)
        {
            return (char *)&body_[0];
        }
        else
        {
            return nullptr;
        }
    }

    std::int32_t header_size() override
    {
        return sizeof(body_size_);
    }

    std::int32_t body_size() override
    {
        return (int)body_.size();
    }

    void prepare_body() override
    {
        body_.resize(body_size_);
    }
};

#endif // !body_MESSAGE_HPP_