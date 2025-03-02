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

#pragma once

/**
 * @brief find a colored box via hsv analysis
 * 
 */
#pragma once

#include <string>

#include <condition_variable>
#include <mutex>
#include <vector>

#include "toffy/filter.hpp"

namespace oatpp_sio {

/** data listener super class; implements thread sync artefacts */
class WebListener
{
    std::string n;

   public:
    WebListener(const std::string& name) : n(name) {}

    const std::string& name() const { return n; }

    // wait on semaphore
    void waitForWork();

    // notify semaphore
    void haveWork(const Frame& in);

    bool valid;

   protected:
    std::mutex m;
    std::condition_variable workToDo;

    // isWaiting is set to true when a web requeset is pending; do work only if this is true.
    bool isWaiting = false;
    // do something with the frame data
    virtual bool process(const Frame& in) = 0;
};

class FrameInfoListener : public WebListener
{
   public:
    FrameInfoListener(const std::string& name) : WebListener(name) {}

    // std::vector<std::string> keys;
    std::vector<Frame::SlotInfo> slots;

   protected:
    virtual bool process(const Frame& in);
};

class ImgListener : public WebListener
{
   public:
    ImgListener(const std::string& name) : WebListener(name) {}

    std::shared_ptr<std::string> strVal;

   protected:
    virtual bool process(const Frame& in);
};

class NumberListener : public WebListener
{
   public:
    NumberListener(const std::string& name) : WebListener(name) {}

    long numVal;

   protected:
    virtual bool process(const Frame& in);
};

class BtaAdapter : public Filter
{
    static const std::string id_name;  ///< Filter identifier
   public:
    WebAdapter();

    virtual ~WebAdapter() {}

    unsigned int fc = -1;

    std::vector<WebListener*> listeners;

    // virtual int loadConfig(const boost::property_tree::ptree& pt);
    virtual boost::property_tree::ptree getConfig() const;

    virtual void updateConfig(const boost::property_tree::ptree& pt);

    virtual bool filter(const Frame& in, Frame& out);

   private:
    static size_t _filter_counter;
};

extern toffy::Filter* CreateWebAdapter(void);

}  // namespace oatpp_sio
