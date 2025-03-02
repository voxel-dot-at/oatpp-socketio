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

#ifndef WEB_CONTROLLER_HPP
#define WEB_CONTROLLER_HPP

#include "oatpp/base/Log.hpp"
#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include "oatpp_sio/globals.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)  //<-- Begin Codegen

/**
 * controller for static web contents.
 */
class StaticContentsController : public oatpp::web::server::api::ApiController
{
   public:
    /**
   * Constructor with object mapper.
   * @param apiContentMappers - mappers used to serialize/deserialize `s.
   */
    StaticContentsController(OATPP_COMPONENT(
        std::shared_ptr<oatpp::web::mime::ContentMappers>, apiContentMappers))
        : oatpp::web::server::api::ApiController(apiContentMappers)
    {
    }

    static std::string getContentType(const std::string &path)
    {
        const size_t i = path.find_last_of(".");
        if (i != std::string::npos) {
            const std::string extension = path.substr(i + 1);
            if (extension == "html" || extension == "htm" ||
                extension == "shtml")
                return "text/html";
            if (extension == "js") return "application/javascript";
            if (extension == "css") return "text/css";
            if (extension == "jpg" || extension == "jpg") return "image/jpeg";
            if (extension == "gif") return "image/gif";
            if (extension == "ico") return "image/x-icon";
            if (extension == "png") return "image/png";
            if (extension == "svg" || extension == "svgz")
                return "image/svg+xml";
        }
        return "text/plain"; // (in)sane fallback
    };

   public:
    ENDPOINT_ASYNC("GET", "*", StaticContents)
    {
        ENDPOINT_ASYNC_INIT(StaticContents);

        Action act() override
        {
            oatpp_sio::WebApiState &theState = oatpp_sio::getGlobalState();

            std::string path = request->getPathTail();
            // Load the home page in case of no path, it means calling root like:
            // http://localhost/
            if (path.empty()) {
                path = "index.html";
            }
            if (path.at(path.size() - 1) == '/') {
                path += "index.html";
            }
            if (path.find('?') >= 0) {
                path = path.substr(0, path.find('?'));
            }
            // TODO: check jail escape sequences.

            // We will check the file if exist and send the index.html in case of
            // not
            OATPP_LOGd("WEB", "GET {}", theState.webRoot + path);
            auto file =
                oatpp::String::loadFromFile((theState.webRoot + path).c_str());
            // Send 404 not found in case of no file
            OATPP_ASSERT_HTTP(file.get() != nullptr, Status::CODE_404,
                              "File not found");

            // As file already found, we can search the content type
            const std::string contentType = getContentType(path);

            // Creating the response
            auto response = controller->createResponse(Status::CODE_200, file);
            if (contentType
                    .size())  // Add the content-type header only if we have a
                              // known mime
                response->putHeader(Header::CONTENT_TYPE, contentType);

            return _return(response);
        }
    };
};

#include OATPP_CODEGEN_END(ApiController)  //<-- End Codegen

#endif /* MyController_hpp */