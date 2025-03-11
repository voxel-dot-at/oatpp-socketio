#include "oatpp_sio/sio/namespace.hpp"

using namespace oatpp_sio::sio;
using namespace oatpp_sio;

void Space::addListener(SpaceListener::Ptr listener)
{
    subscriptions.insert({listener->id(), listener});
}

void Space::removeListener(SpaceListener::Ptr listener)
{
    auto iter = subscriptions.begin();
    while (iter != subscriptions.end()) {
        if (iter->second == listener) {
            iter = subscriptions.erase(iter);
            return;
        }
        iter++;
    }
}

void Space::publish(SpaceListener::Ptr sender, std::shared_ptr<oatpp_sio::Message> msg)
{
    auto iter = subscriptions.begin();
    while (iter != subscriptions.end()) {
        iter->second->onSioMessage(msg);
        iter++;
    }
}

