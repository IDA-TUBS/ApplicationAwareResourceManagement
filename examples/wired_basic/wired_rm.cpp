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


#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

#include <rscmng/utils/log.hpp>
#include <rscmng/utils/config_reader.hpp>
#include <rscmng/attributes/demonstrator_config.hpp>
#include <rscmng/abstraction/socket_endpoint.hpp>

#include <rscmng/data_sharing/safe_queue.hpp>
#include <rscmng/rm/net_layer_rm.hpp>
#include <rscmng/protocols/rm_wired_payload.hpp>
#include <rscmng/traffic_sink_statistic.hpp>


using namespace rscmng;
using namespace traffic_statistic;
using namespace config;


bool active = true;
bool init_done = false;

// CV for rm communication
std::condition_variable cv_rm_notification;
std::mutex cv_mutex;
std::condition_variable cv_traffic_sink_notification;

struct scenario_configuration
{
    uint32_t start_delay;
    bool random;
    uint32_t max_random_time;
    uint32_t min_random_time;
    
};

/**
 * 
 */
void signalHandler(int signum)
{
    std::unique_lock<std::mutex> lock(cv_mutex);
    active = false;
    if(!init_done)
    {
        exit(signum);
    }
    cv_rm_notification.notify_one();
}


/**
 * @brief central rm
 * 
 */
int main(int argc, char* argv[])
{    
    RM_logInfo("Wired RM starting...") 
    rscmng::init_rm_file_log(std::string("rm_"), " ");

    std::string host_name;
    if (argc > 1)
    {
        host_name = argv[1];
    }
    else 
    {
        host_name = "SENSORFUSION";
    }

    ConfigReader config_reader;
    struct rscmng::config::unit_settings local_rm_configuration  = config_reader.load_unit_settings(host_name, DEFAULT_CONFIG);
    std::map<uint32_t, std::map<uint32_t, struct rscmng::config::service_settings>> service_configuration_map = config_reader.load_rm_client_services(DEFAULT_CONFIG);
    std::map<uint32_t, struct rscmng::config::unit_settings> endnode_configuration_map = config_reader.load_rm_client_addresses(DEFAULT_CONFIG);
    struct rscmng::config::experiment_parameter experiment_1_parameter = config_reader.load_experiment_settings(DEFAULT_CONFIG);

    struct rscmng::network_device_configs network_configuration;
    network_configuration.local_rm_configuration    = local_rm_configuration;
    network_configuration.service_configuration_map = service_configuration_map;
    network_configuration.endnode_configuration_map = endnode_configuration_map;
    //network_configuration.scenario_config = 1;

    SafeQueue<rscmng::wired::RMMessage> aRM_nRM_queue;
    SafeQueue<rscmng::wired::RMMessage> nRM_aRM_queue;

    RM_logInfo("Init done.") 

    NetLayerRM<double> nRM(        
        local_rm_configuration.client_uuid,
        demonstrator::NUMBER_PRIORITY_LEVEL, 
        local_rm_configuration,
        network_configuration,
        experiment_1_parameter,
        aRM_nRM_queue, 
        nRM_aRM_queue
    );
    RM_logInfo("NetRM running.")


    TrafficSink sink_ip1(
        local_rm_configuration.service_local_ip[0], 
        local_rm_configuration.service_local_port, 
        cv_traffic_sink_notification);

    TrafficSink sink_ip2(   
        local_rm_configuration.service_local_ip[1], 
        local_rm_configuration.service_local_port, 
        cv_traffic_sink_notification);

    TrafficSink sink_ip3(   
        local_rm_configuration.service_local_ip[2], 
        local_rm_configuration.service_local_port, 
        cv_traffic_sink_notification);
 
    TrafficSink sink_ip4(   
        local_rm_configuration.service_local_ip[3], 
        local_rm_configuration.service_local_port, 
        cv_traffic_sink_notification);

    TrafficSink sink_ip5(   
        local_rm_configuration.service_local_ip[4], 
        local_rm_configuration.service_local_port, 
        cv_traffic_sink_notification);

    // join threads
    //aRM.join();
    nRM.join();

    sink_ip1.join();
    sink_ip2.join();
    sink_ip3.join();
    sink_ip4.join();  
    sink_ip5.join(); 

    return 0;
}