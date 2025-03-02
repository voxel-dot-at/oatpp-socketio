#include <iostream>

#include "oatpp_sio/eio/messagePool.hpp"
#include "oatpp_sio/eio/connection.hpp"

using namespace std;
using namespace oatpp_sio::eio;

void MessagePool::subscribe(const std::string& id, MessageConsumerPtr conn)
{  // todo lock connections
    std::lock_guard<std::mutex> guard(connLock);
    connections.emplace(id, conn);
};

void MessagePool::unsubscribe(const std::string& id)
{
    // std::lock_guard<std::mutex> guard(connLock);
    // todo lock connections
    auto iter = connections.find(id);
    if (iter != connections.end()) {
        connections.erase(iter);
        cout << "MP::unsub REMOVED " << id << endl;
    } else {
        cout << "MP::unsub COULD NOT FIND " << id << endl;
        printSubscribers();
    }
}

void MessagePool::publish(const std::string& message)
{
    std::shared_ptr<Message> msg = std::make_shared<Message>();
    msg->body = message;
    publish(msg);
}

void MessagePool::publish(std::shared_ptr<Message> msg)
{
    // std::lock_guard<std::mutex> guard(connLock);
    // todo lock connections, parallelize
    auto iter = connections.begin();
    while (iter != connections.end()) {
        MessageConsumerPtr cons = iter->second;
        cons->handleMessage(msg);
        iter++;
    }
}

void MessagePool::printSubscribers() const
{
    auto iter = connections.begin();
    cout << "MP STORE #entries " << connections.size() << endl;
    while (iter != connections.end()) {
        MessageConsumerPtr cons = iter->second;
        cout << "MP STORE U=" << cons.use_count() << " ";
        cons->print();
        iter++;
    }
}
