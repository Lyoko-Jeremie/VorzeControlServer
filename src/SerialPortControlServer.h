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

#ifndef VORZECONTROLSERVER_SERIALPORTCONTROLSERVER_H
#define VORZECONTROLSERVER_SERIALPORTCONTROLSERVER_H

#ifdef MSVC
#pragma once
#endif

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <memory>
#include <string>
#include <sstream>
#include <map>
#include <list>
#include <set>
#include <tuple>
#include <utility>
#include <exception>
#include <mutex>
#include <algorithm>
#include <functional>
#include <utility>
#include "ConfigLoader.h"
#include "ActionModeManager.h"
#include "error_info.h"
#include "SerialPortFinder.h"

class SerialPortSession : public std::enable_shared_from_this<SerialPortSession> {
protected:
    boost::asio::executor ex;
    std::shared_ptr<ConfigLoader> configLoader;
    boost::asio::serial_port serialPort;
    std::string serialPortName;

    std::set<std::shared_ptr<std::string>> sendingData;
    std::mutex sendingDataMtx;

public:
    SerialPortSession(
            boost::asio::executor ex,
            std::shared_ptr<ConfigLoader> configLoader,
            std::shared_ptr<ActionModeManager> actionModeManager
    ) : ex(ex),
        configLoader(configLoader),
        serialPort(ex),
        actionModeManager(actionModeManager) {}

    auto open(const std::string &_serialPortName) -> std::pair<bool, error_info> {
        close();
        serialPortName = _serialPortName;
        boost::system::error_code ec;
        serialPort.open(serialPortName, ec);
        error_info ei;
        if (ec) {
            std::stringstream ss;
            ss << "SerialPortSession::open() error on serialPortName:" << serialPortName
               << " error:" << ec.message();
            ei.info += ss.str();
            std::cerr << ei.info << std::endl;
            return {false, ei};
        }
        return {true, ei};
    }

    const std::string &getSerialPortName() {
        return serialPortName;
    }

    bool is_open() {
        return serialPort.is_open();
    }

    auto setBaudRate(unsigned int baudRate) -> std::pair<bool, error_info> {
        if (serialPort.is_open()) {
            boost::asio::serial_port::baud_rate o(baudRate);
            boost::system::error_code ec;
            serialPort.set_option(o, ec);
            error_info ei;
            if (ec) {
                std::stringstream ss;
                ss << "setBaudRate::setBaudRate() error on serialPortName:" << serialPortName
                   << " with baudRate:" << baudRate
                   << " error:" << ec.message();
                ei.info += ss.str();
                std::cerr << ei.info << std::endl;
                return {false, ei};
            }
            return {true, ei};
        }
        return {false, {"!serialPort.is_open()"}};
    }

    [[nodiscard]]
    auto getBaudRate() -> std::pair<unsigned int, error_info> {
        if (serialPort.is_open()) {
            boost::asio::serial_port::baud_rate o;
            boost::system::error_code ec;
            serialPort.get_option(o, ec);
            error_info ei;
            if (ec) {
                std::stringstream ss;
                ss << "setBaudRate::getBaudRate() error on serialPortName:" << serialPortName
                   << " error:" << ec.message();
                ei.info += ss.str();
                std::cerr << ei.info << std::endl;
                return {0, ei};
            }
            return {o.value(), ei};
        }
        return {0, {"!serialPort.is_open()"}};
    }

    auto setParity(
            boost::asio::serial_port::parity::type parityType = boost::asio::serial_port::parity::type::none
    ) -> std::pair<bool, error_info> {
        if (serialPort.is_open()) {
            boost::asio::serial_port::parity o(parityType);
            boost::system::error_code ec;
            serialPort.set_option(o, ec);
            error_info ei;
            if (ec) {
                std::stringstream ss;
                ss << "setBaudRate::setParity() error on serialPortName:" << serialPortName
                   << " with parityType:" << parityType
                   << " error:" << ec.message();
                ei.info += ss.str();
                std::cerr << ei.info << std::endl;
                return {false, ei};
            }
            return {true, ei};
        }
        return {false, {"!serialPort.is_open()"}};
    }

    [[nodiscard]]
    auto getParity() -> std::pair<boost::asio::serial_port::parity::type, error_info> {
        if (serialPort.is_open()) {
            boost::asio::serial_port::parity o;
            boost::system::error_code ec;
            serialPort.get_option(o, ec);
            error_info ei;
            if (ec) {
                std::stringstream ss;
                ss << "setBaudRate::getParity() error on serialPortName:" << serialPortName
                   << " error:" << ec.message();
                ei.info += ss.str();
                std::cerr << ei.info << std::endl;
            }
            return {o.value(), ei};
        }
        return {boost::asio::serial_port::parity::type::none, {"!serialPort.is_open()"}};
    }

    using SendCompleteCallback = std::function<void(const error_info &ec)>;

    static const SendCompleteCallback noop;

    void sendAsync(const std::string &data, const SendCompleteCallback &cb = noop) {
        if (!serialPort.is_open()) {
            cb({"!serialPort.is_open()"});
            return;
        }

        decltype(std::declval<decltype(sendingData)>().emplace()) refData;
        {
            // https://stackoverflow.com/questions/20516773/stdunique-lockstdmutex-or-stdlock-guardstdmutex
            // https://stackoverflow.com/questions/43019598/stdlock-guard-or-stdscoped-lock
            std::lock_guard<decltype(sendingDataMtx)> lockGuard{sendingDataMtx};

            // a copy of data, make sure the data alive during write
            refData = sendingData.emplace(std::make_shared<std::string>(data));
            if (data.data() == refData.first->get()->data()) {
                // never into there
                throw std::exception("SerialPortSession::sendAsync() design error, data copy not work.");
            }
        }
        boost::asio::async_write(
                serialPort,
                boost::asio::buffer(*refData.first->get()),
                [self = shared_from_this(), this, refData, cb](
                        const boost::system::error_code &ec, std::size_t bytes_transferred
                ) {
                    boost::ignore_unused(bytes_transferred);
                    if (ec) {
                        // dont care it
                    }
                    {
                        std::lock_guard<decltype(sendingDataMtx)> lockGuard{sendingDataMtx};
                        // delete data copy
                        sendingData.erase(refData.first);
                    }
                    cb(ec);
                });
    }

    void sendSync(const std::string &data, const SendCompleteCallback &cb = noop) {
        if (!serialPort.is_open()) {
            cb({"!serialPort.is_open()"});
            return;
        }

        decltype(std::declval<decltype(sendingData)>().emplace()) refData;
        {
            // https://stackoverflow.com/questions/20516773/stdunique-lockstdmutex-or-stdlock-guardstdmutex
            // https://stackoverflow.com/questions/43019598/stdlock-guard-or-stdscoped-lock
            std::lock_guard<decltype(sendingDataMtx)> lockGuard{sendingDataMtx};
            // a copy of data, make sure the data alive during write
            refData = sendingData.emplace(std::make_shared<std::string>(data));
            if (data.data() == refData.first->get()->data()) {
                // never into there
                throw std::exception("SerialPortSession::sendSync() design error, data copy not work.");
            }
        }
        // call sync or async
        // to keep write op run in same thread
        // try-inline run, if the caller same in the `ex`
        boost::asio::dispatch(ex, [self = shared_from_this(), this, refData, cb]() {
            if (!serialPort.is_open()) {
                cb({"!serialPort.is_open()"});
                return;
            }
            boost::system::error_code ec;
            std::size_t bytes_transferred = boost::asio::write(
                    serialPort,
                    boost::asio::buffer(*refData.first->get()),
                    ec);
            boost::ignore_unused(bytes_transferred);
            if (ec) {
                // dont care it
            }
            {
                std::lock_guard<decltype(sendingDataMtx)> lockGuard{sendingDataMtx};
                // delete data copy
                sendingData.erase(refData.first);
            }
            cb(ec);
        });
    }

    void close() {
        if (serialPort.is_open()) {
            // send end op
            stop([self = shared_from_this(), this](const error_info &) {

                // then close it
                boost::system::error_code ec;
                serialPort.close(ec);
                if (ec) {
                    std::cerr << "setBaudRate::close() error on serialPortName:" << serialPortName
                              << " error:" << ec.message() << std::endl;
                }

            });
        }
    }

public:

    void init(const std::string &_serialPortName, const SendCompleteCallback &cb = noop) {
        boost::asio::dispatch(ex, [self = shared_from_this(), this, _serialPortName, cb] {

            auto start = [self = shared_from_this(), this, _serialPortName, cb](const error_info &) {
                setBaudRate(19200);
                auto rVo = open(_serialPortName);
                if (rVo.first) {
                    setBaudRate(19200);
                }
                cb(rVo.second);
            };

            if (is_open()) {
                stop(start);
            } else {
                start(error_info{});
            }
        });
    }

    void stop(const SendCompleteCallback &cb = noop) {
        sendCommand(0, cb);
    }

    void sendCommand(unsigned char c, const SendCompleteCallback &cb = noop, bool async = false) {
        if (is_open()) {
            std::array<unsigned char, 3> dataBuf{1, 1, 0};
            dataBuf[2] = c;
            if (async) {
                sendAsync(std::string{(char *) dataBuf.data(), dataBuf.size()}, cb);
            } else {
                sendSync(std::string{(char *) dataBuf.data(), dataBuf.size()}, cb);
            }
        } else {
            cb({"!serialPort.is_open()"});
            return;
        }
    }

    void setState(bool direct = true, uint8_t speed = 0, const SendCompleteCallback &cb = noop) {
        if (!is_open()) {
            cb({"!serialPort.is_open()"});
            return;
        }
        if (speed > 100) {
            speed = 100;
        }
        if (speed >= 0x80) {
            speed = 0;
        }
        sendCommand(static_cast<uint8_t>(direct ? 0x80 : 0x00) + speed, cb, true);
    }

protected:

    std::shared_ptr<ActionModeManager> actionModeManager;

public:
    void setState(
            std::string mode,
            bool direct = true,
            uint8_t speed = 0,
            const SendCompleteCallback &cb = noop) {
        if ("none" == mode) {
            setState(direct, speed, cb);
            return;
        }
        // TODO the mode manager
        actionModeManager->init(mode, ex);
        // TODO init timer and callback chain
    }

};


using SerialPortSessionTarget = SerialPortSession;


class SerialPortControlServer : public std::enable_shared_from_this<SerialPortControlServer> {
    boost::asio::executor ex;
    std::shared_ptr<ConfigLoader> configLoader;
    std::shared_ptr<ActionModeManager> actionModeManager;
    std::shared_ptr<SerialPortFinder> serialPortFinder;

    std::list<std::shared_ptr<SerialPortSessionTarget>> sessions;

    std::vector<SerialPortNameInfo> portsInfo;
public:

    SerialPortControlServer(boost::asio::executor ex,
                            std::shared_ptr<ConfigLoader> configLoader,
                            std::shared_ptr<ActionModeManager> actionModeManager,
                            std::shared_ptr<SerialPortFinder> serialPortFinder) :
            ex(ex),
            configLoader(configLoader),
            actionModeManager(actionModeManager),
            serialPortFinder(serialPortFinder) {
    }

public:
    void start() {
#ifdef PromiseCpp_FOUND
        serialPortFinder->promiseFind().then(
                [self = shared_from_this(), this](const std::vector<SerialPortNameInfo> &ports) {
                    portsInfo = ports;
                });
#else // ^^^ PromiseCpp_FOUND / PromiseCpp not FOUND vvv
        serialPortFinder->find(
                [self = shared_from_this(), this](const std::vector<SerialPortNameInfo> &ports, const error_info &e) {
                    if (e) {
                        return;
                    }
                    portsInfo = ports;
                })
#endif // PromiseCpp_FOUND

        // TODO
    }

    const std::vector<SerialPortNameInfo> &getPortsInfo() {
        return portsInfo;
    }

    std::shared_ptr<SerialPortSessionTarget> create(
            const std::string &_serialPortName
    ) {
        auto s = std::make_shared<SerialPortSessionTarget>(ex, configLoader, actionModeManager);
        sessions.push_back(s->shared_from_this());
        s->init(_serialPortName);
        return s;
    }

    std::shared_ptr<SerialPortSessionTarget> get(const std::string &_serialPortName) {
        auto it = std::find_if(
                sessions.begin(), sessions.end(),
                [_serialPortName](const decltype(sessions)::value_type &a) {
                    return a->getSerialPortName() == _serialPortName;
                }
        );
        return (it != sessions.end() ? *it : std::shared_ptr<SerialPortSessionTarget>{});
    }

    void stopAll() {
        for (auto &a: sessions) {
            if (a) {
                a->close();
            }
        }
        sessions.clear();
    }

};


#endif //VORZECONTROLSERVER_SERIALPORTCONTROLSERVER_H
