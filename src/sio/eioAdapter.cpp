#include "oatpp_sio/sio/eioAdapter.hpp"

using namespace oatpp_sio::sio;

EioAdapter::EioAdapter(): universe(NameSpaces::spaces()), engine(*oatpp_sio::eio::theEngine)
{

//    std::shared_ptr<MessagePool> getSpace() const { return theSpace; }
}
