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

#ifndef VORZECONTROLSERVER_WEBCONTROLSERVER_H
#define VORZECONTROLSERVER_WEBCONTROLSERVER_H

#ifdef MSVC
#pragma once
#endif

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <sstream>
#include <map>
#include "ConfigLoader.h"
#include "SerialPortControlServer.h"


// https://www.boost.org/doc/libs/1_73_0/libs/beast/example/http/server/small/http_server_small.cpp

class HttpConnectSession : public std::enable_shared_from_this<HttpConnectSession> {
    std::shared_ptr<ConfigLoader> configLoader;
    std::shared_ptr<SerialPortControlServer> serialPortControlServer;

public:
    HttpConnectSession(boost::asio::ip::tcp::socket socket,
                       std::shared_ptr<ConfigLoader> configLoader,
                       std::shared_ptr<SerialPortControlServer> serialPortControlServer)
            : configLoader(configLoader),
              serialPortControlServer(serialPortControlServer),
              socket_(std::move(socket)) {}

    void start() {
        read_request();
        check_deadline();
    }

protected:
    std::string createJsonString();

protected:
    // The socket for the currently connected client.
    boost::asio::ip::tcp::socket socket_;

    // The buffer for performing reads.
    boost::beast::flat_buffer buffer_{8192};

    // The request message.
    boost::beast::http::request<boost::beast::http::dynamic_body> request_;

    // The response message.
    boost::beast::http::response<boost::beast::http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing.
    boost::asio::steady_timer deadline_{socket_.get_executor(), std::chrono::seconds(60)};

protected:
    // Asynchronously receive a complete request message.
    void read_request();

    // Determine what needs to be done with the request message.
    void process_request();

    // Construct a response message based on the program state.
    void create_response();

    // Asynchronously transmit the response message.
    void write_response();

    // Check whether we have spent enough time on this connection.
    void check_deadline();

protected:
    using QueryPairsType = std::multimap<std::string, std::string>;

    void path_op(QueryPairsType &queryPairs);

};

class WebControlServer : public std::enable_shared_from_this<WebControlServer> {
    boost::asio::executor ex;
    std::shared_ptr<ConfigLoader> configLoader;
    std::shared_ptr<SerialPortControlServer> serialPortControlServer;

public:

    WebControlServer(
            boost::asio::executor ex,
            std::shared_ptr<ConfigLoader> configLoader,
            std::shared_ptr<SerialPortControlServer> serialPortControlServer
    ) :
            ex(ex),
            configLoader(configLoader),
            serialPortControlServer(serialPortControlServer),
            address(boost::asio::ip::make_address(configLoader->config.controlServerHost)),
            port(configLoader->config.controlServerPort),
            acceptor(ex, {address, port}),
            socket(ex) {
    }


protected:
    const boost::asio::ip::address address;
    unsigned short port;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket socket;


public:
    void start() {
        std::cout << "WebControlServer start on:" << address.to_string() << ":" << port << std::endl;
        http_server();
    }

private:
    // "Loop" forever accepting new connections.
    void http_server() {
        acceptor.async_accept(
                socket,
                [this, self = shared_from_this()](boost::beast::error_code ec) {
                    if (!ec) {
                        std::make_shared<HttpConnectSession>(
                                std::move(socket),
                                configLoader,
                                serialPortControlServer
                        )->start();
                    }
                    if (ec && (
                            boost::asio::error::operation_aborted == ec ||
                            boost::asio::error::host_unreachable == ec ||
                            boost::asio::error::address_in_use == ec ||
                            boost::asio::error::address_family_not_supported == ec ||
                            boost::asio::error::host_unreachable == ec ||
                            boost::asio::error::host_unreachable == ec
                    )) {
                        std::cerr << "StateMonitorServer http_server async_accept error:"
                                  << ec << std::endl;
                        return;
                    }
                    http_server();
                });
    }

};


#endif //VORZECONTROLSERVER_WEBCONTROLSERVER_H
