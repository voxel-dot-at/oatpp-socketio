#pragma once

#include <string>
#include <memory>
#include <vector>

// timer functionality...:
#include "oatpp/async/Lock.hpp"
#include "oatpp/async/ConditionVariable.hpp"

#include "oatpp/async/Executor.hpp"

#include "oatpp/macro/component.hpp"

#include "oatpp_sio/eio/engineIo.hpp"
#include "oatpp_sio/eio/messageReceiver.hpp"
#include "oatpp_sio/eio/messagePool.hpp"

class WSConnection;
typedef std::shared_ptr<WSConnection> WsConnPtr;

namespace oatpp_sio {
namespace eio {

typedef enum
{
    eioOpen = '0',
    eioClose,
    eioPing,
    eioPong,
    eioMessage,
    eiouUgrade,
    eioNoop,
    eioBinary = 'b'
} EioPacketType;

typedef enum
{
    connInit = '0',
    connOpening,
    connOpen,
    connClosing,
    connClosed
} ConnState;

typedef enum
{
    polling,
    websocket
} ConnType;

class Engine;
class MessagePool;

class EioConnection : public MessageReceiver, public MessageConsumer
{
    typedef std::shared_ptr<MessagePool> PoolPtr;
    typedef std::shared_ptr<Message> MessagePtr;
    typedef oatpp::web::protocol::http::incoming::Request Request;
    typedef std::shared_ptr<Request> RequestPtr;

    std::string sid;
    std::atomic<ConnState> state;
    Engine& engine;
    unsigned long startTs;
    ConnType connType;

    WsConnPtr wsConn;
    PoolPtr space;

    int pongCount = 0;

    OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, asyncExecutor,
                    "ws");

    ////// long-polling section:

    RequestPtr longPollRequest;

    // lock for the message queue:
    oatpp::async::Lock lpLock;
    oatpp::async::ConditionVariable lpSema;

    std::vector<std::string> msgs;  //< msg queue, already type-encoded!

   public:
    EioConnection(Engine& engine, std::string sid);

    virtual ~EioConnection();

    EioConnection(const EioConnection&) = delete;
    EioConnection& operator=(const EioConnection&) = delete;

    // upgrade to ws connectionl if webSockOnly, it is a 'clean' websocket without upgrade
    void upgrade(WsConnPtr ws, bool webSockOnly = false);

    const std::string& getSid() const { return sid; }
    const int getPongCount() const { return pongCount; }
    ConnState getState() const { return state.load(); }

    void wsHasClosed();

    void setSpace(PoolPtr deep) { space = deep; }

    WsConnPtr getWsConn() const { return wsConn; }

    bool hasWebsocket() const { return connType == websocket; }

    bool hasMsgs() const;
    bool hasLongPoll() const { return longPollRequest.get() != nullptr; }
    void setLongPoll(RequestPtr r) { longPollRequest = r; }
    void clearLongPoll() { longPollRequest.reset(); }
    void injectNoop() { msgs.push_back(pktEncode(eioNoop, "")); }
    void injectClose() { msgs.push_back(pktEncode(eioClose, "")); }

    // from long-poll:
    virtual void handleLpPostMessage(const std::string& body);

    // from websocket
    virtual bool handleMessageFromWs(const std::string& body,
                                     bool isBinary = false);

    // from our space
    virtual void handleMessage(std::shared_ptr<Message> msg);

    typedef std::shared_ptr<EioConnection> Ptr;

    std::string pktEncode(EioPacketType pkt, const std::string& msg);

    // unsubscribe from space, close wsconn, remove sid from engine
    void shutdownConnection();

    virtual void print() const override;

    // dequeue messages, return encoded string for long-polling
    std::string deqMsg();

    void enqMsg(const std::string& msg);

   private:
    void handleEioMessage(const std::string& body, bool isBinary);

    void scheduleDelayedPingMsg();

    // send a pong message either to the message queue or to websocket
    void sendPongAsync();
};

}  // namespace eio
}  // namespace oatpp_sio