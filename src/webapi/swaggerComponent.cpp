#include <iostream>

#include "oatpp_sio/webapi/swaggerComponent.hpp"

using namespace std;

std::vector<std::pair<std::string,std::string> > SwaggerComponent::serverUrls = {};

std::shared_ptr<oatpp::swagger::DocumentInfo> SwaggerComponent::build()
{
    oatpp::swagger::DocumentInfo::Builder builder;

    std::cout << "SWAG BUILDING " << serverUrls.size() << std::endl;

    builder.setTitle("Camera control api")
        .setDescription("BTA Camera control api")
        .setVersion("1.0")
        .setContactName("Simon Vogl")
        .setContactUrl("https://voxel.at/")

        .setLicenseName("Apache License, Version 2.0")
        .setLicenseUrl("http://www.apache.org/licenses/LICENSE-2.0")

        .addServer("http://localhost:8000", "server on localhost");
    // .addServer("http://192.168.2.133:8000", "server on lab.net");

    auto iter = serverUrls.begin();
    while (iter != serverUrls.end()) {
        auto pair = *iter;
        std::cout << "SWAG add server " << pair.first << " " << pair.second << std::endl;
        builder.addServer(pair.first, pair.second);
        iter++;
    }
    return builder.build();
}

void SwaggerComponent::addSwaggerServerUrl(const std::string& url,
                                           const std::string& name)
{
    serverUrls.push_back({url, name});
}