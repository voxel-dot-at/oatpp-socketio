#include <memory>

#include "oatpp/json/Serializer.hpp"
#include "oatpp/json/ObjectMapper.hpp"

#include "oatpp_sio/sio/sioConnector.hpp"
#include "oatpp_sio/sio/sioServer.hpp"

using namespace oatpp_sio::sio;

void SioAdapter::shutdown()
{
    auto iter = mySpaces.begin();
    while (iter != mySpaces.end()) {
        iter->second->removeListener(id());
        iter++;
    }
    mySpaces.clear();
}

void SioAdapter::subscribed(std::shared_ptr<Space> space) {
    mySpaces.insert({space->id(), space});
}

void SioAdapter::left(std::shared_ptr<Space> space) {
    auto iter = mySpaces.find(space->id());
    if (iter != mySpaces.end()) {
        mySpaces.erase(iter);
    }
}


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

// from socketio --> send down:
void SioAdapter::onSioMessage(std::shared_ptr<Space> space, Ptr sender,
                              oatpp_sio::Message::Ptr msg)
{
    OATPP_LOGi("SADAP", "INCOMING SIo Message {} : {}", space->id(), msg->body);
    if (sender->id() == id()) {
        OATPP_LOGi("SADAP", "MSG TO SELF {} : {}", sender->id(), id());
        return;
    }
    // @TODO: encode packets here

    eioConn->handleMessage(msg);
}

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
    std::string spc = "/";
    // find the start of things...
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
        if (data[i] == ',') {  // found non-standard namespace
            spc = data.substr(0, i - 1);
            break;
        }
    }

    if (start > 0) {
        ackId = data.substr(0, start);
        OATPP_LOGi("SADAP", "onSioEvent ack {} |{}| spc {}", start, ackId, spc);
    }
    std::string payload = data.substr(start);
    if (arrStart > 0) {
        // emit!
        OATPP_LOGi("SADAP", "onSioEvent EMIT |{}|", payload);
    }
    if (objStart > 0) {
        OATPP_LOGi("SADAP", "onSioEvent PUBL |{}|", payload);
    }
    // publish to space
    {
        auto self = eioConn->getSio();
        auto msg = std::make_shared<oatpp_sio::Message>();
        msg->body = payload;
        auto space = SioServer::serverInstance().getSpace(spc);
        space->publish(space, self, msg);
    }
    // ack message...:
    bool success = start > 0;
    if (success) {
        std::string encoded;
        encoded = "3" + ackId + "[]";

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
        std::string encoded;  // -> disconnect
        encoded = "1";        //{\"sid\":\"" + sioId + "\"}";

        auto msg = std::make_shared<oatpp_sio::Message>();
        msg->body = encoded;
        this->eioConn->handleMessage(msg);
    }
}
