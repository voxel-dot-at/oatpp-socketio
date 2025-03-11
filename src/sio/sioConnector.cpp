#include <memory>

#include "oatpp/json/Serializer.hpp"
#include "oatpp/json/ObjectMapper.hpp"

#include "oatpp_sio/sio/sioConnector.hpp"
#include "oatpp_sio/sio/sioServer.hpp"

using namespace oatpp_sio::sio;

// low-level ->up
void SioAdapter::onEioMessage(oatpp_sio::Message::Ptr msg)
{
    OATPP_LOGi("SADAP", "SIo Packet... {}", msg->body);
    // decode, then push
    char pType = msg->body[0];
    switch (pType) {
        case SioPacketType::connect:
            onSioConnect(msg->body.substr(1));
            break;
        case SioPacketType::event:
            onSioEvent(msg->body.substr(1));
            break;
        default:
            OATPP_LOGi("SADAP", "Unknown SIo Packet {} {}", pType, (char)pType);
    }
}

void SioAdapter::onSioMessage(oatpp_sio::Message::Ptr msg) {}

static auto json = std::make_shared<oatpp::json::ObjectMapper>();

// from socketio
void SioAdapter::onSioEvent(const std::string& data)
{
    OATPP_LOGi("SADAP", "onSioEvent |{}|", data);
    std::string ackId = "";
    //    2    0["foo",{"d":1741689505974}]

    int start = -1;
    int arrStart = -1;
    int objStart = -1;
    for (unsigned int i = 0; i < data.size(); i++) {
        if (data[i] == '[') {
            arrStart = i;
            start = i;
            break;
        }
        if (data[i] == '{') {
            objStart = i;
            start = i;
            break;
        }
    }

    if (start > 0) {
        ackId = data.substr(0, start );
        OATPP_LOGi("SADAP", "onSioEvent ack {} |{}|", start, ackId);
    }
    std::string payload = data.substr(start);
    if (arrStart > 0) {
        // emit!
        OATPP_LOGi("SADAP", "onSioEvent EMIT |{}|", payload);
    }
    if (objStart > 0) {
        OATPP_LOGi("SADAP", "onSioEvent PUBL |{}|", payload);
    }
    bool success = start > 0;
    if (success) {
        std::string encoded;
        encoded = "3" + ackId;

        auto msg = std::make_shared<oatpp_sio::Message>();
        msg->body = encoded;
        this->eioConn->handleMessage(msg);
    }
}

// from socketio
void SioAdapter::onSioConnect(const std::string& connTo)
{
    OATPP_LOGi("SADAP", "onSioConnect... |{}|", connTo);
    // if (msg->binary) {
    //     OATPP_LOGw("SIO", "Unknown binary SIo Packet from space!");
    //     return;
    // }
    std::string space = "/";
    if (connTo.size()) {
        int obj = connTo.find('{');
        // int arr = connTo.find('{');
        if (obj >= 0) {
            std::string data = connTo.substr(obj);
            //json->
        }
    }
    std::string sioId;
    auto self = eioConn->getSio();
    bool success =
        SioServer::serverInstance().connectToSpace(space, self, sioId);

    if (success) {
        std::string encoded;
        encoded = "0{\"sid\":\"" + sioId + "\"}";

        auto msg = std::make_shared<oatpp_sio::Message>();
        msg->body = encoded;
        this->eioConn->handleMessage(msg);
    } else {
        std::string encoded; // -> disconnect
        encoded = "1";//{\"sid\":\"" + sioId + "\"}";

        auto msg = std::make_shared<oatpp_sio::Message>();
        msg->body = encoded;
        this->eioConn->handleMessage(msg);

    }
}
