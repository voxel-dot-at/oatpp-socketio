#include <memory>

#include "oatpp/json/Serializer.hpp"
#include "oatpp/json/ObjectMapper.hpp"

#include "oatpp_sio/sio/sioConnector.hpp"
#include "oatpp_sio/sio/sioServer.hpp"

#include <iostream>
using namespace std;

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

void SioAdapter::subscribed(std::shared_ptr<Space> space)
{
    mySpaces.insert({space->id(), space});
}

void SioAdapter::left(std::shared_ptr<Space> space)
{
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
    oatpp_sio::Message::Ptr m = std::make_shared<oatpp_sio::Message>(*msg);
    if (space->id() != "/") {
        m->body = "2" + space->id() + "," + m->body;
    } else {
        m->body = "2" + m->body;
    }

    OATPP_LOGi("SADAP", "INCOMING SIo Message {} : {}", space->id(), m->body);
    eioConn->handleMessage(m);
    // eioConn->handleMessage(msg);
}

static auto json = std::make_shared<oatpp::json::ObjectMapper>();

static bool parseMsg(const std::string& data, std::string& nbin,
                     std::string& nsp, std::string& ack, std::string& payload)
{
    unsigned int i = 0;
    if (data[i] >= '0' && data[i] <= '9') {
        // TODO: parse # binary packets of available.
        unsigned int start = i;
        for (; i < data.size(); i++) {
            if (data[i] < '0' || data[i] > '9') {
                break;
            }
        }
        if (data[i] != '-') {
            OATPP_LOGw("PRS", "Parse error in # bin packets!");
            return false;
        }
        if (i > start) {
            nbin = data.substr(start, i - start);
        }
    }
    if (data[i] == '/') {
        // collect namespace:
        unsigned int start = i;

        for (; i < data.size(); i++) {
            if (data[i] == ',') {
                break;
            }
        }
        if (data[i] != ',') {
            OATPP_LOGw("PRS", "Parse error in namespace!");
            return false;
        }
        nsp = data.substr(start, i);
        i++;  // jump over ','
    } else {
        nsp = "/";
    }

    if (data[i] >= '0' && data[i] <= '9') {
        // parse ack id:
        unsigned int start = i;
        for (; i < data.size(); i++) {
            if (data[i] < '0' || data[i] > '9') {
                break;
            }
        }
        if (i > start) {
            ack = data.substr(start, i - start);
        }
    }

    // rest is json data
    payload = data.substr(i);

    return true;
}

void SioAdapter::onSioEvent(const std::string& data)
{
    OATPP_LOGi("SADAP", "onSioEvent |{}|", data);
    std::string nbin = "";
    std::string nsp = "/";
    std::string ackId = "";
    std::string payload = "{}";

    bool success = parseMsg(data, nbin, nsp, ackId, payload);

    OATPP_LOGi("SADAP", "onSioEvent EMIT |{}|", payload);

    // publish to space
    {
        auto self = eioConn->getSio();
        auto msg = std::make_shared<oatpp_sio::Message>();
        msg->body =  payload;
        auto space = SioServer::serverInstance().getSpace(nsp);
        space->publish(space, self, msg);
    }
    // ack message...:
    if (success) {
        std::string encoded;
        encoded = "3";
        if (nsp != "/") {  // encode namespace
            encoded += nsp + ",";
        }
        encoded += ackId + "[]";

        auto msg = std::make_shared<oatpp_sio::Message>();
        msg->body = encoded;
        this->eioConn->handleMessage(msg);
    }
}

// from socketio
void SioAdapter::onSioConnect(const std::string& connTo)
{
    OATPP_LOGi("SADAP", "onSioConnect... |{}|", connTo);

    std::string nbin = "";
    std::string space = "/";
    std::string ackId = "";
    std::string payload = "{}";

    bool success = parseMsg(connTo, nbin, space, ackId, payload);

    std::string sioId;
    auto self = eioConn->getSio();
    success &= SioServer::serverInstance().connectToSpace(space, self, sioId);

    if (success) {
        std::string encoded;

        encoded = "0";
        if (space != "/") {  // encode namespace
            encoded += space + ",";
        }
        encoded += "{\"sid\":\"" + sioId + "\"}";

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
