#include "oatpp_sio/sio/sioServer.hpp"
#include "oatpp_sio/util.hpp"

using namespace oatpp_sio::sio;

SioServer* SioServer::universe = nullptr;

SioServer::SioServer()
{
    auth = std::make_shared<SioAuth>();
    newSpace("/");
}

SioServer::~SioServer() {
}

SioServer& SioServer::serverInstance()
{
    if (!universe) {
        universe = new SioServer();
    }
    return *universe;
}

Space::Ptr SioServer::getSpace(const std::string& id) const
{
    auto iter = mySpaces.find(id);
    if (iter == mySpaces.end()) {
        throw std::runtime_error("space does not exist!");
    }
    return iter->second;
}

Space::Ptr SioServer::newSpace(const std::string& id)
{
    auto iter = mySpaces.find(id);
    if (iter != mySpaces.end()) {
        throw std::runtime_error("space exists!");
    }
    auto spc = std::make_shared<Space>("id");
    mySpaces.insert({id, spc});
    return spc;
}

bool SioServer::connectToSpace(const std::string& spaceName,
                               oatpp_sio::sio::SpaceListener::Ptr listener,
                               std::string& sioId)
{
    Space::Ptr space = getSpace(spaceName);

    OATPP_LOGd("SioServer", "connectToSpace -> {} ", spaceName);

    if (!space.get()) {
        OATPP_LOGd("SioServer", "connectToSpace() SioServer could not find {} ",
                   spaceName);
        return false;
    }
    // @TODO: AUTH connection here...
    bool authed = true;
    if (authed) {
        sioId = generateRandomString(SID_LENGTH);
        space->addListener(listener);
        listener->subscribed(space);
    }
    return authed;
}

bool SioServer::leaveSpace(const std::string& spaceName, std::string& sioId)
{
    Space::Ptr space = getSpace(spaceName);
    if (!space.get()) {
        OATPP_LOGw("SioServer", "leaveSpace could not find {} ", spaceName);
        return false;
    }
    space->removeListener(sioId);

    auto listener = space->getListener(sioId);
    if (listener.get()) {
        listener->left(space);
    }
    return true;
}