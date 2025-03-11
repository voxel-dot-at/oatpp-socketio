#pragma once

#include <unordered_map>

#include "oatpp_sio/eio/engineIo.hpp"
#include "oatpp_sio/eio/connection.hpp"

#include "oatpp_sio/sio/namespaces.hpp"

namespace oatpp_sio {
namespace sio {

/** A connector between the lower-level engine connection and a number of socket.io
 * namespaces. this is also responsible for en/decoding the messages into the wire format  */
class SioServer
{
    static SioServer* universe;
    std::unordered_map<std::string, Space::Ptr> mySpaces;

    SioServer();
    virtual ~SioServer();

   public:
    static SioServer& serverInstance();

    Space::Ptr newSpace(const std::string& id);

    // void dropSpace(const std::string& id);

    /**
     * @return true if ok, false if not
     */
    bool connectToSpace(const std::string& spaceName,
                        oatpp_sio::sio::SpaceListener::Ptr listener,
                        std::string& sioId);
};

}  // namespace sio
}  // namespace oatpp_sio