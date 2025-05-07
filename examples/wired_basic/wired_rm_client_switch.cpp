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
#include <unistd.h>
#include <mutex>
#include <condition_variable>

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
void handle_reconfigure_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number);

void handle_sync_start_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number);
void handle_sync_reconfigure_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number);

void reconfigure_sw_to_mode(uint32_t mode, uint8_t experiment_number);


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
    RM_logInfo("Load config done.") 


    // initialize variables
    // Storage for payload sent to RM
    MessageNet_t rm_payload_sendout;
    // payload
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    RM_logInfo("Variables init done.") 

    
    // Initialize RM client
    RMAbstraction rm_client(client_configuration, cv_rm_notification);

    rm_client.rm_handler_start();
    RM_logInfo("RM client started ...")

    struct timespec time_now = {0,0};
    clock_gettime(CLOCK_REALTIME, &time_now);
    RM_logInfo("switch started on timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")

    init_done = true;    
    RM_logInfo(" ")

    sleep(2);

    rm_client.sync_request(&rm_payload_sendout, client_configuration.client_id);
    
    while(active)
    {
        std::unique_lock<std::mutex> lock(cv_mutex);
        cv_rm_notification.wait(lock);
        if (active) 
        {    
            while (rm_client.check_next_control_message_aviable() == false)
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

                    case MessageTypes::RM_CLIENT_RECONFIGURE:
                        RM_logInfo("RM Message RM_CLIENT_RECONFIGURE")
                        rm_client.sync_ack_receive(&rm_payload_sendout, client_configuration.client_id);
                        handle_reconfigure_message(rm_payload_received, recieved_control_message.mode, experiment_number);
                        rm_client.sync_ack_reconfigure_done(&rm_payload_sendout, client_configuration.client_id);
                        break;
                        
                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_START:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_START")
                        rm_client.sync_ack_receive(&rm_payload_sendout, client_configuration.client_id);
                        handle_sync_start_message(rm_payload_received, recieved_control_message.mode, experiment_number);
                        break;
                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_STOP:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_STOP")
                        break;
                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE")
                        rm_client.sync_ack_receive(&rm_payload_sendout, client_configuration.client_id);
                        handle_sync_reconfigure_message(rm_payload_received, recieved_control_message.mode, experiment_number);
                        rm_client.sync_ack_reconfigure_done(&rm_payload_sendout, client_configuration.client_id);
                        break;
                    
                    default:
                        RM_logInfo("RM client unknown message type arrived: " << recieved_control_message.message_type)
                        break;
                };            
            }                                                  
        }
    }
    rm_client.close();

    return 0;
}


/*
*
*/
void handle_start_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number)
{
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;     
    rm_client_protocol_payload.deserialize(&received_payload);
    //rm_client_protocol_payload.print();

    reconfigure_sw_to_mode(mode, experiment_number);

    return;
}


/*
*
*/
void handle_reconfigure_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number)
{
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;     
    rm_client_protocol_payload.deserialize(&received_payload);
    //rm_client_protocol_payload.print();

    reconfigure_sw_to_mode(mode, experiment_number);

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
                    reconfigure_sw_to_mode(mode, experiment_number);
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
void handle_sync_reconfigure_message(MessageNet_t received_payload, uint32_t mode, uint8_t experiment_number)
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
                    reconfigure_sw_to_mode(mode, experiment_number);
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
void reconfigure_sw_to_mode(uint32_t mode, uint8_t experiment_number)
{
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
    RM_logInfo("RM client execution of script: " << cmd_buf)
}