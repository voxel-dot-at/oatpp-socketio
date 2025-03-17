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
}

EioConnection::~EioConnection()
{
    if (dbg) cout << "EICO:~EICO " << sid << "  " << this << endl;
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
        OATPP_LOGd("EICO", "{} handleMessage state {} ws {} msg {}", sid,
                   (char)getState(), wsConn.get() != nullptr, msg->body);
    if (getState() == connClosed) {
        if (dbg)
            OATPP_LOGw("EICO",
                       "UNSUB - should not happen!");  // we don't come here!
        if (space.get()) {
            space->unsubscribe(sid);
        }
        if (sioFunnel.get()) {
            sioFunnel->shutdown();
        }
        return;
    }
    if (wsConn.get()) {
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
    OATPP_LOGw("EICO", "{} upgrade()??? webSockOnly {} ", sid, webSockOnly);
    wsConn = ws;
    // get the shared pointer of self:
    wsConn->setMessageReceiver(theEngine->getConnection(sid));

    if (webSockOnly) {
        connType = websocket;
        state = connWebSocket;
    } else {
        // in the middle of the upgrade process
        state = connUpgrading;
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
        if (sioFunnel.get()) {
            OATPP_LOGd("EICO", "{} handleMessageFromWs msg {} {} FUNNEL!", sid,
                       msgs.size(), body);
            sioFunnel->onEioMessage(msg);
        } else {
            OATPP_LOGd("EICO", "{} handleMessageFromWs msg {} {} SPACY!", sid,
                       msgs.size(), body);
            space->publish(msg);
        }
        return true;
    }

    handleEioMessage(body, false);
    return true;
}

void EioConnection::handleEioMessage(const std::string& body, bool isBinary)
{
    char pType = body[0];

    // if (dbg)
    //     OATPP_LOGd("EICO", "{} handleEioMessage |{}| {} {} st {} pt {}", sid,
    //                body, body.size(), isBinary, (int)state, pType);

    switch (pType) {
        case eioClose:
            if (dbg) OATPP_LOGd("EICO", "{} handleEioMessage close", sid);
            shutdownConnection();
            break;
        case eioPing:
            if (state == connUpgrading) {
                if (body == "2probe") {
                    if (dbg)
                        OATPP_LOGd("EICO", "{} handleEioMessage ping probe! :D",
                                   sid);
                    this->wsConn->sendMessageAsync(pktEncode(eioPong, "probe"),
                                                   false);
                }
                checkUpgradeTimeout();
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
            OATPP_LOGd("EICO", "{} handleEioMessage upgrade #MSGS {} LP? {}",
                       sid, msgs.size(), longPollRequest.get() != nullptr);

            for (unsigned int i = 0; i < msgs.size(); i++) {
                OATPP_LOGd("EICO", "{} handleEioMessage upgrade MSG Q {} {}", i,
                           msgs[i]);
            }
            if (longPollRequest
                    .get()) {  // clear pending request by sending a nop
                longPollRequest.reset();
                enqMsg(pktEncode(eioNoop, ""));
            }
            connType = websocket;
            state = connWebSocket;

            this->wsConn->sendMessageAsync(deqMsg(), false);

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
            if (sioFunnel.get()) {
                sioFunnel->onEioMessage(msg);
            } else {
                space->publish(msg);
            }
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
    if (!msgs.size()) {
        return "";
    }
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
    if (state == connUpgrading) {
        return;
    }
    {
        oatpp::async::LockGuard guard(&lpLock);
        msgs.push_back(msg);
    }
    if (dbg)
        OATPP_LOGd("EICO", "{} ENQ MSG {} s {} |{}|", sid, msgs.size(),
                   state == connUpgrading, msg);
    if (state != connUpgrading) {
        lpSema.notifyAll();
    }
}

void EioConnection::sendPingAsync()
{
    if (getState() == connWebSocket) {
        // assert wsConn
        if (wsConn.get()) {
            wsConn->sendMessageAsync(pktEncode(eioPing, ""), false);
        }
    } else {
        enqMsg(pktEncode(eioPing, ""));
    }
}

void EioConnection::checkUpgradeTimeout()
{
    class TimeoutCoRo : public oatpp::async::Coroutine<TimeoutCoRo>
    {
       private:
        // EioConnection::Ptr conn;
        std::string sid;
        unsigned long to;

       public:
        TimeoutCoRo(const std::string& sid, unsigned long timeout)
            : sid(sid), to(timeout)
        {
        }

        Action act() override
        {
            OATPP_LOGd("EICO:TO", "{} TO....", sid);
            auto conn = theEngine->getConnection(sid);
            if (!conn) {
                OATPP_LOGw("EICO:TO", "{} no conn found - ignoring....", sid);
                finish();
            }
            if (conn->getState() == connClosed) {
                if (dbg)
                    OATPP_LOGd("EICO:TO", "{} conn was closed, leaving", sid);
                return finish();
            }
            if (conn->getState() != connWebSocket) {
                if (dbg)
                    OATPP_LOGi("EICO:TO",
                               "{} Conn not upgraded! s={} - closing!", sid,
                               conn->getState());
                conn->shutdownConnection();
                return finish();
            } else {
                // upgraded. stop.
                return finish();
            }
        }
    };

    async->execute<TimeoutCoRo>(sid, engine.pingTimeout);
}

void EioConnection::checkPongTimeout(EioConnection::Ptr conn)
{
    class TimeoutCoRo : public oatpp::async::Coroutine<TimeoutCoRo>
    {
       private:
        std::string sid;
        EioConnection::Ptr conn;
        unsigned long to;
        bool first = true;

       public:
        TimeoutCoRo(const std::string& sid, EioConnection::Ptr conn,
                    unsigned long timeout)
            : sid(sid), conn(conn), to(timeout)
        {
        }

        Action shutdown()
        {
            OATPP_LOGd("EICO:PO", "{} shutdown", sid);
            conn->shutdownConnection();
            return finish();
        }

        Action act() override
        {
            if (first) {  // perform initial delay
                first = false;
                return waitRepeat(1 * std::chrono::milliseconds(to));
            }
            OATPP_LOGd("EICO:PO", "{} PO # {} ", sid, conn->pongCount);

            if (conn->getState() == connClosed) {
                if (dbg)
                    OATPP_LOGd("EICO:PO", "{} conn was closed, leaving", sid);
                return finish();
            }
            if (!conn->pongCount) {
                if (dbg) OATPP_LOGi("EICO:PO", "{} NO Pong! - closing!", sid);
                conn->enqMsg(conn->pktEncode(eioClose, ""));
                return yieldTo(&TimeoutCoRo::shutdown);
            }
            conn->pongCount = 0;
            return finish();
        }
    };

    async->execute<TimeoutCoRo>(sid, conn, engine.pingTimeout);
}

void EioConnection::scheduleDelayedPingMsg()
{
    class SendDelayedPingCoroutine
        : public oatpp::async::Coroutine<SendDelayedPingCoroutine>
    {
       private:
        std::string sid;
        EioConnection::Ptr conn;
        unsigned long pi, pt;
        bool pingSent = false;
        long count, idle, notYet;
        bool first = true;

       public:
        SendDelayedPingCoroutine(const std::string& sid,
                                 EioConnection::Ptr conn,
                                 unsigned long pingInterval,
                                 unsigned long pingTimeout)
            : sid(sid),
              conn(conn),
              pi(pingInterval),
              pt(pingTimeout),
              count(-1),
              idle(0),
              notYet(0)
        {
        }

        Action act() override
        {
            if (first) {  // perform initial delay
                first = false;
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
            if (conn->getState() == connUpgrading) {
                // skip during upgrade..
                return waitRepeat(std::chrono::milliseconds(pi));
            }

            OATPP_LOGi("EICO:SCHED", "{} ping.. # {}", sid, conn->pongCount);
            // pongCount is reset in the pongtimeout handler
            conn->checkPongTimeout(conn);
            conn->sendPingAsync();

            return waitRepeat(std::chrono::milliseconds(pi));
        }
    };
    EioConnection::Ptr conn = theEngine->getConnection(sid);
    if (!conn.get()) {
        OATPP_LOGi("EICO:PING", "NO CONN YET?!? Try next time....");
    }
    OATPP_LOGd("EICO:SCHED", "{} START", sid);

    async->execute<SendDelayedPingCoroutine>(sid, conn, engine.pingInterval,
                                             engine.pingTimeout);
}