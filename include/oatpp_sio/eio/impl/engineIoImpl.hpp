#pragma once
#include <map>

#include "oatpp/async/Lock.hpp"
#include "oatpp/async/Executor.hpp"

#include "oatpp/json/ObjectMapper.hpp"
#include "oatpp/macro/component.hpp"

#include "oatpp_sio/eio/engineIo.hpp"
#include "oatpp_sio/eio/connection.hpp"

#include "oatpp_sio/webapi/dto/eioDto.hpp"

using namespace oatpp;
using namespace oatpp::web;

namespace oatpp_sio {
namespace eio {

class EngineImpl : public Engine
{
    typedef oatpp::web::protocol::http::Status Status;
    typedef oatpp::async::Action Action;

    // executor for timer tasklets

    std::shared_ptr<oatpp::json::ObjectMapper> om =
        std::make_shared<oatpp::json::ObjectMapper>();

    std::map<std::string, EioConnection::Ptr> connections;

   public:
    EngineImpl();
    virtual ~EngineImpl();

    virtual ResponsePtr startLpConnection(
        const oatpp::web::server::api::ApiController* controller,
        const std::shared_ptr<oatpp::web::protocol::http::incoming::Request>
            req,
        bool testSioLayer = false) override;

    virtual std::string startConnection(
        const std::shared_ptr<oatpp::web::protocol::http::incoming::Request>
            req,
        bool testSioLayer = false) override;

    /** start websocket-based connection */
    virtual void registerConnection(std::shared_ptr<WSConnection> wsConn,
                                    bool testSioLayer = false);

    virtual void removeConnection(std::string& sid) override;

    bool checkHeaders(const std::shared_ptr<Request> req) { return true; }

    virtual EioConnection::Ptr getConnection(const std::string& sid) override;

    virtual EioConnection::Ptr getConnection(const WebsocketPtr& socket) override;

    virtual std::shared_ptr<WSConnection> getWsConn(
        const std::string& sid) const override;

    std::string generateSid();

    /** debugging: print known sockets */
    void printSockets() const;

   private:
    std::string pktEncode(EioPacketType pkt, const std::string& msg);

    // virtual ResponsePtr handleStartConnection(
    //     oatpp::web::server::api::ApiController* controller,
    //     const std::shared_ptr<Request> req);

    virtual ResponsePtr handleSocketMsg(
        oatpp::web::server::api::ApiController* controller,
        const std::shared_ptr<Request> req, const std::string& sid);

    // void pingAsync(EioConnection::Ptr conn);
};

}  // namespace eio
}  // namespace oatpp_sio
