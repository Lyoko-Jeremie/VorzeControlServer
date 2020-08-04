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
#include <mutex>
#include "ConfigLoader.h"

using ActionTimePoint = std::chrono::time_point<std::chrono::system_clock>;
using ActionTimeDuration = std::chrono::milliseconds;


template<class T>
ActionTimeDuration ActionTimeDuration_cast(T d) {
    return std::chrono::duration_cast<ActionTimeDuration>(d);
}

inline
ActionTimePoint getActionTimePointNow() {
    return std::chrono::system_clock::now();
}


struct ActionItem {
    uint8_t direct;
    uint8_t speed;
    size_t timeTick;
};

struct ActionInfo : public std::enable_shared_from_this<ActionInfo> {
    std::string name;
    std::vector<ActionItem> ops;
};

class ActionSession : public std::enable_shared_from_this<ActionSession> {
    boost::asio::executor ex;

    std::shared_ptr<ActionInfo> action;
    const ActionTimePoint initTime;

public:
    ActionSession(
            boost::asio::executor ex,
            std::shared_ptr<ActionInfo> action
    ) : ex(ex),
        action(action),
        initTime(getActionTimePointNow()) {}

    auto getNowAction() {
        auto dt = ActionTimeDuration_cast(getActionTimePointNow() - initTime);
        // TODO calc
        if (dt.count() % action->ops.back().timeTick) {
//            action->ops.at()
        }
    }

};

class ActionModeManager : public std::enable_shared_from_this<ActionModeManager> {
    std::shared_ptr<ConfigLoader> configLoader;

    std::map<std::string, std::shared_ptr<ActionInfo>> actionLib;
    std::mutex actionLibMtx;

public:
    ActionModeManager(
            std::shared_ptr<ConfigLoader> configLoader
    ) : configLoader(configLoader) {}

    void loadActionFromConfig() {
        std::map<std::string, std::shared_ptr<ActionInfo>> actionLibTemp;
        // TODO load actionLib from file


        {
            std::lock_guard lg{actionLibMtx};
            actionLib.clear();
            actionLib = actionLibTemp;
        }
    }

    std::shared_ptr<ActionSession> init(const std::string &mode, boost::asio::executor &ex) {
        std::lock_guard lg{actionLibMtx};
        auto n = actionLib.find(mode);
        if (n != actionLib.end()) {
            return std::make_shared<ActionSession>(ex, n->second);
        } else {
            return {};
        }
    }

};


#endif //VORZECONTROLSERVER_ACTIONMODEMANAGER_H
