#pragma once
#include <string>
#include <memory>

namespace oatpp_sio {
namespace eio {

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
};

class MessageConsumer
{
   public:
    virtual void handleMessage(std::shared_ptr<Message> msg) = 0;

    virtual void print() const = 0;
};

class MessageReceiver
{
   public:
   /** handle a message coming from the websocket
    * @return true if successful, false if failed, killing the connection and closing the socket.
    */
    virtual bool handleMessageFromWs(const std::string& msg, bool isBinary) = 0;

    virtual void shutdownConnection() = 0;
};

}  // namespace eio
}  // namespace oatpp_sio