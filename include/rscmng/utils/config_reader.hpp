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


#ifndef CONFIG_READER_h
#define CONFIG_READER_h


#include <chrono>
#include <iostream>
#include <fstream> 
#include <iomanip>
#include <stdexcept>

#include <boost/bind/bind.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp> 

#include <rscmng/attributes/demonstrator_config.hpp>
#include <rscmng/utils/log.hpp>
#include <rscmng/attributes/uuid.hpp>
#include <rscmng/abstraction/socket_endpoint.hpp>


namespace rscmng{
    
    namespace config{

        #ifndef DEFAULT_CONFIG
        #define DEFAULT_CONFIG "demonstrator_configuration.json"
        #endif

        #define UNIT_SETTINGS "UNIT_SETTINGS"
        #define HOST_ID "HOST_ID"
        #define CLIENT_ID "CLIENT_ID"     
        #define RM_CONTROL_LOCAL_IP "RM_CONTROL_LOCAL_IP"
        #define RM_CONTROL_LOCAL_PORT "RM_CONTROL_LOCAL_PORT"
        #define RM_CONTROL_RM_IP "RM_CONTROL_RM_IP"     
        #define RM_CONTROL_RM_PORT "RM_CONTROL_RM_PORT"
        #define SERVICE_LOCAL_IP "SERVICE_LOCAL_IP"
        #define SERVICE_LOCAL_PORT "SERVICE_LOCAL_PORT"
        #define CLIENT_PRIORITY "CLIENT_PRIORITY"
        

        #define SERVICE_SETTINGS "SERVICE_SETTINGS"
        #define SERVICE_ID "SERVICE_ID"
        #define SERVICE_IP "SERVICE_IP"
        #define SERVICE_PORT "PORT"
        #define DATA_PATH "PATH"
        #define SOURCE_TYPE "SOURCE_TYPE"
        #define DEADLINE "DEADLINE [ms]"
        #define OBJECTSIZE "OBJECT_SIZE [KByte]"
        #define PRIORITY "PRIORITY"
        #define SLOT_OFFSET "SLOT_OFFSET [ms]"
        #define SLOT_LENGTH "SLOT_LENGTH [ms]"
        #define INTER_PACKET_GAP "INTER_PACKET_GAP[us]"
        #define INTER_OBJECT_GAP "INTER_OBJECT_GAP[us]"

        #define EXPERIMENT_SETTINGS "EXPERIMENT_SETTINGS"
        #define EXPERIMENT_NUMBER "EXPERIMENT_NUMBER"
        #define CLIENT_INIT_TIME "CLIENT_INIT_TIME[ms]"
        #define EXPERIMENT_BEGIN_OFFSET "EXPERIMENT_BEGIN_OFFSET[ms]"
        #define EXPERIMENT_END_OFFSET "EXPERIMENT_END_OFFSET[ms]"
        #define EXPERIMENT_ITERATIONS "EXPERIMENT_ITERATIONS"
        #define EXPERIMENT_SYNCHRONOUS_FLAG "EXPERIMENT_SYNCHRONOUS_FLAG"
        #define EXPERIMENT_SYNCHRONOUS_START_FLAG "EXPERIMENT_SYNCHRONOUS_START_FLAG"
        #define MC_DISTRIBUTION_PHASE_DURATION "MC_DISTRIBUTION_PHASE_DURATION[ms]"
        #define MC_CLIENT_STOP_OFFSET "MC_CLIENT_STOP_OFFSET[ms]"
        #define MC_CLIENT_RECONFIG_OFFSET "MC_CLIENT_RECONFIG_OFFSET[ms]"
        #define MC_CLIENT_START_OFFSET "MC_CLIENT_START_OFFSET[ms]"
        #define INTER_MC_GAP_MIN "INTER_MC_GAP_MIN[ms]"
        #define INTER_MC_GAP_MAX "INTER_MC_GAP_MAX[ms]"
        #define HYPERPERIOD_DURATION "HYPERPERIOD_DURATION[ms]"
        #define HYPERPERIOD_SLOTS "HYPERPERIOD_SLOTS"
        #define EXPERIMENT_STARTUP_MODE "EXPERIMENT_STARTUP_MODE"
        #define EXPERIMENT_STARTUP_MODE_MAP "EXPERIMENT_STARTUP_MODE_MAP"
        #define EXPERIMENT_RECONFIGURATION_ORDER "EXPERIMENT_RECONFIGURATION_ORDER"
        #define EXPERIMENT_RECONFIGURATION_MAP "EXPERIMENT_RECONFIGURATION_MAP"


        struct service_settings 
        {
            serviceID_t service_id;
            std::string ip_address;
            uint32_t port;
            std::vector<uint32_t> data_path;
            //std::string source_type;
            uint32_t object_size;
            uint32_t deadline;
            uint32_t service_priority;
            uint32_t slot_offset;
            uint32_t slot_length;
            std::chrono::microseconds inter_packet_gap;
            std::chrono::microseconds inter_object_gap;
            //calculated values
            uint32_t number_packets;
            double estimated_transmission_time_ms;
        };

        struct unit_settings 
        {
            std::string host_name;
            UUID_t client_uuid;
            uint32_t client_id;
            std::vector<std::string> rm_control_local_ip;
            uint32_t rm_control_local_port;
            std::vector<std::string> rm_control_rm_ip;
            uint32_t rm_control_rm_port;
            std::vector<std::string> service_local_ip;
            std::vector<uint32_t> service_local_port;
            uint32_t client_priority;
        };

        struct experiment_parameter
        {
            uint32_t experiment_number;
            std::chrono::milliseconds client_init_time;
            std::chrono::milliseconds experiment_begin_offset;
            std::chrono::milliseconds experiment_end_offset;
            uint32_t experiment_iterations;
            //bool synchronous_mode;
            bool synchronous_start_mode;
            std::chrono::milliseconds mc_distribution_phase_duration;
            std::chrono::milliseconds mc_client_stop_offset;
            std::chrono::milliseconds mc_client_reconfig_offset;
            std::chrono::milliseconds mc_client_start_offset;
            std::chrono::milliseconds inter_mc_gap_min;
            std::chrono::milliseconds inter_mc_gap_max;
            std::chrono::milliseconds hyperperiod_duration;
            uint32_t hyperperiod_slots;
            uint32_t startup_mode;
            std::map<uint32_t,std::map<uint32_t,uint32_t>> startup_mode_map;
            std::list<uint32_t> reconfiguration_order;
            // mode <client_id, mode>
            std::map<uint32_t,std::map<uint32_t,uint32_t>> reconfiguration_map;

        };

        class ConfigReader
        {
            public:
            /*--------------------- Methods -----------------------*/
            
            /**
             * @brief Construct a new empty ConfigReader object
             * 
             */
            ConfigReader();


            /**
             * @brief Destroy the ConfigReader object
             * 
             */
            ~ConfigReader();

            /**
             * @brief load a service configuration
             * 
             * @param service_name service name
             * @param config_file_path path to config_parsed_file file
             */
            std::map<uint32_t, struct service_settings> load_service_settings
            (
                uint32_t service_id, 
                std::string config_file_path = DEFAULT_CONFIG
            );

            /**
             * @brief load a host configuration
             * 
             * @param host_name host name
             * @param config_file_path path to config_parsed_file file
             */
            struct unit_settings load_unit_settings
            (
                std::string client_name, 
                std::string config_file_path = DEFAULT_CONFIG
            );

            /**
             * @brief load a host configuration
             * 
             * @param host_name host name
             * @param config_file_path path to config_parsed_file file
             */
            struct unit_settings load_unit_settings
            (
                uint32_t client_id, 
                std::string config_file_path = DEFAULT_CONFIG
            );

            /**
             * @brief filter client configuration
             * 
             * @param config_file_path path to config_parsed_file file 
             */
            std::map<uint32_t, struct unit_settings> load_rm_client_addresses
            (
                std::string config_file_path
            );

            /**
             * @brief 
             * 
             * @param config_file_path path to config_parsed_file file 
             */
            struct experiment_parameter load_experiment_settings
            (
                std::string config_file_path
            ); 

            /**
             * @brief filter client configuration
             * 
             * @param config_file_path path to config_parsed_file file 
             */
            std::map<uint32_t, std::map<uint32_t, struct rscmng::config::service_settings>> load_rm_client_services
            (
                std::string config_file_path
            );
            
            /**
             * @brief print the configuration 
             * 
             */
            void print_all_services(std::map<uint32_t, std::map<uint32_t, struct service_settings>> service);

            /**
             * @brief print the configuration 
             * 
             */
            void print_service(std::map<uint32_t, struct service_settings> service);

            /**
             * @brief print the configuration 
             * 
             */
            void print_unit(struct unit_settings unit);

            /**
             * @brief print the configuration 
             * 
             */
            void print_rm_clients(std::map<uint32_t, struct unit_settings> rm_client_adress_map);


            /**
             * @brief print the configuration 
             * 
             */
            void print_experiment_settings(struct experiment_parameter& experiment_settings);


            private:

            /**
             * @brief
             */
            std::map<uint32_t, std::map<uint32_t, struct service_settings>> check_service_settings(boost::property_tree::ptree config_parsed_file);

            /**
             * @brief
             */
            std::map<uint32_t, struct unit_settings> check_unit_settings(boost::property_tree::ptree config_parsed_file);

            /**
             * @brief
             */
            std::map<std::string, struct unit_settings> check_unit_settings_string(boost::property_tree::ptree config_parsed_file);

            /**
             * @brief
             */
            struct service_settings parse_service_settings(boost::property_tree::ptree service_tree);

            /**
             * @brief
             */
            struct unit_settings parse_unit_settings(boost::property_tree::ptree unit_tree);

            /**
             * @brief
             */
            struct experiment_parameter check_experiment_settings(boost::property_tree::ptree config_parsed_file);

            /**
             * @brief
             */
            uint32_t convert_hex_to_int(std::string hex_string);

            /**
             * @brief
             */
            std::string remove_hex_prefix_string(std::string hex_string);

            std::map<uint32_t, std::map<uint32_t, struct service_settings>> rm_client_service_map;
            std::map<uint32_t, struct unit_settings> rm_client_unit_map;
            std::map<std::string, struct unit_settings> rm_client_unit_string_map;
        };

    }; // end namespace conifg

}; // end namespace rscmng

#endif