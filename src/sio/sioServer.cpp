#include "oatpp_sio/sio/sioServer.hpp"
#include "oatpp_sio/util.hpp"

using namespace oatpp_sio::sio;

SioServer* SioServer::universe = nullptr;

SioServer::SioServer()
{
    auto spc = std::make_shared<Space>("/");
    mySpaces.insert({"/", spc});
}

SioServer::~SioServer() {}

SioServer& SioServer::serverInstance()
{
    if (!universe) {
        universe = new SioServer();
    }
    return *universe;
}

Space::Ptr SioServer::newSpace(const std::string& id)
{
    auto iter = mySpaces.find(id);
    if (iter != mySpaces.end()) {
        throw std::runtime_error("space exists!");
    }
    auto spc = std::make_shared<Space>("id");
    mySpaces.insert({id, spc});
}

bool SioServer::connectToSpace(const std::string& spaceName,
                               oatpp_sio::sio::SpaceListener::Ptr listener,
                               std::string& sioId)
{
    auto iter = mySpaces.find(spaceName);
    if (iter == mySpaces.end()) {
            OATPP_LOGd("SioServer", "SioServer could not find {} ", spaceName);
            return false;
    }
    sioId = generateRandomString(6);
    iter->second->addListener(listener);

    return true;
}