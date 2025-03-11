#pragma once
#include <string>
#include <memory>

namespace oatpp_sio {

class Message
{
   public:
    std::string topic = "";
    std::string body = "";
    bool binary = false;

    Message() {}
    Message(const Message& m) : topic(m.topic), body(m.body), binary(m.binary)
    {
    }

    Message& operator=(const Message& m)
    {
        topic = m.topic;
        body = m.body;
        binary = m.binary;
        return *this;
    }

    typedef std::shared_ptr<Message> Ptr;
};

}  // namespace oatpp_sio