#ifndef INTERFACE_RECEIVABLE_HPP_
#define INTERFACE_RECEIVABLE_HPP_

#include <cstddef>

class IReceivable
{
    public:
    virtual ~IReceivable() {}
    virtual char* header_ptr() = 0;
    virtual char* body_ptr() = 0;
    virtual std::int32_t header_size() = 0;
    virtual std::int32_t body_size() = 0;
    virtual void prepare_body() = 0;
};

#endif // !INTERFACE_RECEIVABLE_HPP_