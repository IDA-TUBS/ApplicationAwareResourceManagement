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


#ifndef TRAFFIC_GENERATOR_h
#define TRAFFIC_GENERATOR_h

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

#include <rscmng/utils/log.hpp>
#include <rscmng/utils/config_reader.hpp>
#include <rscmng/attributes/demonstrator_config.hpp>
#include <rscmng/messages.hpp>
#include <rscmng/rm_abstraction.hpp>


namespace traffic_generator 
{
    enum TrafficSourceType
    {
        OBJECT_BURST_DYNAMIC_CHANGE = 0,
        NORMAL_BURST
    };

    enum TrafficSourceControl
    {
        THREAD_START = 0,
        THREAD_STOP,
        THREAD_PAUSED,
        THREAD_TRANSMISSION,
        THREAD_TRANSMISSION_FINISH_OBJECT,
        THREAD_RECONFIGURE
    };


    struct traffic_generator_parameter 
    {
        bool info_flag;
        uint32_t object_size_kb;
        std::chrono::microseconds inter_object_gap;    
        std::chrono::microseconds inter_packet_gap;    
        std::chrono::milliseconds period = std::chrono::milliseconds(100);
        long load_mbit_per_second = 200;        
        u_int32_t number_packets = 2000;
        long auto_traffic_termination = 5000;
        long slack_factor = 0.75;
    };


    class TrafficGenerator
    {
        private:

        std::mutex global_mutex;

        std::condition_variable generator_conditioning_variable;

        TrafficSourceControl traffic_generator_control;

        uint32_t mode_global;

        bool stop_thread;

        std::string log_prefix = "";


        public:

        uint32_t sender_id;

        uint32_t thread_id;

        boost::asio::io_context traffic_context;

        udp::socket traffic_socket;

        udp::endpoint traffic_endpoint_local;

        std::map<uint32_t, struct rscmng::config::service_settings> service_settings_struct;

        struct rscmng::config::unit_settings client_configuration_struct;

        struct rscmng::config::experiment_parameter experiment_parameter_struct;

        traffic_generator_parameter traffic_pattern;

        TrafficSourceType traffic_source_type;

        std::thread traffic_generator;
       
        struct timespec global_period_timestamp;
        
        bool global_update_period;

        
        /**
         * @brief Construct the Traffic Generator object
         * 
         */
        TrafficGenerator(
            std::map<uint32_t, struct rscmng::config::service_settings> service_settings,
            struct rscmng::config::unit_settings client_configuration,
            struct rscmng::config::experiment_parameter experiment_parameter,
            TrafficSourceType traffic_source_type,
            traffic_generator_parameter traffic_pattern
        );

        /**
         * @brief Destroy the Traffic Generator object
         * 
         */
        ~TrafficGenerator();

        /**
         * @brief Start Generator
         * 
         */
        void start();

        /**
         * @brief Stop Generator
         * 
         */
        void stop();

        /**
         * @brief Notify shared transmission state for mutex
         * 
         */
        void notify_generator(TrafficSourceControl state);

        /**
         * @brief Notify shared transmission state for mutex with mode
         * 
         */
        void notify_generator_mode_change(TrafficSourceControl state, uint32_t mode);

        /**
         * @brief Notify shared transmission state for mutex with mode and new timestamp
         * 
         */
        void notify_generator_timestamp(TrafficSourceControl state, struct timespec, uint32_t mode, bool update_period);

        /**
         * @brief wait function for shaping
         * 
         */
        void precise_wait_us(double microseconds);

        /**
         * @brief Main send function
         * 
         */
        void send_objects_dynamic_change();

        /**
         * @brief Main send function
         * 
         */
        void send_objects_dynamic_change_asynchron();

        /**
         * @brief Select send function
         * 
         */
        void thread_type_select ();

        /**
         * @brief Join threads
         * 
         */
        void join();

    };

};

#endif