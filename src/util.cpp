#include "oatpp_sio/util.hpp"


static const char chars[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
//    "_+!?^*"
    ;
static int arrayLen = sizeof(chars) - 1;

std::string generateRandomString(int len)
{
    std::string sid(len, 'S');

    for (int i = 0; i < len; i++) {
        sid[i] = chars[rand() % arrayLen];
    }
    return sid;
}