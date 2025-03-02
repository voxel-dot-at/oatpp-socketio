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

#include "oatpp_sio/globals.hpp"

#include "oatpp_sio/webapi/webApp.hpp"

using namespace std;

namespace oatpp_sio {

static WebApiState theState;

WebApiState &getGlobalState() { return theState; }


/** perform controller-specific initialisation before registering to the web api & starting it */
static void registerOatppControllers(WebApiState &state)
{
    // auto btaAdapCtrl = std::make_shared<BtaAdapterController>();

    // btaAdapCtrl->registerBta(state.bta);
    // oatpp_sio::webapi::registerController(btaAdapCtrl);

    // auto fic = std::make_shared<FrameInfoController>();
    // fic->setApi(&state.api);
    // oatpp_sio::webapi::registerController(fic);
}

void setupGlobalState(WebApiState &state, bool withSwagger)
{
    state.enableSwaggerUi = withSwagger;

    // initialize oatpp controller objects:
    // web api:::
    // oatpp_sio::webapi::WebAdapter *webAdap = new oatpp_sio::webapi::WebAdapter();
    // webAdap->setApi(&theState.api);

    // initialize web api linkage:
    registerOatppControllers(theState);
}

}  // namespace oatpp_sio