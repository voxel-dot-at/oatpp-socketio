#pragma once

#include <string>
#include <memory>
#include <unordered_map>

// timer functionality...:
#include "oatpp/async/Lock.hpp"
#include "oatpp/async/ConditionVariable.hpp"

#include "oatpp/async/Executor.hpp"

#include "oatpp/macro/component.hpp"

#include "oatpp_sio/message.hpp"
// #include "oatpp_sio/eio/messageReceiver.hpp"

namespace oatpp_sio {
namespace sio {

class Space;

class SpaceListener
{
    std::string myId;

    SpaceListener(const SpaceListener&) = delete;
    SpaceListener& operator=(const SpaceListener&) = delete;

   public:  // convenience type-defs
    typedef std::shared_ptr<SpaceListener> Ptr;

   public:
    SpaceListener(const std::string& id) : myId(id) {}

    const std::string& id() const { return myId; }

    virtual void onSioMessage(std::shared_ptr<Space> space, Ptr sender,
                              oatpp_sio::Message::Ptr msg) = 0;

    virtual void subscribed(std::shared_ptr<Space> space) {}
    virtual void left(std::shared_ptr<Space> space) {}
};

class Space
{
    std::string myId;

    Space(const Space&) = delete;
    Space& operator=(const Space&) = delete;

    std::unordered_map<std::string, SpaceListener::Ptr> subscriptions;
    // lock for manipulations of the subscriptions map
    oatpp::async::Lock lock;

    OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, async, "ws");

   public:
    Space(const std::string& id) : myId(id) {}
    virtual ~Space() {}

    const std::string& id() { return myId; }

    const std::unordered_map<std::string, SpaceListener::Ptr>& subs()
    {
        return subscriptions;
    }

    int size() const { return subscriptions.size(); }

    void addListener(SpaceListener::Ptr listener);

    void removeListener(const std::string& id);

    SpaceListener::Ptr getListener(const std::string& id) const;

    void publish(std::shared_ptr<Space> space, SpaceListener::Ptr sender,
                 std::shared_ptr<oatpp_sio::Message> msg);

    void publishAsync(std::shared_ptr<Space> space, SpaceListener::Ptr sender,
                      std::shared_ptr<oatpp_sio::Message> msg);

   public:  // convenience type-defs
    typedef std::shared_ptr<Space> Ptr;
};

}  // namespace sio
}  // namespace oatpp_sio
