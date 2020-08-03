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

#ifndef VORZECONTROLSERVER_CONFIGLOADER_H
#define VORZECONTROLSERVER_CONFIGLOADER_H

#ifdef MSVC
#pragma once
#endif

#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <memory>
#include <optional>
#include <chrono>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using ConfigTimeDuration = std::chrono::milliseconds;

struct EmbedWebServerConfig {
    bool enable = false;
    std::string host;
    uint16_t port;
    std::string root_path;
    std::string index_file_of_root;
    std::string backendHost;
    uint16_t backendPort;

    std::string allowFileExtList;

    // calc
    std::string backend_json_string;
};

struct Config {
    std::string listenHost;
    uint16_t listenPort;

    std::string controlServerHost;
    uint16_t controlServerPort;

    EmbedWebServerConfig embedWebServerConfig;

    size_t threadNum = 0;
};

class ConfigLoader : public std::enable_shared_from_this<ConfigLoader> {
public:
    Config config;

    void print();

    void
    load(const std::string &filename);

    void parse_json(const boost::property_tree::ptree &tree);

};


#endif //VORZECONTROLSERVER_CONFIGLOADER_H
