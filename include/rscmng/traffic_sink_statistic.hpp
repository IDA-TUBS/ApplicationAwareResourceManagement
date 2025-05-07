// Copyright (C) 2025 Dominik Stöhrmann 
//
// <stoehrmann@ida.ing.tu-bs.de>
//
// This file is part of a project licensed under the GNU Lesser General Public License v3.0.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.


#ifndef TRAFFIC_SINC_STATISTIC_h
#define TRAFFIC_SINC_STATISTIC_h

#include <iostream>
#include <thread>
#include <mutex>
#include <string> 
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <csignal>
#include <ctime>
#include <memory>

#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/bind/bind.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <rscmng/messages.hpp>
#include <rscmng/rm_abstraction.hpp>
#include <rscmng/attributes/demonstrator_config.hpp>


#define FILENAME_LOG "timestamps_object"
#define LOGGING true


namespace traffic_statistic
{

    class TrafficSink
    {
        private:

        
        public:


        const std::string filename_log = FILENAME_LOG;

        uint32_t sender_id;

        uint32_t thread_id;

        uint32_t sink_port_id;

        //struct socket_endpoint endpoint_parameter_local;

        boost::asio::io_context traffic_context;

        //udp::socket traffic_socket;

        //udp::endpoint traffic_endpoint_local;

        //rscmng::wired::application_identification &traffic_generator_meta;

        std::condition_variable &traffic_generator_notify;

        std::vector<std::thread> traffic_generator_list;
         


        /**
         * @brief Construct the Traffic Generator object
         * 
         */
        TrafficSink(
            std::string local_ip_address,
            std::vector<uint32_t> client_local_port,
            std::condition_variable &traffic_generator_notify
        );

        /**
         * @brief Destroy the Traffic Generator object
         * 
         */
        ~TrafficSink();

        /**
         * @brief Initialize ports for incomming data messages 
         * 
         */
        void initialize_sink_port(std::string local_ip_address, uint32_t local_port);

        /**
         * @brief Handle incomming data messages 
         * 
         */
        void handle_message(udp::socket &traffic_socket);


        /**
         * @brief Join Threads
         * 
         */
        void join();

    };

};

#endif