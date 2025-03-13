#include <iostream>
#include <sstream>

#include <oatpp/encoding/Base64.hpp>

#include "oatpp_sio/eio/connection.hpp"
#include "oatpp_sio/eio/wsConnection.hpp"
#include "oatpp_sio/eio/engineIo.hpp"

#include "oatpp_sio/sio/sioConnector.hpp"

using namespace oatpp_sio::eio;
using namespace std;

static const bool dbg = true;

static const char SEPARATOR = '\x1e';  // ASCII RS

static oatpp::encoding::Base64 scrambler;

EioConnection::EioConnection(Engine& engine, std::string sid)
    : sid(sid), state(connInit), engine(engine), connType(polling)
{
    if (dbg) cout << "EICO::EICO " << sid << "  " << this << endl;
    scheduleDelayedPingMsg();
}

EioConnection::~EioConnection()
{
    if (dbg) cout << "EICO:~EICO " << sid << "  " << this << endl;
    // space->unsubscribe(sid);
    // engine.removeConnection(sid);
}

void EioConnection::print() const
{
    if (dbg)
        cout << "EICO " << sid << " " << (char)getState() << " " << getWsConn()
             << endl;
}

void EioConnection::setSio(SioAdapterPtr adapter, Ptr self)
{
    sioFunnel = adapter;
    adapter->eioConn = self;
}

std::string EioConnection::pktEncode(EioPacketType pkt, const std::string& msg)
{
    std::string packet(msg.size() + 1,
                       (char)(pkt));  // init with packet type
    int len = msg.size();
    memcpy(packet.data() + 1, msg.data(), len);
    return packet;
}

// from pool:
void EioConnection::handleMessage(std::shared_ptr<Message> msg)
{
    if (dbg)
        OATPP_LOGd("EICO", "{} handleMessage state {} msg {} {}", sid,
                   (char)getState(), msgs.size(), msg->body);
    if (getState() == connClosed) {
        if (dbg) OATPP_LOGd("EICO", "UNSUB");  // we don't come here!
        if (space.get()) {
            space->unsubscribe(sid);
        }
        return;
    }
    if (this->wsConn.get()) {
        if (msg->binary) {
            this->wsConn->sendMessageAsync(msg->body, msg->binary);
        } else {
            this->wsConn->sendMessageAsync(pktEncode(eioMessage, msg->body),
                                           msg->binary);
        }
        return;
    }
    // else long-polling:
    if (msg->binary) {
        auto bin = scrambler.encode(msg->body);
        enqMsg(pktEncode(eioBinary, bin));
    } else {
        enqMsg(pktEncode(eioMessage, msg->body));
    }
}

void EioConnection::upgrade(WsConnPtr ws, bool webSockOnly)
{
    wsConn = ws;
    // get the shared pointer of self:
    wsConn->setMessageReceiver(theEngine->getConnection(sid));

    if (webSockOnly) {
        connType = websocket;
        state = connOpen;
    } else {
        // in the middle of the upgrade process
        state = connOpening;
    }
}

bool EioConnection::hasMsgs() const { return msgs.size() > 0; }

bool EioConnection::handleMessageFromWs(const std::string& body, bool isBinary)
{
    // OATPP_LOGd("EICO", "{} handleMessageFromWs msg {} {}", sid, msgs.size(),
    //            body);

    if (isBinary) {
        auto msg = std::make_shared<Message>();
        msg->body = body;
        msg->binary = isBinary;
        space->publish(msg);
        return true;
    }

    handleEioMessage(body, false);
    return true;
}

void EioConnection::handleEioMessage(const std::string& body, bool isBinary)
{
    char pType = body[0];

    if (dbg)
        OATPP_LOGd("EICO", "{} handleEioMessage |{}| {} {}", sid, body,
                   body.size(), isBinary);

    switch (pType) {
        case eioClose:
            if (dbg) OATPP_LOGd("EICO", "{} handleEioMessage close", sid);
            shutdownConnection();
            break;
        case eioPing:
            if (dbg) OATPP_LOGd("EICO", "{} handleEioMessage ping", sid);
            if (state == connOpening) {
                if (body == "2probe") {
                    if (dbg)
                        OATPP_LOGd("EICO", "{} handleEioMessage ping probe! :D",
                                   sid);
                    this->wsConn->sendMessageAsync(pktEncode(eioPong, "probe"),
                                                   false);
                }
            } else {
                OATPP_LOGw("EICO", "{} handleEioMessage NO PING ALLOWED!", sid);
                shutdownConnection();
            }
            break;
        case eioPong:
            if (dbg) OATPP_LOGd("EICO", "{} handleEioMessage pong", sid);
            pongCount++;
            // reset timer
            break;
        case eiouUgrade:
            if (dbg)
                ;
            OATPP_LOGd("EICO", "{} handleEioMessage upgrade #MSGS {}", sid,
                       msgs.size());
            connType = websocket;
            state = connOpen;
            break;
        case eioMessage: {
            if (dbg) OATPP_LOGd("EICO", "{} handleEioMessage msg", sid);
            // strip off header and forward data:
            auto msg = std::make_shared<Message>();
            msg->body = std::string(body.data() + 1, body.size() - 1);
            msg->binary = false;
            if (sioFunnel.get()) {
                sioFunnel->onEioMessage(msg);
            } else {
                space->publish(msg);
            }
            break;
        }
        case eioBinary: {
            auto bin =
                scrambler.decode(std::string(body.data() + 1, body.size() - 1));
            auto msg = std::make_shared<Message>();
            msg->body = bin;
            msg->binary = true;
            space->publish(msg);
            break;
        }
        default: {
            OATPP_LOGw("EICO", "handleEioMessage unknown type {}", (int)pType);

            // this->wsConn->sendMessageAsync(pktEncode(eioClose, ""), false);

            shutdownConnection();
        }
    }
}

void EioConnection::wsHasClosed()
{
    state = connClosed;
    shutdownConnection();
}

// from long-poll:
void EioConnection::handleLpPostMessage(const std::string& body)
{
    // handleEioMessage(body, false);
    stringstream ss(body);
    string tok;

    while (getline(ss, tok, SEPARATOR)) {
        // cout << "HDL " << tok << endl;
        handleEioMessage(tok, false);
    }
}

// unsubscribe from space, close wsconn, remove sid from engine
void EioConnection::shutdownConnection()
{
    state = connClosed;
    // clear downstream refs
    engine.removeConnection(sid);

    // clear upstream refs
    if (space.get()) {
        space->unsubscribe(sid);
        space.reset();
    }
    if (sioFunnel.get()) {
        sioFunnel->shutdown();
        sioFunnel.reset();
    }
    msgs.clear();

    if (wsConn.get()) {
        wsConn->closeSocketAsync();
        wsConn->wsHasClosed();
        wsConn.reset();
    }
    if (longPollRequest) {
        cout << "SHUT: LPnoti " << endl;
        lpSema.notifyAll();
        longPollRequest.reset();
    }
}

std::string EioConnection::deqMsg()
{
    oatpp::async::LockGuard guard(&lpLock);

    if (dbg) OATPP_LOGd("EICO", "GO {} ", msgs.size());
    stringstream ss;
    auto iter = msgs.begin();

    while (iter != msgs.end()) {
        ss << *iter;
        iter++;
        if (iter != msgs.end()) {
            ss << SEPARATOR;
        }
    }
    msgs.clear();
    return ss.str();
}

void EioConnection::enqMsg(const std::string& msg)
{
    {
        oatpp::async::LockGuard guard(&lpLock);
        msgs.push_back(msg);
    }
    if (dbg) OATPP_LOGd("EICO", "{} ENQ MSG {}", sid, msgs.size());
    lpSema.notifyAll();
}

void EioConnection::sendPongAsync()
{
    if (connType == websocket) {
        // assert wsConn
        if (wsConn.get()) {
            wsConn->sendMessageAsync(pktEncode(eioPing, ""), false);
        }
    } else {
        enqMsg(pktEncode(eioPing, ""));
    }
}

void EioConnection::scheduleDelayedPingMsg()
{
    class SendDelayedPingCoroutine
        : public oatpp::async::Coroutine<SendDelayedPingCoroutine>
    {
       private:
        // EioConnection::Ptr conn;
        std::string sid;
        unsigned long pi, pt;
        bool pingSent = false;
        long count, idle;
        bool first = true;

       public:
        SendDelayedPingCoroutine(const std::string& sid,
                                 unsigned long pingInterval,
                                 unsigned long pingTimeout)
            : sid(sid), pi(pingInterval), pt(pingTimeout), count(-1), idle(0)
        {
        }

        Action act() override
        {
            if (first) {  // perform initial delay
                first = false;
                return waitRepeat(1 * std::chrono::milliseconds(pi));
            }
            auto conn = theEngine->getConnection(sid);
            if (!conn) {
                return waitRepeat(std::chrono::milliseconds(pi));
            }
            int pongs = conn->getPongCount();

            if (conn->getState() == connClosed) {
                if (dbg)
                    OATPP_LOGi("EICO:SCHED",
                               "{} ping state {} p={} conn was closed, leaving",
                               sid, conn->getState(), pongs);
                return finish();
            }
            // todo: add pong timer. instead, we check on the next iteration
            if (pongs - count == 0) {
                // OATPP_LOGd("EICO:SCHED", "{} ping-pong: idle... {} {} {}", sid,
                //            pongs, count, idle);
                if (idle > 0) {
                    if (dbg)
                        OATPP_LOGi("EICO:SCHED",
                                   "{} ping-pong: idle closing! {} {} {}", sid,
                                   pongs, count, idle);
                    // no need to send close, the connection is stale already
                    conn->shutdownConnection();
                    return finish();
                }
                idle++;
            } else {
                idle = 0;
            }
            count = pongs;

            conn->sendPongAsync();

            return waitRepeat(std::chrono::milliseconds(pi));
        }
    };
    // MAYBE refactor to getConnection() inside the timer, need to check existence...
    async->execute<SendDelayedPingCoroutine>(sid, engine.pingInterval,
                                             engine.pingTimeout);
}