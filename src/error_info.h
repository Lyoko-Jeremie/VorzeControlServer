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

#ifndef VORZECONTROLSERVER_ERROR_INFO_H
#define VORZECONTROLSERVER_ERROR_INFO_H

#ifdef MSVC
#pragma once
#endif

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <string>

struct error_info {
    std::string info;
    boost::system::error_code ec;

    error_info() = default;

    error_info(std::string info) : info(std::move(info)) {}

    error_info(std::string info, boost::system::error_code ec) : info(std::move(info)), ec(ec) {}

    error_info(boost::system::error_code ec) : ec(ec) {}

    explicit operator bool() const {
        return !info.empty() || ec;
    }

    [[nodiscard]]
    std::string message() const {
        if (!info.empty()) {
            return info;
        }
        if (ec) {
            return ec.message();
        }
        // no error
        return {};
    }
};



#endif //VORZECONTROLSERVER_ERROR_INFO_H
