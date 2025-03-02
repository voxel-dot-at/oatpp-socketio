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

#ifndef WEB_APP_H_
#define WEB_APP_H_

#include "oatpp/web/server/api/ApiController.hpp"

#include "oatpp_sio/globals.hpp"

//class WebApiState;

namespace oatpp_sio {
namespace webapi {

extern void registerController(
    std::shared_ptr<oatpp::web::server::api::ApiController> controller);

/** add URLs to the swagger UI init list. cal this before(!) webApiInit() */
extern void addSwaggerServerUrl(const std::string& url, const std::string& name);

/** initialize the oatpp web infrasturcture, register internal controllers. */
extern void webApiInit();

/** start the actual oapp web frontend in it's own thread. webApiInit() must have been called first. */
extern void webApiStart(WebApiState& state);

extern void webApiStop();

}  // namespace webapi
}  // namespace oatpp_sio

#endif