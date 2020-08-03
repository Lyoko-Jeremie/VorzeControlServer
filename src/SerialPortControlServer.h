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
#include <exception>
#include <mutex>
#include <algorithm>
#include "ConfigLoader.h"

class SerialPortSession : public std::enable_shared_from_this<SerialPortSession> {
    boost::asio::executor ex;
    boost::asio::serial_port serialPort;
    std::string serialPortName;

    std::set<std::shared_ptr<std::string>> sendingData;
    std::mutex sendingDataMtx;
public:
    SerialPortSession(
            boost::asio::executor ex
    ) : ex(ex), serialPort(ex) {}

    bool open(const std::string &_serialPortName) {
        close();
        serialPortName = _serialPortName;
        boost::system::error_code ec;
        serialPort.open(serialPortName, ec);
        if (ec) {
            std::cerr << "SerialPortSession::open() error on serialPortName:" << serialPortName
                      << " error:" << ec.message() << std::endl;
        }
        return !!ec;
    }

    const std::string &getSerialPortName() {
        return serialPortName;
    }

    struct error_info {
        std::string info;

        error_info() = default;

        error_info(std::string info) : info(info) {}

        explicit operator bool() const {
            return !info.empty();
        }
    };

    bool is_open() {
        return serialPort.is_open();
    }

    auto setBaudRate(unsigned int baudRate) -> std::tuple<bool, error_info> {
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
            }
            return {true, ei};
        }
        return {false, {"!serialPort.is_open()"}};
    }

    auto getBaudRate() -> std::tuple<unsigned int, error_info> {
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
            }
            return {o.value(), ei};
        }
        return {0, {"!serialPort.is_open()"}};
    }

    auto setParity(
            boost::asio::serial_port::parity::type parityType = boost::asio::serial_port::parity::type::none
    ) -> std::tuple<bool, error_info> {
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
            }
            return {true, ei};
        }
        return {false, {"!serialPort.is_open()"}};
    }

    auto getParity() -> std::tuple<boost::asio::serial_port::parity::type, error_info> {
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

    void sendAsync(const std::string &data) {
        if (!serialPort.is_open()) {
            return; // {.info="!serialPort.is_open()"}
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
                [self = shared_from_this(), this, refData](
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
                });
    }

    void sendSync(const std::string &data) {
        if (!serialPort.is_open()) {
            return; // {.info="!serialPort.is_open()"}
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
        boost::asio::dispatch(ex, [self = shared_from_this(), this, refData]() {
            if (!serialPort.is_open()) {
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
        });
    }

    void close() {
        if (serialPort.is_open()) {
            // send end op
            stop();

            // close
            boost::system::error_code ec;
            serialPort.close(ec);
            if (ec) {
                std::cerr << "setBaudRate::close() error on serialPortName:" << serialPortName
                          << " error:" << ec.message() << std::endl;
            }
        }
    }

public:

    void init(const std::string &_serialPortName) {
        boost::asio::dispatch(ex, [self = shared_from_this(), this, _serialPortName] {
            if (is_open()) {
                stop();
            }
            setBaudRate(19200);
            open(_serialPortName);
            setBaudRate(19200);
            dataBuf = {1, 1, 0};
        });
    }

    void stop() {
        sendCommand(0);
    }

    std::array<unsigned char, 3> dataBuf{1, 1, 0};

    void sendCommand(unsigned char c) {
        if (is_open()) {
            dataBuf[2] = c;
            sendSync(std::string{(char *) dataBuf.data(), dataBuf.size()});
        }
    }

    void setState(bool direct = true, uint8_t speed = 0) {
        if (speed > 100) {
            speed = 100;
        }
        if (speed >= 0x80) {
            speed = 0;
        }
        sendCommand((direct ? 0x80 : 0x00) + speed);
    }

};


class SerialPortControlServer : public std::enable_shared_from_this<SerialPortControlServer> {
    boost::asio::executor ex;
    std::shared_ptr<ConfigLoader> configLoader;

    std::list<std::shared_ptr<SerialPortSession>> sessions;
public:

    SerialPortControlServer(
            boost::asio::executor ex,
            std::shared_ptr<ConfigLoader> configLoader
    ) :
            ex(ex),
            configLoader(configLoader) {
    }

public:
    void start() {
        // TODO
    }

    std::shared_ptr<SerialPortSession> create(
            const std::string &_serialPortName
    ) {
        auto s = std::make_shared<SerialPortSession>(this->ex);
        sessions.push_back(s->shared_from_this());
        s->init(_serialPortName);
        return s;
    }

    std::shared_ptr<SerialPortSession> get(const std::string &_serialPortName) {
        auto it = std::find_if(
                sessions.begin(), sessions.end(),
                [_serialPortName](const decltype(sessions)::value_type &a) {
                    return a->getSerialPortName() == _serialPortName;
                }
        );
        return (it != sessions.end() ? *it : std::shared_ptr<SerialPortSession>{});
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
