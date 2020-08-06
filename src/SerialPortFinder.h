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

#ifndef VORZECONTROLSERVER_SERIALPORTFINDER_H
#define VORZECONTROLSERVER_SERIALPORTFINDER_H

#ifdef MSVC
#pragma once
#endif

#include <memory>
#include <regex>
#include <algorithm>
#include <functional>
#include <vector>
#include <iostream>
#include <sstream>
#include <boost/asio.hpp>
#include "wmi.hpp"
#include "wmiclasses.hpp"
#include "error_info.h"

struct Win32_PnPEntity {
    std::string Name;

    void setProperties(const Wmi::WmiResult &result, std::size_t index) {
        result.extract(index, "Name", (*this).Name);
    }

    static std::string getWmiClassName() {
        // from https://github.com/Net005/Vorze-PlayerHelper/blob/master/Libraries/Cyclone2/Devices/UsbDongle.cs
        return "Win32_PnPEntity";
    }
};

struct Win32_SerialPort {
    std::string Name;

    void setProperties(const Wmi::WmiResult &result, std::size_t index) {
        result.extract(index, "Name", (*this).Name);
    }

    static std::string getWmiClassName() {
        // from http://www.naughter.com/enumser.html
        return "Win32_SerialPort";
    }
};

using TargetWmi = Win32_SerialPort;

struct SerialPortNameInfo {
    std::string userFriendlyName;
    std::string comName;

    SerialPortNameInfo() = default;

    SerialPortNameInfo(
            std::string userFriendlyName,
            std::string comName
    ) : userFriendlyName(userFriendlyName), comName(comName) {}
};

class SerialPortFinder : public std::enable_shared_from_this<SerialPortFinder> {
    boost::asio::executor ex;
public:
    SerialPortFinder(
            boost::asio::executor ex
    ) : ex(ex) {}

    using FindCallback = std::function<void(const std::vector<SerialPortNameInfo> &ports, const error_info &e)>;

    void find(FindCallback callback) {
        // non-block
        boost::asio::post(ex, [callback]() {
            std::vector<SerialPortNameInfo> ports;
            try {

                auto wmis = Wmi::retrieveAllWmi<TargetWmi>();
                // from https://github.com/Net005/Vorze-PlayerHelper/blob/master/Libraries/Cyclone2/Devices/UsbDongle.cs
                std::regex regexFind{R"(\((COM[1-9][0-9]?[0-9]?)\)$)"};
                std::regex regexGet{R"(.*\((COM[1-9][0-9]?[0-9]?)\)$)"};
                decltype(wmis) wmif;
                std::copy_if(wmis.begin(), wmis.end(), std::back_inserter(wmif),
                             [&regexFind](const TargetWmi &service) {
                                 return std::regex_search(service.Name, regexFind);
                             });
                // std::cout << "\n";
                // std::cout << "wmis" << "\n";
                // for (const TargetWmi &service : wmis) {
                //     std::cout << service.Name << std::endl;
                // }

                // std::cout << "\n";
                // std::cout << "wmif" << "\n";
                for (const TargetWmi &service : wmif) {
                    // std::cout << service.Name << std::endl;
                    std::smatch sm;
                    std::regex_match(service.Name, sm, regexGet);
                    // std::cout << sm[1].str() << std::endl;
                    ports.emplace_back(service.Name, sm[1].str());
                }

            } catch (const Wmi::WmiException &ex) {
                error_info ei;
                std::stringstream ss;
                ss << "Wmi error: " << ex.errorMessage << ", Code: " << ex.hexErrorCode();
                ei.info = ss.str();
                std::cerr << ei.info << std::endl;

                callback(ports, ei);
                return;
            }
            callback(ports, error_info{});
            return;
        });
    }
};


#endif //VORZECONTROLSERVER_SERIALPORTFINDER_H
