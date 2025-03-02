#pragma once

#include "oatpp/async/Lock.hpp"
#include "oatpp/async/Executor.hpp"

#include "oatpp/macro/component.hpp"

#include "oatpp-websocket/AsyncConnectionHandler.hpp"
#include "oatpp-websocket/AsyncWebSocket.hpp"

#include "oatpp_sio/eio/messageReceiver.hpp"

/**
 * WebSocket listener listens on incoming WebSocket events.
 */
class WSConnection : public oatpp::websocket::AsyncWebSocket::Listener
{
   private:
    static constexpr const char* TAG = "WSCO";

   private:
    /**
   * Buffer for messages. Needed for multi-frame messages.
   */
    oatpp::data::stream::BufferOutputStream m_messageBuffer;

    /**
   * Lock for synchronization of writes to the web socket.
   */
    oatpp::async::Lock wLock;

   private:
    /* Inject application components */

    OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, m_asyncExecutor,
                    "ws");

   private:
    std::shared_ptr<AsyncWebSocket> m_socket;
    v_int64 m_peerId;

   private:
    std::shared_ptr<oatpp_sio::eio::MessageReceiver> recv;

   public:
    WSConnection(const std::shared_ptr<AsyncWebSocket>& socket, v_int64 peerId)
        : m_socket(socket), m_peerId(peerId)  //, m_pingPoingCounter(0)
    {
    }

    std::shared_ptr<AsyncWebSocket> theSocket() const { return m_socket; }

    void close() { m_socket->sendCloseAsync(); }

    // associate an eio connection with the low-level ws connection
    void setMessageReceiver(
        std::shared_ptr<oatpp_sio::eio::MessageReceiver> recv)
    {
        this->recv = recv;
    }

    /**
   * Called on "ping" frame.
   */
    CoroutineStarter onPing(const std::shared_ptr<AsyncWebSocket>& socket,
                            const oatpp::String& message) override;

    /**
   * Called on "pong" frame
   */
    CoroutineStarter onPong(const std::shared_ptr<AsyncWebSocket>& socket,
                            const oatpp::String& message) override;

    /**
   * Called on "close" frame
   */
    CoroutineStarter onClose(const std::shared_ptr<AsyncWebSocket>& socket,
                             v_uint16 code,
                             const oatpp::String& message) override;

    /**
   * Called on each message frame. After the last message will be called once-again with size == 0 to designate end of the message.
   */
    CoroutineStarter readMessage(const std::shared_ptr<AsyncWebSocket>& socket,
                                 v_uint8 opcode, p_char8 data,
                                 oatpp::v_io_size size) override;

    void closeSocketAsync();

    void sendMessageAsync(const oatpp::String& message, bool isBinary = false);

    // indication that the underlying websocket has closed; remove it
    void wsHasClosed();

    // convenience typedef
    typedef std::shared_ptr<WSConnection> Ptr;

   private:
    // CoroutineStarter closeSock();
    // CoroutineStarter sendMsg();
};

/**
 * Listener on new WebSocket connections.
 */
class WSInstanceListener
    : public oatpp::websocket::AsyncConnectionHandler::SocketInstanceListener
{
   private:
    static constexpr const char* TAG = "WSI";

   public:
    /**
   * Counter for connected clients.
   */
    static std::atomic<v_int32> SOCKETS;

   public:
    /**
   *  Called when socket is created
   */
    void onAfterCreate_NonBlocking(
        const std::shared_ptr<WSConnection::AsyncWebSocket>& socket,
        const std::shared_ptr<const ParameterMap>& params) override;

    /**
   *  Called before socket instance is destroyed.
   */
    void onBeforeDestroy_NonBlocking(
        const std::shared_ptr<WSConnection::AsyncWebSocket>& socket) override;
};
