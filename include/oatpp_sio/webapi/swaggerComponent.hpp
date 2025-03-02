#pragma once
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

#ifndef SwaggerComponent_hpp
#define SwaggerComponent_hpp

#include <string>
#include <utility>

#include "oatpp-swagger/Model.hpp"
#include "oatpp-swagger/Resources.hpp"
#include "oatpp/macro/component.hpp"

/**
 *  Swagger ui is served at
 *  http://host:port/swagger/ui
 */
class SwaggerComponent
{
    static std::shared_ptr<oatpp::swagger::DocumentInfo> build();
    static std::vector<std::pair<std::string,std::string> > serverUrls;

   public:
    static void addSwaggerServerUrl(const std::string& url,
                                 const std::string& name);

    /**
   *  General API docs info
   */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::swagger::DocumentInfo>,
                           swaggerDocumentInfo)
    ([] { return SwaggerComponent::build(); }());

    /**
   *  Swagger-Ui Resources (<oatpp-examples>/lib/oatpp-swagger/res)
   */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::swagger::Resources>,
                           swaggerResources)
    ([] {
        // Make sure to specify correct full path to oatpp-swagger/res folder !!!
        return oatpp::swagger::Resources::loadResources(OATPP_SWAGGER_RES_PATH);
    }());
};

#endif /* SwaggerComponent_hpp */