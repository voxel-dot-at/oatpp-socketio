#pragma once

#include "oatpp/macro/codegen.hpp"
#include "oatpp/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)  ///< Begin DTO codegen section

class ServerInfo : public oatpp::DTO
{
    DTO_INIT(SpaceInfo, DTO /* extends */)

    DTO_FIELD(String, name);

    DTO_FIELD(List<String>, spaces) = {};
};

class SpaceInfo : public oatpp::DTO
{
    DTO_INIT(SpaceInfo, DTO /* extends */)

    DTO_FIELD(String, name);

    DTO_FIELD(List<String>, listeners) = {};
};

#include OATPP_CODEGEN_END(DTO)  ///< End DTO codegen section