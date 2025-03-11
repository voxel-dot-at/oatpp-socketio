#pragma once
#include <string>
#include <memory>

#include "oatpp_sio/message.hpp"

namespace oatpp_sio {
namespace eio {

typedef oatpp_sio::Message Message;

class MessageConsumer
{
   public:
    virtual void handleMessage(std::shared_ptr<Message> msg) = 0;

    virtual void print() const = 0;

    typedef std::shared_ptr<MessageConsumer> Ptr;
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