/**
 * VorzeControlServer : A VORZE electric toys Vorze A10 Cyclone/Piston SA Remote Control Adapter Server Powered by Boost.Asio
 * Copyright (C) 2020 Jeremie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ConfigLoader.h"


void ConfigLoader::print() {
    std::cout << "config.listenHost:" << config.listenHost << "\n";
    std::cout << "config.listenPort:" << config.listenPort << "\n";
    std::cout << "config.controlServerHost:" << config.controlServerHost << "\n";
    std::cout << "config.controlServerPort:" << config.controlServerPort << "\n";

    std::cout << "config.threadNum:" << config.threadNum << "\n";

    if (!config.embedWebServerConfig.enable) {
        std::cout << "config.embedWebServerConfig.enable : false .\n";
    } else {
        auto &ew = config.embedWebServerConfig;
        std::cout << "config.embedWebServerConfig.enable : true :\n";
        std::cout << "\t" << "embedWebServerConfig.host:" << ew.host << "\n";
        std::cout << "\t" << "embedWebServerConfig.port:" << ew.port << "\n";
        std::cout << "\t" << "embedWebServerConfig.root_path:" << ew.root_path << "\n";
        std::cout << "\t" << "embedWebServerConfig.index_file_of_root:" << ew.index_file_of_root << "\n";
        std::cout << "\t" << "embedWebServerConfig.backendHost:" << ew.backendHost << "\n";
        std::cout << "\t" << "embedWebServerConfig.backendPort:" << ew.backendPort << "\n";
        std::cout << "\t" << "embedWebServerConfig.backend_json_string:" << ew.backend_json_string << "\n";
    }

}

void ConfigLoader::load(const std::string &filename) {
    boost::property_tree::ptree tree;
    boost::property_tree::read_json(filename, tree);
    parse_json(tree);
}

void ConfigLoader::parse_json(const boost::property_tree::ptree &tree) {
    Config c{};

    auto listenHost = tree.get("listenHost", std::string{"127.0.0.1"});
    auto listenPort = tree.get<uint16_t>("listenPort", static_cast<uint16_t>(5000));

    c.listenHost = listenHost;
    c.listenPort = listenPort;

    auto controlServerHost = tree.get("controlServerHost", std::string{"127.0.0.1"});
    auto controlServerPort = tree.get<uint16_t>("controlServerPort", static_cast<uint16_t>(5010));

    c.controlServerHost = controlServerHost;
    c.controlServerPort = controlServerPort;

    auto threadNum = tree.get("threadNum", static_cast<size_t>(0));
    c.threadNum = threadNum;


    c.embedWebServerConfig = {};
    c.embedWebServerConfig.enable = false;
    c.embedWebServerConfig.host = "127.0.0.1";
    c.embedWebServerConfig.port = 5002;
    c.embedWebServerConfig.backendHost = "";
    c.embedWebServerConfig.backendPort = 0;
    c.embedWebServerConfig.root_path = "./html/";
    c.embedWebServerConfig.index_file_of_root = "state.html";
    c.embedWebServerConfig.allowFileExtList = "htm html js json jpg jpeg png bmp gif ico svg";
#ifndef DISABLE_EmbedWebServer
    if (tree.get_child_optional("EmbedWebServerConfig")) {
        auto pts = tree.get_child("EmbedWebServerConfig");
        auto &embedWebServerConfig = c.embedWebServerConfig;
        embedWebServerConfig.enable = pts.get("enable", false);
        if (embedWebServerConfig.enable) {
            embedWebServerConfig.host = pts.get("host", embedWebServerConfig.host);
            embedWebServerConfig.port = pts.get("port", embedWebServerConfig.port);
            embedWebServerConfig.backendHost = pts.get("backendHost", embedWebServerConfig.backendHost);
            embedWebServerConfig.backendPort = pts.get("backendPort", embedWebServerConfig.backendPort);
            embedWebServerConfig.root_path = pts.get("root_path", embedWebServerConfig.root_path);
            embedWebServerConfig.index_file_of_root = pts.get("index_file_of_root",
                                                              embedWebServerConfig.index_file_of_root);
            embedWebServerConfig.allowFileExtList = pts.get("allowFileExtList", embedWebServerConfig.allowFileExtList);

        }
    }
#endif // DISABLE_EmbedWebServer


    c.embedWebServerConfig.backend_json_string = (
            boost::format{R"({"host":"%1%","port":%2%})"}
            % (!c.embedWebServerConfig.backendHost.empty()
               ? c.embedWebServerConfig.backendHost : "")
            % (c.embedWebServerConfig.backendPort != 0
               ? c.embedWebServerConfig.backendPort : c.controlServerPort)
    ).str();


    config = c;
}

