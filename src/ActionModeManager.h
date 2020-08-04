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

#ifndef VORZECONTROLSERVER_ACTIONMODEMANAGER_H
#define VORZECONTROLSERVER_ACTIONMODEMANAGER_H

#ifdef MSVC
#pragma once
#endif

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "ConfigLoader.h"


class ActionModeManager : public std::enable_shared_from_this<ActionModeManager> {
    boost::asio::executor ex;
    std::shared_ptr<ConfigLoader> configLoader;

public:
    ActionModeManager(
            boost::asio::executor ex,
            std::shared_ptr<ConfigLoader> configLoader
    ) : ex(ex),
        configLoader(configLoader) {}

    void init() {}
};


#endif //VORZECONTROLSERVER_ACTIONMODEMANAGER_H
