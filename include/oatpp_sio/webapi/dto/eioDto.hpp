#pragma once

#include "oatpp/macro/codegen.hpp"
#include "oatpp/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)  ///< Begin DTO codegen section

class EioStartup : public oatpp::DTO
{
    DTO_INIT(EioStartup, DTO /* extends */)

    DTO_FIELD(String, sid);

    DTO_FIELD(List<String>, upgrades) = {};

    DTO_FIELD(Int32, pingInterval) = 25000;
    DTO_FIELD(Int32, pingTimeout) = 20000;
    DTO_FIELD(Int32, maxPayload) = 128 * 1024;

    //   {
    //     "sid": "lv_VI97HAXpY6yYWAAAC",
    //     "upgrades": ["websocket"],
    //     "pingInterval": 25000,
    //     "pingTimeout": 20000,
    //     "maxPayload": 1000000
    //   }
};

#include OATPP_CODEGEN_END(DTO)  ///< End DTO codegen section