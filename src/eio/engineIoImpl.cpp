#include <iostream>
#include "oatpp/json/Serializer.hpp"

#include "oatpp_sio/eio/impl/engineIoImpl.hpp"

#include "oatpp_sio/eio/wsConnection.hpp"

#include "oatpp_sio/util.hpp"

using namespace std;

using namespace oatpp_sio::eio;

Engine* oatpp_sio::eio::theEngine = new EngineImpl();

EngineImpl::EngineImpl() {}
EngineImpl::~EngineImpl() {}

/*******************************
 * *****************************
 * *****************************
 * *****************************
 * *****************************
 * *****************************
 * *****************************
 */

std::string EngineImpl::startConnection(
    const std::shared_ptr<protocol::http::incoming::Request> request,
    bool testSioLayer)
{
    std::string sid = generateSid();
    auto conn = std::make_shared<EioConnection>(*this, sid);
    connections.emplace(sid, conn);

    if (!testSioLayer) {
        conn->setSio(std::make_shared<oatpp_sio::sio::SioAdapter>(sid), conn);
    }

    auto dto = EioStartup::createShared();
    dto->sid = sid;
    dto->upgrades->push_back("websocket");
    // dto->upgrades->push_back("polling");
    dto->pingInterval = pingInterval;
    dto->pingTimeout = pingTimeout;
    dto->maxPayload = maxPayload;

    oatpp::String s = om->writeToString(dto);
    const std::string& ss = *s;

    OATPP_LOGd("EIO", "EngineIoController: startConnection {}", s);

    return pktEncode(eioOpen, ss);
}

void EngineImpl::removeConnection(std::string& sid)
{
    auto iter = connections.find(sid);
    if (iter != connections.end()) {
        // iter->second.close();
        connections.erase(iter);
    }
}

EioConnection::Ptr EngineImpl::getConnection(const std::string& sid)
{
    auto iter = connections.find(sid);
    if (iter != connections.end()) {
        return iter->second;
    }
    return nullptr;
}

std::shared_ptr<WSConnection> EngineImpl::getWsConn(
    const std::string& sid) const
{
    auto iter = connections.find(sid);
    if (iter != connections.end()) {
        // iter->second.close();
        auto eioConn = iter->second;
        std::shared_ptr<WSConnection> ws = eioConn->getWsConn();
        return ws;
    }
    return nullptr;
}

void EngineImpl::printSockets() const
{
    auto iter = connections.begin();
    cout << "EI STORE #entries " << connections.size() << endl;
    while (iter != connections.end()) {
        EioConnectionPtr conn = iter->second;
        cout << "EI STORE ";
        conn->print();
        iter++;
    }
}

Engine::ResponsePtr EngineImpl::startLpConnection(
    const oatpp::web::server::api::ApiController* controller,
    const std::shared_ptr<oatpp::web::protocol::http::incoming::Request> req,
    bool testSioLayer)

{
    std::string sid = generateSid();
    auto conn = std::make_shared<EioConnection>(*this, sid);
    connections.emplace(sid, conn);

    if (testSioLayer) {
        conn->setSpace(theSpace);
        theSpace->subscribe(sid, conn);
    } else {
        conn->setSio(std::make_shared<oatpp_sio::sio::SioAdapter>(sid), conn);
    }

    auto dto = EioStartup::createShared();
    dto->sid = sid;
    dto->upgrades->push_back("websocket");
    dto->pingInterval = pingInterval;
    dto->pingTimeout = pingTimeout;
    dto->maxPayload = maxPayload;

    oatpp::String s = om->writeToString(dto);
    const std::string& ss = *s;

    OATPP_LOGd("EIO", "EngineIoController: startConnection {}", s);

    std::string msg = pktEncode(eioOpen, ss);

    cout << "MP------------\\" << endl;
    theSpace->printSubscribers();
    cout << "MP------------/" << endl;

    auto response = controller->createResponse(Status::CODE_200, msg);
    response->putHeader("Content-Type", "text/plain");
    return response;
}

Engine::ResponsePtr EngineImpl::handleSocketMsg(
    oatpp::web::server::api::ApiController* controller,
    const std::shared_ptr<oatpp::web::protocol::http::incoming::Request> req,
    const std::string& sid)
{
    auto response = controller->createResponse(Status::CODE_200, "");
    response->putHeader("Content-Type", "text/plain");
    return response;
}

std::string EngineImpl::pktEncode(EioPacketType pkt, const std::string& msg)
{
    std::string packet(msg.size() + 1,
                       (char)(pkt));  // init with packet type
    int len = msg.size();
    memcpy(packet.data() + 1, msg.data(), len);
    return packet;
}

std::string EngineImpl::generateSid()
{
    std::string sid = generateRandomString(12);

    return "sid_" + sid;
}

void EngineImpl::registerConnection(std::shared_ptr<WSConnection> wsConn,
                                    bool testSioLayer)
{
    OATPP_LOGd("EIO", "EngineIoController: registerConnection");
    // no sid. start new.
    std::string sid = generateSid();
    auto conn = std::make_shared<EioConnection>(*this, sid);
    connections.emplace(sid, conn);

    auto dto = EioStartup::createShared();
    dto->sid = sid;
    // no upgrades, as we've upgraded already
    // dto->upgrades->push_back("websocket");
    dto->pingInterval = pingInterval;
    dto->pingTimeout = pingTimeout;
    dto->maxPayload = maxPayload;

    oatpp::String s = om->writeToString(dto);

    const std::string& ss = *s;

    OATPP_LOGd("EIO", "EngineIoController: registerConnection {}", s);

    if (testSioLayer) {
        theSpace->subscribe(sid, conn);
        conn->setSpace(theSpace);
    } else {
        conn->setSio(std::make_shared<oatpp_sio::sio::SioAdapter>(sid), conn);
    }
    wsConn->setMessageReceiver(conn);

    std::string msg = pktEncode(eioOpen, ss);

    wsConn->sendMessageAsync(msg);

    conn->upgrade(wsConn, true);
}
