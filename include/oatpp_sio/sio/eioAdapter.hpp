#pragma once

#include <unordered_map>

#include "oatpp_sio/eio/engineIo.hpp"
#include "oatpp_sio/eio/connection.hpp"

#include "oatpp_sio/sio/namespaces.hpp"

namespace oatpp_sio {
namespace sio {

class EioAdapter
{
    // Socket.IO side:
    NameSpaces& universe;

    // Engine.io side:
    oatpp_sio::eio::Engine& engine;

   public:
    EioAdapter();
};

}  // namespace sio
}  // namespace oatpp_sio