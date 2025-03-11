#pragma once

#include <map>
#include <unordered_map>
#include <string>
#include <memory>

#include "oatpp_sio/sio/namespace.hpp"

namespace oatpp_sio {
namespace sio {

/** collection of available namespaces */
class NameSpaces

{
    static NameSpaces* universe;
    std::unordered_map<std::string, Space::Ptr> mySpaces;

    NameSpaces();
    virtual ~NameSpaces();

   public:
    static NameSpaces& spaces();

    Space::Ptr newSpace(const std::string& id);

    // void dropSpace(const std::string& id);

    /**
     * @return true if ok, false if not
     */
    bool connectToSpace(oatpp_sio::sio::SpaceListener::Ptr listener, const std::string& id);
};

}  // namespace sio
}  // namespace oatpp_sio
