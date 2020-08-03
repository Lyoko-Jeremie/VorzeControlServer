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

#include "WebControlServer.h"


#include <regex>
#include <type_traits>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

std::string HttpConnectSession::createJsonString() {
    boost::property_tree::ptree root;

    if (configLoader) {
        boost::property_tree::ptree config;

        const auto &c = configLoader->config;
        config.put("listenHost", c.listenHost);
        config.put("listenPort", c.listenPort);
        config.put("controlServerHost", c.controlServerHost);
        config.put("controlServerHost", c.controlServerHost);
//        config.put("retryTimes", c.retryTimes);
//        config.put("connectTimeout", c.connectTimeout.count());
//        config.put("sleepTime", c.sleepTime.count());

        const auto &ews = c.embedWebServerConfig;
        boost::property_tree::ptree pEWS;
        {
            pEWS.put("enable", ews.enable);
            pEWS.put("host", ews.host);
            pEWS.put("port", ews.port);
            pEWS.put("backendHost", ews.backendHost);
            pEWS.put("backendPort", ews.backendPort);
            pEWS.put("root_path", ews.root_path);
            pEWS.put("index_file_of_root", ews.index_file_of_root);
            pEWS.put("backend_json_string", ews.backend_json_string);
        }
        config.add_child("EmbedWebServerConfig", pEWS);

        root.add_child("config", config);
    }


    std::stringstream ss;
    boost::property_tree::write_json(ss, root);
    return ss.str();
}

void HttpConnectSession::read_request() {
    auto self = shared_from_this();

    boost::beast::http::async_read(
            socket_,
            buffer_,
            request_,
            [self](boost::beast::error_code ec,
                   std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);
                if (!ec)
                    self->process_request();
            });
}

void HttpConnectSession::process_request() {
    response_.version(request_.version());
    response_.keep_alive(false);

    if (request_.find(boost::beast::http::field::origin) != request_.end()) {
        auto origin = request_.at(boost::beast::http::field::origin);
        if (!origin.empty()) {
            response_.set(boost::beast::http::field::access_control_allow_origin, origin);
        }
    }

    switch (request_.method()) {
        case boost::beast::http::verb::get:
            response_.result(boost::beast::http::status::ok);
            response_.set(boost::beast::http::field::server, "Beast");
            create_response();
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            response_.result(boost::beast::http::status::bad_request);
            response_.set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(response_.body())
                    << "Invalid request-method '"
                    << std::string(request_.method_string())
                    << "'";
            break;
    }

    write_response();
}

void HttpConnectSession::path_op(HttpConnectSession::QueryPairsType &queryPairs) {
    response_.result(boost::beast::http::status::ok);

    if (!queryPairs.empty()) {

        // find target
        auto targetMode = queryPairs.find("_targetMode");
        auto target = queryPairs.find("_target");

        try {

            // TODO

        } catch (const boost::bad_lexical_cast &e) {
            std::cout << "boost::bad_lexical_cast:" << e.what() << std::endl;
            response_.result(boost::beast::http::status::bad_request);
            response_.set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(response_.body()) << "boost::bad_lexical_cast:" << e.what() << "\r\n";
        } catch (const std::exception &e) {
            std::cout << "std::exception:" << e.what() << std::endl;
            response_.result(boost::beast::http::status::bad_request);
            response_.set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(response_.body()) << "std::exception:" << e.what() << "\r\n";
        }
    }
}

void HttpConnectSession::create_response() {
    std::cout << "request_.target():" << request_.target() << std::endl;

    if (request_.target() == "/") {
        response_.set(boost::beast::http::field::content_type, "text/json");
        boost::beast::ostream(response_.body())
                << createJsonString() << "\n";
        return;
    }

    // from https://github.com/boostorg/beast/issues/787#issuecomment-376259849
    static const std::regex PARSE_URL{R"((/([^ ?]+)?)?/?\??([^/ ]+\=[^/ ]+)?)",
                                      std::regex_constants::ECMAScript | std::regex_constants::icase};
    std::smatch match;
    auto url = request_.target().to_string();
    if (std::regex_match(url, match, PARSE_URL) && match.size() == 4) {
        std::string path = match[1];
        std::string query = match[3];

        std::vector<std::string> queryList;
        boost::split(queryList, query, boost::is_any_of("&"));

        HttpConnectSession::QueryPairsType queryPairs;
        for (const auto &a : queryList) {
            std::vector<std::string> p;
            boost::split(p, a, boost::is_any_of("="));
            if (p.size() == 1) {
                queryPairs.emplace(p.at(0), "");
            }
            if (p.size() == 2) {
                queryPairs.emplace(p.at(0), p.at(1));
            }
            if (p.size() > 2) {
                std::stringstream ss;
                for (size_t i = 1; i != p.size(); ++i) {
                    ss << p.at(i);
                }
                queryPairs.emplace(p.at(0), ss.str());
            }
        }

        std::cout << "query:" << query << std::endl;
        std::cout << "queryList:" << "\n";
        for (const auto &a : queryList) {
            std::cout << "\t" << a;
        }
        std::cout << std::endl;
        std::cout << "queryPairs:" << "\n";
        for (const auto &a : queryPairs) {
            std::cout << "\t" << a.first << " = " << a.second;
        }
        std::cout << std::endl;

        if (path == "/op") {
            return path_op(queryPairs);
        }
    }

    response_.result(boost::beast::http::status::not_found);
    response_.set(boost::beast::http::field::content_type, "text/plain");
    boost::beast::ostream(response_.body()) << "File not found\r\n";

}

void HttpConnectSession::write_response() {
    auto self = shared_from_this();

    response_.set(boost::beast::http::field::content_length, response_.body().size());

    boost::beast::http::async_write(
            socket_,
            response_,
            [self](boost::beast::error_code ec, std::size_t) {
                self->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                self->deadline_.cancel();
            });
}

void HttpConnectSession::check_deadline() {
    auto self = shared_from_this();

    deadline_.async_wait(
            [self](boost::beast::error_code ec) {
                if (!ec) {
                    // Close socket to cancel any outstanding operation.
                    self->socket_.close(ec);
                }
            });
}

