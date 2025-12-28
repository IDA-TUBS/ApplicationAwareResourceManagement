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
//#include <filesystem>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <functional> 
//#include <experimental/filesystem>
#include <dirent.h>     // for opendir, readdir, closedir
#include <sys/types.h>  // for DIR

#include <rscmng/utils/log.hpp>
#include <rscmng/utils/config_reader.hpp>
#include <rscmng/rm_communication.hpp>
#include <rscmng/rm_abstraction.hpp>
#include <rscmng/protocols/rm_wired_payload.hpp>
#include <rscmng/traffic_generator.hpp>


using namespace rscmng;
using namespace wired;
using namespace basic_protocol;
using namespace traffic_generator; 
using namespace config;
//namespace fs = std::filesystem;
//namespace fs = std::experimental::filesystem;

#define SCRIPT_PATH_BASE "./scripts/"
#define SCRIPT_MODE00 "_switch_m0"
#define SCRIPT_MODE01 "_switch_m0_m1"
#define SCRIPT_MODE10 "_switch_m1_m0"


bool active = true;
bool init_done = false;

// CV for rm communication
std::condition_variable cv_rm_notification;
std::condition_variable cv_traffic_generator_notification;
std::mutex cv_mutex;

void handle_start_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number);
void handle_reconfigure_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number, uint32_t mc_number);

void handle_sync_start_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number);
void handle_sync_reconfigure_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number, uint32_t mc_number);

void reconfigure_sw_to_mode(uint32_t mode, uint8_t experiment_number, uint32_t mc_number);

void handle_exit_message(MessageNet_t received_payload);
void handle_sync_exit_message(MessageNet_t received_payload);

std::vector<std::pair<std::string, int>> check_interfaces();


/*
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


/*
*
*/
int main(int argc, char* argv[])
{
    RM_logInfo("Wired RM client switch is starting...")
    rscmng::init_rm_file_log(std::string("rm_client_sw_"), " ");
    signal(SIGINT, signalHandler);  


    // read input
    if (argc != 2)
    {
        std::cerr << "Require Host ID" << std::endl;
        return 1;
    }
    std::string host_name = argv[1];

    // load config
    ConfigReader config_reader;
    struct rscmng::config::unit_settings client_configuration  = config_reader.load_unit_settings(host_name, DEFAULT_CONFIG);
    struct rscmng::config::experiment_parameter experiment_parameter = config_reader.load_experiment_settings(DEFAULT_CONFIG);
    RM_logInfo("RM client switch Load config done.") 


    // initialize variables
    // Storage for payload sent to RM
    MessageNet_t rm_payload_sendout;
    // payload
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    RM_logInfo("RM client switchVariables init done.") 

    
    // Initialize RM client
    RMAbstraction rm_client(client_configuration, cv_rm_notification);

    rm_client.rm_handler_start();
    RM_logInfo("RM client started ...")

    struct timespec time_now = {0,0};
    clock_gettime(CLOCK_REALTIME, &time_now);
    RM_logInfo("RM client switch started on timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")

    init_done = true;    
    RM_logInfo(" ")

    sleep(2);

    std::vector<std::pair<std::string, int>> slow_interfaces_vector;
    slow_interfaces_vector = check_interfaces();
    if (!slow_interfaces_vector.empty()) 
    {
        //RM_logInfo("RM client switch Inferface error")
        for (const auto& entry : slow_interfaces_vector) 
        {
            const std::string& interface = entry.first;
            int speed = entry.second;
            uint32_t interface_id = static_cast<uint32_t>(std::hash<std::string>{}(interface));
            //RM_logInfo("RM client switch sending")
            //rm_client.send_error(static_cast<uint32_t>(interface_id), static_cast<uint32_t>(speed));
            RM_logInfo("RM client switch ERROR at" << interface << " with speed: " << speed << " Mb/s")
        }        
    }
    
    rm_client.sync_request(&rm_payload_sendout, client_configuration.client_id);
    
    while(active)
    {
        std::unique_lock<std::mutex> lock(cv_mutex);
        cv_rm_notification.wait(lock);
        //if (active) 
        //{    
            while (active && rm_client.check_next_control_message_aviable() == false)
            {
                rscmng::wired::RMMessage recieved_control_message = rm_client.get_next_control_message();
                MessageNet_t rm_payload_received;

                rm_payload_received.add(recieved_control_message.payload, recieved_control_message.payload_size);
                rm_payload_received.reset();

                struct timespec timestamp_target_restart = {0,0};
                uint8_t experiment_number = experiment_parameter.experiment_number;

                switch(recieved_control_message.message_type)
                {                
                    case MessageTypes::RM_CLIENT_START:
                        RM_logInfo("RM Message RM_CLIENT_START")
                        handle_start_message(rm_payload_received, recieved_control_message.mode, experiment_number);
                        break;

                    case MessageTypes::RM_CLIENT_RECONFIGURE_HW:
                        RM_logInfo("RM Message RM_CLIENT_RECONFIGURE_HW")
                        rm_client.sync_ack_receive(&rm_payload_sendout, client_configuration.client_id);
                        handle_reconfigure_message(rm_payload_received, recieved_control_message.mode, experiment_number, recieved_control_message.destination_id);
                        rm_client.sync_ack_reconfigure_done(&rm_payload_sendout, client_configuration.client_id);
                        break;
                        
                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_START:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_START")
                        rm_client.sync_ack_receive(&rm_payload_sendout, client_configuration.client_id);
                        handle_sync_start_message(rm_payload_received, recieved_control_message.mode, experiment_number);
                        break;

                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE_HW:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE_HW")
                        rm_client.sync_ack_receive(&rm_payload_sendout, client_configuration.client_id);
                        handle_sync_reconfigure_message(rm_payload_received, recieved_control_message.mode, experiment_number, recieved_control_message.destination_id);
                        rm_client.sync_ack_reconfigure_done(&rm_payload_sendout, client_configuration.client_id);
                        break;

                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE_SYNC_OBJECT_HW:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE_SYNC_OBJECT_HW")
                        rm_client.sync_ack_receive(&rm_payload_sendout, client_configuration.client_id);
                        handle_sync_reconfigure_message(rm_payload_received, recieved_control_message.mode, experiment_number, recieved_control_message.destination_id);
                        rm_client.sync_ack_reconfigure_done(&rm_payload_sendout, client_configuration.client_id);
                        break;

                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_EXIT:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_EXIT")
                        rm_client.sync_ack_receive(&rm_payload_sendout, client_configuration.client_id);
                        handle_sync_exit_message(rm_payload_received);
                        break;
                        
                    case MessageTypes::RM_CLIENT_EXIT:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_EXIT")
                        rm_client.sync_ack_receive(&rm_payload_sendout, client_configuration.client_id);
                        handle_exit_message(rm_payload_received);
                        break;
                    
                    default:
                        RM_logInfo("RM client unknown message type arrived: " << recieved_control_message.message_type)
                        break;
                };            
            //}                                                  
        }
    }
    RM_logInfo("RMClient close")
    rm_client.close();

    RM_logInfo("RMClient shutdown Bye.")
    exit(0);
    return 0;
}


/*
*
*/
void handle_start_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number)
{
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;     
    rm_client_protocol_payload.deserialize(&received_payload);

    reconfigure_sw_to_mode(mode, experiment_number, 0);

    return;
}


/*
*
*/
void handle_reconfigure_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number, uint32_t mc_number)
{
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;     
    rm_client_protocol_payload.deserialize(&received_payload);

    reconfigure_sw_to_mode(mode, experiment_number, mc_number);

    return;
}


/*
*
*/
void handle_sync_start_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number)
{
    struct timespec time_now = {0,0};
    struct timespec timestamp_target_restart = {0,0};

    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;     
    rm_client_protocol_payload.deserialize(&received_payload);

    timestamp_target_restart = rm_client_protocol_payload.get_timestamp_start();

    RM_logInfo("timestamp received restart  : " << timestamp_target_restart.tv_sec  << " s, " << timestamp_target_restart.tv_nsec  << " ns")

    bool started = false;

    while(true)
    {            
        clock_gettime(CLOCK_REALTIME, &time_now);

        if (started == false)
        {
            if (timestamp_target_restart.tv_sec != 0 && timestamp_target_restart.tv_sec >= time_now.tv_sec)
            {
                if (time_now.tv_sec > timestamp_target_restart.tv_sec || (time_now.tv_sec == timestamp_target_restart.tv_sec && time_now.tv_nsec > timestamp_target_restart.tv_nsec))
                {        
                    RM_logInfo("switch started on timestamp : " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    reconfigure_sw_to_mode(mode, experiment_number, 0);
                    started = true;
                    break;
                }
            }
            else 
            {
                RM_logInfo("timestamps invalid! current time is " << time_now.tv_sec << " s " << time_now.tv_nsec << " ns")
                break;
            }
        }
    }
    return;
}


/*
*
*/
void handle_sync_reconfigure_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number, uint32_t mc_number)
{
    struct timespec time_now = {0,0};
    struct timespec timestamp_target_stop = {0,0};
    struct timespec timestamp_target_reconfig = {0,0};
    struct timespec timestamp_target_restart = {0,0};

    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;     
    rm_client_protocol_payload.deserialize(&received_payload);
    //rm_client_protocol_payload.print();

    timestamp_target_restart = rm_client_protocol_payload.get_timestamp_start();
    timestamp_target_reconfig = rm_client_protocol_payload.get_timestamp_reconfig();
    timestamp_target_stop = rm_client_protocol_payload.get_timestamp_stop();

    clock_gettime(CLOCK_REALTIME, &time_now);
    RM_logInfo("timestamp current time      : " << time_now.tv_sec                  << " s, " << time_now.tv_nsec                  << " ns")
    RM_logInfo("timestamp received stop     : " << timestamp_target_stop.tv_sec     << " s, " << timestamp_target_stop.tv_nsec     << " ns")
    RM_logInfo("timestamp received reconfig : " << timestamp_target_reconfig.tv_sec << " s, " << timestamp_target_reconfig.tv_nsec << " ns")
    RM_logInfo("timestamp received restart  : " << timestamp_target_restart.tv_sec  << " s, " << timestamp_target_restart.tv_nsec  << " ns")

    bool reconfigured = false;

    while(true)
    {            
        clock_gettime(CLOCK_REALTIME, &time_now);

        if (reconfigured == false)
        {
            if (timestamp_target_reconfig.tv_sec != 0 && timestamp_target_reconfig.tv_sec >= time_now.tv_sec)
            {
                if (time_now.tv_sec > timestamp_target_reconfig.tv_sec || (time_now.tv_sec == timestamp_target_reconfig.tv_sec && time_now.tv_nsec > timestamp_target_reconfig.tv_nsec))
                {        
                    RM_logInfo("switch reconfigured on timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    reconfigure_sw_to_mode(mode, experiment_number, mc_number);
                    reconfigured = true;
                    break;
                }
            }
            else 
            {
                RM_logInfo("timestamps invalid! current time is " << time_now.tv_sec << " s " << time_now.tv_nsec << " ns")
                break;
            }
        }
    }
    return;
}


/*
*
*/
void handle_exit_message(MessageNet_t received_payload)
{
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    received_payload.reset();
    rm_client_protocol_payload.deserialize(&received_payload);

    active = false;

    return;
}


/*
*
*/
void handle_sync_exit_message(MessageNet_t received_payload)
{
    struct timespec time_now = {0,0};
    struct timespec timestamp_target_stop = {0,0};

    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    received_payload.reset();
    rm_client_protocol_payload.deserialize(&received_payload);

    timestamp_target_stop = rm_client_protocol_payload.get_timestamp_stop();

    RM_logInfo("Timestamp Received restart s: " << timestamp_target_stop.tv_sec << " ns: " << timestamp_target_stop.tv_nsec)

    bool stopped = false;
    while(true)
    {            
        clock_gettime(CLOCK_REALTIME, &time_now);

        
        if (stopped == false)
        {
            if (timestamp_target_stop.tv_sec != 0 && timestamp_target_stop.tv_sec >= time_now.tv_sec)
            {
                if (time_now.tv_sec > timestamp_target_stop.tv_sec || (time_now.tv_sec == timestamp_target_stop.tv_sec && time_now.tv_nsec > timestamp_target_stop.tv_nsec))
                {        
                    clock_gettime(CLOCK_REALTIME, &time_now);
                    RM_logInfo("endnode stopped with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    //rm_client.send_ack_start();
                    //sendRMMessage(sender_ID, std::ref(socket), destination_endpoint, RECONFDONE, content, mode, iterator);
                    stopped = true;        
                    active = false;        
                    break;
                }
            }
            else 
            {
                RM_logInfo("timestamps invalid! current time is " << time_now.tv_sec << " s " << time_now.tv_nsec << " ns")
                break;
            }
        }
    }

    return;
}


/*
*
*/
void reconfigure_sw_to_mode(uint32_t mode, uint8_t experiment_number, uint32_t mc_number)
{
    struct timespec time_now;
    clock_gettime(CLOCK_REALTIME, &time_now);
    RM_logInfo("Start  Reconfiguration " << std::to_string(mc_number) << " of Mode: " << std::to_string(mode) << " time now: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns");

    char cmd_buf[128];
    if(mode == 0)
    {
        sprintf(cmd_buf,"sh %sexperiment_%i%s.sh",SCRIPT_PATH_BASE,experiment_number,SCRIPT_MODE00);
    }
    else if(mode == 01)
    {
        sprintf(cmd_buf,"sh %sexperiment_%i%s.sh",SCRIPT_PATH_BASE,experiment_number,SCRIPT_MODE01);
    }
    else if(mode == 10)
    {
        sprintf(cmd_buf,"sh %sexperiment_%i%s.sh",SCRIPT_PATH_BASE,experiment_number,SCRIPT_MODE10);
    }
    else
    {
        RM_logInfo("ERROR Received an unknown RECONFIGURATION command to mode: " << mode)
        return;
    }
    system(cmd_buf);
    clock_gettime(CLOCK_REALTIME, &time_now);
    RM_logInfo("Finish Reconfiguration " << std::to_string(mc_number) << " of Mode: " << std::to_string(mode) << " time now: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns");
    RM_logInfo("RM client execution of script: " << cmd_buf)
}


/*
*
*/
std::vector<std::pair<std::string, int>> check_interfaces()
{
    const std::string basePath = "/sys/class/net/";
    std::vector<std::pair<std::string, int>> slow_interfaces;

    DIR* dir = opendir(basePath.c_str());
    if (!dir) 
    {
        RM_logInfo("Could not read path dir")
        return slow_interfaces;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) 
    {
        std::string interface(entry->d_name);

        // Skip "." and ".."
        if (interface == "." || interface == "..")
            continue;

        // Only check interfaces starting with "enp"
        if (interface.rfind("enp", 0) == 0) {
            std::string speedPath = basePath + interface + "/speed";
            std::ifstream speedFile(speedPath);

            if (!speedFile.is_open()) 
            {
                std::cerr << "Could not read speed for interface: " << interface << "\n";
                continue;
            }

            int speed = 0;
            speedFile >> speed;

            if (speed == 100) 
            {
                RM_logInfo("ERROR: Interface only running at 100 Mb/s " << interface)
                slow_interfaces.emplace_back(interface, speed);                
            } 
        }
    }
    return slow_interfaces;
}