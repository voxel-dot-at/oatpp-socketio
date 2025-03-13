/*
   Copyright 2012-2025 Simon Vogl <svogl@voxel.at> VoXel Interaction Design - www.voxel.at

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once

#include <string>
#include <memory>

#include "oatpp/async/Coroutine.hpp"

#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"

#include "oatpp/web/server/api/ApiController.hpp"

#include "oatpp-websocket/Handshaker.hpp"

#include "oatpp_sio/eio/engineIo.hpp"


#include OATPP_CODEGEN_BEGIN(ApiController)  /// <-- Begin Code-Gen

// static bool debugSync = false;

class EngineIoController : public oatpp::web::server::api::ApiController
{
   private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>,
                    websocketConnectionHandler, "websocket");

   private:
    typedef EngineIoController __ControllerType;

    std::shared_ptr<oatpp::data::mapping::ObjectMapper> om;

   public:
    EngineIoController(OATPP_COMPONENT(
        std::shared_ptr<oatpp::web::mime::ContentMappers>, apiContentMappers))
        : oatpp::web::server::api::ApiController(apiContentMappers)
    {
        om = apiContentMappers->getDefaultMapper();
        OATPP_LOGd("EIO", "EngineIoController: mapper serializing to {}/{}",
                   om->getInfo().mimeType, om->getInfo().mimeSubtype);
    }

   public:  // ENDPOINTS:
            // GET - connection startup
    ENDPOINT_INFO(EioGet)
    {
        info->summary = "Engine.IO";
        info->description = "Engine.IO default endpoint";
        info->addResponse<String>(Status::CODE_200, "text/plain");
        info->queryParams.add<String>("EIO").description =
            "protocol version (4)";
        info->queryParams.add<String>("transport").description =
            "transport method (polling)";
    }
    ENDPOINT_ASYNC("GET", "/engine.io/*", EioGet)
    {
        ENDPOINT_ASYNC_INIT(EioGet);

        oatpp_sio::eio::EioConnection::Ptr conn;

        Action handleWebSocket()
        {
            auto sid = request->getQueryParameter("sid");
            // immediately start a ws connection, sending the open packet over ws
            if (!sid) {
                // the ws / engine handling is done in the websocketConnectionHandler
                auto response =
                    oatpp::websocket::Handshaker::serversideHandshake(
                        request->getHeaders(),
                        controller->websocketConnectionHandler);
                return _return(response);
            } else {
                OATPP_LOGd("EIO", "EioGet {} WS UPGRADE", sid);

                auto parameters = std::make_shared<
                    oatpp::network::ConnectionHandler::ParameterMap>();
                (*parameters)["sid"] = sid;

                auto response =
                    oatpp::websocket::Handshaker::serversideHandshake(
                        request->getHeaders(),
                        controller->websocketConnectionHandler);
                response->setConnectionUpgradeParameters(parameters);

                return _return(response);
            }
        }

        Action handleLongPoll()
        {
            using namespace oatpp_sio::eio;
            auto sid = request->getQueryParameter("sid");
            OATPP_LOGd("EIO", "EioGet {} handleLp", sid);

            if (!sid) {
                OATPP_LOGd("EIO", "EioGet {} handleLp START", sid);
                // create a new connection
                auto response =
                    theEngine->startLpConnection(controller, request);
                return _return(response);
            }
            // new
            conn = theEngine->getConnection(sid);
            if (!conn) {
                // stale connection id
                auto response =
                    controller->createResponse(Status::CODE_400, "no sid");
                return _return(response);
            }

            if (conn->hasWebsocket()) {
                // already upgraded to websocket. deny access.
                auto response = controller->createResponse(Status::CODE_400);
                return _return(response);
            }
            if (conn->getState() == connOpening) {
                OATPP_LOGw("EIO", "EioGet {} handleLp IN UPGRADE", sid);
                return yieldTo(&SioGet::wait4Msgs);
            }
            if (conn->hasLongPoll()) {
                // another poll request pending. kick.
                OATPP_LOGw("EIO", "EioGet {} handleLp DUP REQ", sid);
                std::string ssid = sid;
                conn->injectClose();
                theEngine->removeConnection(ssid);
                auto response =
                    controller->createResponse(Status::CODE_400, "dup req");
                return _return(response);
            } else {
                conn->setLongPoll(request);
            }
            return yieldTo(&EioGet::wait4Msgs);
        }

        Action wait4Msgs()
        {
            using namespace oatpp_sio::eio;
            auto sid = request->getQueryParameter("sid");

            // assert conn is set...
            if (conn->hasMsgs()) {
                auto msg = conn->deqMsg();

                conn->clearLongPoll();

                OATPP_LOGd("CTRL", "EioGet {} wait4Msgs GOT [{}]", sid, msg);
                auto response =
                    controller->createResponse(Status::CODE_200, msg);
                response->putHeader("Content-Type", "text/plain");
                return _return(response);
            }
            if (conn->getState() == connClosed) {
                OATPP_LOGd("CTRL", "EioGet {} wait4Msgs connClosed", sid);
                return _return(
                    controller->createResponse(Status::CODE_400, "closed"));
            }
            // OATPP_LOGd("CTRL", "EioGet {} wait4Msgs...", sid);
            return oatpp::async::Action::createWaitRepeatAction(
                100 * 1000 + oatpp::Environment::getMicroTickCount());
        }

        Action act() override
        {
            auto eio = request->getQueryParameter("EIO");
            auto transport = request->getQueryParameter("transport");
            // auto sid = request->getQueryParameter("sid");
            // OATPP_LOGd("EIO", "EioGet {} GET {} t {}", sid, eio, transport);

            // check pre-conditions:
            if (!eio || eio != "4") {
                auto response = controller->createResponse(Status::CODE_400);
                return _return(response);
            }

            if (!transport ||
                (transport != "polling" && transport != "websocket")) {
                return _return(controller->createResponse(Status::CODE_400));
            }

            if (transport == "websocket") {
                return yieldTo(&EioGet::handleWebSocket);
            } else {
                return yieldTo(&EioGet::handleLongPoll);
            }
        }
    };

    ENDPOINT_ASYNC("POST", "/engine.io/*", EioPost)
    {
        ENDPOINT_ASYNC_INIT(EioPost);

        oatpp_sio::eio::EioConnection::Ptr conn;

        Action withBody(const String& body)
        {
            // assert conn is set!

            if (!body->size()) {
                auto response =
                    controller->createResponse(Status::CODE_400, "no body");
                return _return(response);
            }

            conn->handleLpPostMessage(body);

            auto response = controller->createResponse(Status::CODE_200, "ok");
            return _return(response);
        }

        Action act() override
        {
            using namespace oatpp_sio::eio;

            auto eio = request->getQueryParameter("EIO");
            auto transport = request->getQueryParameter("transport");
            auto sid = request->getQueryParameter("sid");

            OATPP_LOGd("EIO", "EioPost {} POST {} t {}", sid, eio, transport);

            if (!eio || eio != "4" || !sid) {
                return _return(controller->createResponse(Status::CODE_400));
            }

            if (!transport ||
                (transport != "polling" && transport != "websocket")) {
                return _return(controller->createResponse(Status::CODE_400));
            }

            conn = theEngine->getConnection(sid);
            if (!conn) {
                return _return(controller->createResponse(Status::CODE_400,
                                                          "sid not found"));
            }
            if (conn->hasLongPoll()) {
                OATPP_LOGw("EIO", "EioPost {} DUP REQ", sid);
                std::string ssid = sid;
                // duplicate poll request
                conn->injectClose();
                theEngine->removeConnection(ssid);
                auto response = controller->createResponse(Status::CODE_400,
                                                           "duplicate get req");
                return _return(response);
            }
            OATPP_LOGd("EIO", "EioPost {} POST ok {} t {} s {}", sid, eio,
                       transport);

            return request->readBodyToStringAsync().callbackTo(
                &EioPost::withBody);
        }
    };

    // make the test-suite happy (otherwise we report 404)
    ENDPOINT_ASYNC("PUT", "/engine.io/*", EngineIoPut)
    {
        ENDPOINT_ASYNC_INIT(EngineIoPut);

        Action act() override
        {
            auto response = controller->createResponse(Status::CODE_400);
            return _return(response);
        }
    };
};

#include OATPP_CODEGEN_END(ApiController)  /// <-- End Code-Gen
