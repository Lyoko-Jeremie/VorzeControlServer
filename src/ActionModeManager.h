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
#include <algorithm>
#include "ConfigLoader.h"

/**
 * we are operate in hardware, so we must use `steady_clock`, not `system_clock`
 * steady_clock is a monotonic clock, it never decrease , always moves forward
 */
using ActionTimeClock = std::chrono::steady_clock;
using ActionTimePoint = std::chrono::time_point<ActionTimeClock>;
using ActionTimeDuration = std::chrono::milliseconds;
constexpr long long int ActionDurationPerTick = 1000;


template<class T>
ActionTimeDuration ActionTimeDuration_cast(T d) {
    return std::chrono::duration_cast<ActionTimeDuration>(d);
}

inline
ActionTimePoint getActionTimePointNow() {
    return ActionTimeClock::now();
}


struct ActionItem {
    uint8_t direct;
    uint8_t speed;
    /**
     * the timeTick means : when this ActionItem end.
     *      the tick is a time tick, start from first item.
     *      the tick of first item is how the first item long.
     *      the tick of last item is how the all item long.
     */
    size_t timeTick;

    /**
     * for sort algorithm
     */
    bool operator<(const ActionItem &o) const {
        return this->timeTick < o.timeTick;
    }
};

struct ActionInfo : public std::enable_shared_from_this<ActionInfo> {
    std::string name;
    /**
     * ops must sort by ActionItem::timeTick
     */
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
        if (dt.count() < 0) {
            // never into there
            // if into there, means the ActionTimeClock was decrease. this must be never happened.
            throw std::exception("ActionSession::getNowAction() error : (dt.count() < 0)");
        }

        // calc tick
        auto tick = (dt.count() / ActionDurationPerTick) % action->ops.back().timeTick;

        // find the first item that timeTick bigger than tick
        // way 1
        auto nIt = std::upper_bound(action->ops.begin(), action->ops.end(), ActionItem{.timeTick=tick});

        // way 2
        // size_t index = 0;
        // for (; index != action->ops.size(); ++index) {
        //     auto n = action->ops.at(index);
        //     if (n.timeTick > tick) {
        //         break;
        //     }
        // }
        // auto nIt2 = action->ops.begin();
        // std::advance(nIt2, index);

        if (nIt == action->ops.end()) {
            // never into there
            // if into there, means the `tick` calc wrong.
            throw std::exception("ActionSession::getNowAction() error : (nIt == action->ops.end())");
        }

        return *nIt;

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
