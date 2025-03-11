#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include "oatpp_sio/message.hpp"
// #include "oatpp_sio/eio/messageReceiver.hpp"

namespace oatpp_sio {
namespace sio {

class SpaceListener
{
    std::string myId;

    SpaceListener(const SpaceListener&) = delete;
    SpaceListener& operator=(const SpaceListener&) = delete;

   public:
    SpaceListener(const std::string& id) : myId(id) {}

    const std::string& id() const { return myId; }

    virtual void onSioMessage(oatpp_sio::Message::Ptr msg) = 0;

   public:  // convenience type-defs
    typedef std::shared_ptr<SpaceListener> Ptr;
};

class Space
{
    std::string myId;

    Space(const Space&) = delete;
    Space& operator=(const Space&) = delete;

    std::unordered_map<std::string, SpaceListener::Ptr> subscriptions;

   public:
    Space(const std::string& id) : myId(id) {}
    virtual ~Space() {}

    const std::string& id() { return myId; }

    void addListener(SpaceListener::Ptr listener);

    void removeListener(SpaceListener::Ptr listener);

    void publish(SpaceListener::Ptr sender, std::shared_ptr<oatpp_sio::Message> msg);

   public:  // convenience type-defs
    typedef std::shared_ptr<Space> Ptr;
};

}  // namespace sio
}  // namespace oatpp_sio
