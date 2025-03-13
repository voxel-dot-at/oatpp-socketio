#include "oatpp/async/ConditionVariable.hpp"
#include "oatpp/async/Coroutine.hpp"

#include "oatpp_sio/sio/namespace.hpp"

#include "oatpp_sio/eio/engineIo.hpp"

using namespace oatpp_sio::sio;
using namespace oatpp_sio;

static const bool dbg = false;

void Space::addListener(SpaceListener::Ptr listener)
{
    oatpp::async::LockGuard guard(&lock);
    subscriptions.insert({listener->id(), listener});
}

void Space::removeListener(const std::string& id)
{
    oatpp::async::LockGuard guard(&lock);
    auto iter = subscriptions.begin();
    while (iter != subscriptions.end()) {
        if (iter->second->id() == id) {
            iter = subscriptions.erase(iter);
            return;
        }
        iter++;
    }
}

SpaceListener::Ptr Space::getListener(const std::string& id) const
{
    auto iter = subscriptions.find(id);
    if (iter != subscriptions.end()) {
        return iter->second;
    } else {
        return nullptr;
    }
}

void Space::publish(std::shared_ptr<Space> space, SpaceListener::Ptr sender,
                    std::shared_ptr<oatpp_sio::Message> msg)
{
    oatpp::async::LockGuard guard(&lock);
    auto iter = subscriptions.begin();
    while (iter != subscriptions.end()) {
        iter->second->onSioMessage(space, sender, msg);
        iter++;
    }
}

void Space::publishAsync(std::shared_ptr<Space> space,
                         SpaceListener::Ptr sender,
                         std::shared_ptr<oatpp_sio::Message> msg)
{
    oatpp::async::LockGuard guard(&lock);

    class PublishCoRo : public oatpp::async::Coroutine<PublishCoRo>
    {
       private:
        std::shared_ptr<Space> space;
        SpaceListener::Ptr sender, dest;
        std::shared_ptr<oatpp_sio::Message> msg;

       public:
        PublishCoRo(std::shared_ptr<Space> space, SpaceListener::Ptr sender,
                    SpaceListener::Ptr dest,
                    std::shared_ptr<oatpp_sio::Message> msg)
            : space(space), sender(sender), msg(msg)
        {
            dest->onSioMessage(space, sender, msg);
        }

        Action act() override { return finish(); }
    };

    auto iter = subscriptions.begin();
    while (iter != subscriptions.end()) {
        // sync: iter->second->onSioMessage(space, sender, msg);
        async->execute<PublishCoRo>(space, sender, iter->second, msg);
        iter++;
    }
}
