#pragma once

#include <map>
#include <string>
#include <memory>
#include <mutex>

#include "oatpp_sio/eio/connection.hpp"
#include "oatpp_sio/eio/messageReceiver.hpp"

#include "oatpp_sio/sio/adapter.hpp"

namespace oatpp_sio {

namespace eio {

class MessagePool //: public oatpp_sio::sio::SpaceAdapter
{
    typedef std::shared_ptr<MessageConsumer> MessageConsumerPtr;
    std::string name;

    std::unordered_map<std::string, MessageConsumerPtr> connections;
    std::mutex connLock;

   public:
    MessagePool(const std::string& name = "agora") : name(name) {}

    void subscribe(const std::string& id, MessageConsumerPtr conn);
    void unsubscribe(const std::string& id);

    // distribute one message to all subscribed connections as a simple text message w/o topic
    void publish(const std::string& message);

    void publish(std::shared_ptr<Message> msg);

    void printSubscribers() const;
};

}  // namespace eio
}  // namespace oatpp_sio
