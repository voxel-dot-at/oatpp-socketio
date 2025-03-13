#pragma once

#include <unordered_map>

#include "oatpp_sio/eio/engineIo.hpp"
#include "oatpp_sio/eio/connection.hpp"

#include "oatpp_sio/sio/namespaces.hpp"

namespace oatpp_sio {

namespace eio {

class EioConnection;

}

namespace sio {

typedef enum
{
    connect = '0',
    disconnect,     // 1
    event,          // 2
    ack,            // 3
    connect_error,  // 4
    binary_event,   // 5
    binary_ack      // 6
} SioPacketType;

class SioMessage : public oatpp_sio::Message
{
    std::string space = "/";
    // std::vector<std::string> binAttachments;
};

/** A connector between the lower-level engine connection and a number of socket.io
 * namespaces. this is also responsible for en/decoding the messages into the wire 
 * format and funneling events to the different spaces that are connected  
 */
class SioAdapter : public SpaceListener
{
   public:
    SioAdapter(const std::string& id) : SpaceListener(id) {}

    std::shared_ptr<oatpp_sio::eio::EioConnection> eioConn;

    std::unordered_map<std::string, Space::Ptr> mySpaces;

    // low-level ->up
    virtual void onEioMessage(oatpp_sio::Message::Ptr msg);

    // from socketio
    virtual void onSioMessage(std::shared_ptr<Space> space, Ptr sender,
                              oatpp_sio::Message::Ptr msg) override;

    void shutdown();

   private:
    void onSioConnect(const std::string& data);

    void onSioEvent(const std::string& data);
};

}  // namespace sio
}  // namespace oatpp_sio