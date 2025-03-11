#pragma once

#include <string>
#include <memory>

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/async/Coroutine.hpp"

#include "oatpp-websocket/AsyncWebSocket.hpp"

#include "oatpp_sio/eio/messagePool.hpp"

class WSConnection;

namespace oatpp_sio {
namespace eio {

class EioConnection;
class MessagePool;

class Engine
{
   protected:
    typedef std::shared_ptr<EioConnection> EioConnectionPtr;
    typedef oatpp::websocket::AsyncWebSocket AsyncWebSocket;
    typedef oatpp::web::server::api::ApiController ApiController;
    typedef oatpp::web::protocol::http::incoming::Request Request;
    typedef std::shared_ptr<ApiController::OutgoingResponse> ResponsePtr;
    typedef std::shared_ptr<oatpp_sio::eio::MessagePool> PoolPtr;
    
    std::shared_ptr<oatpp_sio::eio::MessagePool> theSpace;

   public:
    unsigned int pingInterval = 300*1000;
    unsigned int pingTimeout = 200*1000;
    unsigned int maxPayload = 1e6;

    // todo: list of known connections here?

   public:  // convenience typedefs
    // typedef std::shared_ptr<oatpp_sio::eio::MessagePool> PoolPtr;

   public:
    Engine() : theSpace(std::make_shared<MessagePool>()) {}
    virtual ~Engine() {}

    /** start a long-polling connection - assingns a sid and registers a connection onject.
     * @param controller
     * @param req
     * @param testSioLayer - use testing top-level layer or socket.io connector (the standard case)
     */
    virtual ResponsePtr startLpConnection(
        const oatpp::web::server::api::ApiController* controller,
        const std::shared_ptr<oatpp::web::protocol::http::incoming::Request>
            req, bool testSioLayer = false) = 0;

    // websocket
    virtual std::string startConnection(const std::shared_ptr<Request> req, bool testSioLayer = false) = 0;

    virtual void removeConnection(std::string& sid) = 0;

    virtual EioConnectionPtr getConnection(const std::string& sid) = 0;

    virtual std::shared_ptr<WSConnection> getWsConn(
        const std::string& sid) const = 0;

    void setSpace(PoolPtr spc) { theSpace = spc; }

    PoolPtr getSpace() const { return theSpace; }
};

extern Engine* theEngine;

}  // namespace eio
}  // namespace oatpp_sio