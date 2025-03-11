#include <iostream>

#include "oatpp_sio/sio/namespaces.hpp"

using namespace oatpp_sio::sio;

NameSpaces* NameSpaces::universe = nullptr;

NameSpaces::NameSpaces() {
    mySpaces.insert({ "/" , std::make_shared<Space>("/")});
}

NameSpaces::~NameSpaces() {
    mySpaces.clear();
}

NameSpaces& NameSpaces::spaces()
{
    if (!universe) {
        // urknall
        universe = new NameSpaces();
    }
    return *universe;
}

Space::Ptr NameSpaces::newSpace(const std::string& id)
{
    Space::Ptr spc = std::make_shared<Space>(id);
    mySpaces.insert({id, spc});
    return spc;
}

bool NameSpaces::connectToSpace(oatpp_sio::sio::SpaceListener::Ptr listener, const std::string& spaceName)
{
    auto iter = mySpaces.find(spaceName);
    Space::Ptr spc;
    if (iter != mySpaces.end()) {
        spc = iter->second;
    } else {
        spc = newSpace(spaceName);
    }
    spc->addListener(listener);
    return true;
}
