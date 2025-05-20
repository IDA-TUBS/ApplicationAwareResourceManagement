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


bool active = true;
bool init_done = false;

// CV for rm communication
std::condition_variable cv_rm_notification;
std::condition_variable cv_traffic_generator_notification;
std::mutex cv_mutex;


void handle_message(MessageNet_t received_payload);
void handle_start_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator);
void handle_stop_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator);
void handle_exit_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator);
void handle_pause_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator);
void handle_reconfigure_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator, uint32_t mode);
void handle_reconfigure_message_soft(MessageNet_t received_payload, TrafficGenerator &traffic_generator, uint32_t mode);

void handle_sync_start_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator);
void handle_sync_stop_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator);
void handle_sync_exit_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator);
void handle_sync_pause_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator);
void handle_sync_reconfigure_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator, uint32_t mode);
void handle_sync_reconfigure_soft_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator, uint32_t mode);



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
    RM_logInfo("Wired RM client endnode is starting...")
    rscmng::init_rm_file_log(std::string("rm_client_sg_"), " ");
    signal(SIGINT, signalHandler);  


    // read input
    if (argc != 3)
    {
        std::cerr << "Require endnode ID and service ID" << std::endl;
        return 1;
    }
    std::string host_name = argv[1];
    uint32_t service_id_int = std::stoi(argv[2]);


    // load config
    ConfigReader config_reader;
    struct rscmng::config::unit_settings client_configuration  = config_reader.load_unit_settings(host_name, DEFAULT_CONFIG);
    std::map<uint32_t, struct rscmng::config::service_settings> service_configuration = config_reader.load_service_settings(service_id_int, DEFAULT_CONFIG);
    struct rscmng::config::experiment_parameter experiment_parameter = config_reader.load_experiment_settings(DEFAULT_CONFIG);

    RM_logInfo("Load config done.") 

    // initialize variables
    uint32_t client_id = client_configuration.client_id;
    serviceID_t service_id = static_cast<serviceID_t>(service_id_int);
    // Storage for payload sent to RM
    MessageNet_t rm_payload_sendout;
    // payload
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    RM_logInfo("Variables init done.") 

    
    // Initialize RM client
    RMAbstraction rm_client(client_configuration, cv_rm_notification);


    traffic_generator_parameter traffic_pattern;
    traffic_pattern.info_flag = false;
    traffic_pattern.period = std::chrono::milliseconds(service_configuration[0].deadline);
    traffic_pattern.object_size_kb = service_configuration[0].object_size;
    traffic_pattern.inter_object_gap = service_configuration[0].inter_object_gap;
    traffic_pattern.inter_packet_gap = service_configuration[0].inter_packet_gap;

    TrafficSourceType traffic_source_type = TrafficSourceType::OBJECT_BURST_SHAPED_IP;
    if (service_configuration[0].source_type == "OBJECT_BURST_SHAPED_IP")
    {
        traffic_source_type = TrafficSourceType::OBJECT_BURST_SHAPED_IP;
    }
    else if (service_configuration[0].source_type == "OBJECT_BURST_SHAPED")
    {
        traffic_source_type = TrafficSourceType::OBJECT_BURST_SHAPED;
    }

    TrafficGenerator traffic_generator(service_configuration, client_configuration, traffic_source_type, traffic_pattern);

    struct rscmng::config::service_settings service_setting_mode_0 = service_configuration[0];
    rscmng::rm_wired_basic_protocol::RMPayload::network_resource_request client_resources_request_mode_0;
    client_resources_request_mode_0.client_id = client_configuration.client_id;
    client_resources_request_mode_0.service_id = service_setting_mode_0.service_id;
    client_resources_request_mode_0.data_path = service_setting_mode_0.data_path;
    client_resources_request_mode_0.priority = service_setting_mode_0.service_priority;
    client_resources_request_mode_0.bandwidth = service_setting_mode_0.object_size;
    client_resources_request_mode_0.deadline = std::chrono::milliseconds(service_setting_mode_0.deadline);

    rm_client.rm_handler_start();
    RM_logInfo("RM client started ...")

    traffic_generator.start();
    RM_logInfo("Traffic source started ...")

    init_done = true;    
    RM_logInfo(" ")

    sleep(2);
    
    rscmng::rm_wired_basic_protocol::RMPayload resource_request_payload(
        2000, 
        std::chrono::microseconds(100000), 
        client_configuration.client_priority,
        0,
        RMCommand::IDLE,
        {0,0},
        {0,0},
        {0,0},
        client_resources_request_mode_0        
        );

    resource_request_payload.serialize(&rm_payload_sendout);
    rm_client.sync_request(&rm_payload_sendout, service_id);


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

                switch(recieved_control_message.message_type)
                {                
                    case MessageTypes::RM_CLIENT_START:
                        RM_logInfo("RM Message RM_CLIENT_START")
                        handle_start_message(rm_payload_received, traffic_generator);
                        break;

                    case MessageTypes::RM_CLIENT_STOP:
                        RM_logInfo("RM Message RM_CLIENT_STOP")
                        rm_client.sync_ack_receive(&rm_payload_sendout, service_id);
                        handle_stop_message(rm_payload_received, traffic_generator);
                        break;

                    case MessageTypes::RM_CLIENT_PAUSE:
                        RM_logInfo("RM Message RM_CLIENT_PAUSE")
                        rm_client.sync_ack_receive(&rm_payload_sendout, service_id);
                        handle_pause_message(rm_payload_received, traffic_generator);
                        break;

                    case MessageTypes::RM_CLIENT_RECONFIGURE:
                        rm_client.sync_ack_receive(&rm_payload_sendout, service_id);
                        if (experiment_parameter.experiment_number == 2)
                        {
                            RM_logInfo("RM Message RM_CLIENT_RECONFIGURE Experiment 2")
                            handle_reconfigure_message(rm_payload_received, traffic_generator, recieved_control_message.mode);
                        }
                        else if (experiment_parameter.experiment_number == 3)
                        {
                            RM_logInfo("RM Message RM_CLIENT_RECONFIGURE Experiment 3")
                            handle_reconfigure_message_soft(rm_payload_received, traffic_generator, recieved_control_message.mode);
                        }
                        else
                        {
                            RM_logInfo("RM Message RM_CLIENT_RECONFIGURE with unknown experiment: "<< experiment_parameter.experiment_number)

                        }
                        rm_client.sync_ack_reconfigure_done(&rm_payload_sendout, service_id);
                        break;
                        
                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_START:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_START")
                        rm_client.sync_ack_receive(&rm_payload_sendout, service_id);
                        handle_sync_start_message(rm_payload_received, traffic_generator);
                        break;
                    
                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_STOP:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_STOP")
                        rm_client.sync_ack_receive(&rm_payload_sendout, service_id);
                        handle_sync_stop_message(rm_payload_received, traffic_generator);
                        break;

                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_PAUSE:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_PAUSE")
                        rm_client.sync_ack_receive(&rm_payload_sendout, service_id);
                        handle_sync_pause_message(rm_payload_received, traffic_generator);
                        break;

                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE:                    
                        rm_client.sync_ack_receive(&rm_payload_sendout, service_id);
                        if (experiment_parameter.experiment_number == 2)
                        {
                            RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE Experiment 2")
                            handle_sync_reconfigure_message(rm_payload_received, traffic_generator, recieved_control_message.mode);
                        }
                        else if (experiment_parameter.experiment_number == 3)
                        {
                            RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE Experiment 3")
                            handle_sync_reconfigure_soft_message(rm_payload_received, traffic_generator, recieved_control_message.mode);
                        }
                        else
                        {
                            RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE with unknown experiment: "<< experiment_parameter.experiment_number)

                        }
                        rm_client.sync_ack_reconfigure_done(&rm_payload_sendout, service_id);
                        break;

                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE_SOFT:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE_SOFT")
                        rm_client.sync_ack_receive(&rm_payload_sendout, service_id);
                        rm_client.sync_ack_reconfigure_done(&rm_payload_sendout, service_id);
                        break;

                    case MessageTypes::RM_CLIENT_SYNC_TIMESTAMP_EXIT:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_TIMESTAMP_EXIT")
                        rm_client.sync_ack_receive(&rm_payload_sendout, service_id);
                        handle_sync_exit_message(rm_payload_received, traffic_generator);
                        break;
                        
                    case MessageTypes::RM_CLIENT_EXIT:
                        RM_logInfo("RM Message RM_CLIENT_SYNC_EXIT")
                        rm_client.sync_ack_receive(&rm_payload_sendout, service_id);
                        handle_exit_message(rm_payload_received, traffic_generator);
                        break;

                    default:
                        RM_logInfo("RM client unknown message type arrived: " << recieved_control_message.message_type)
                        break;
                };   
            //}                                                             
        }
    }
    RM_logInfo("RMClient release resources")
    rm_client.resource_release(service_id);
    rm_client.close();

    RM_logInfo("RMClient shutdown bye.")
    exit(0);
    return 0;
}


/*
*
*/
void handle_message(MessageNet_t received_payload)
{
    struct timespec time_now = {0,0};
    struct timespec timestamp_target_stop = {0,0};
    struct timespec timestamp_target_reconfig = {0,0};
    struct timespec timestamp_target_restart = {0,0};

    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    received_payload.reset();
    rm_client_protocol_payload.deserialize(&received_payload);

    timestamp_target_restart = rm_client_protocol_payload.get_timestamp_start();
    timestamp_target_reconfig = rm_client_protocol_payload.get_timestamp_reconfig();
    timestamp_target_stop = rm_client_protocol_payload.get_timestamp_stop();

    //rm_client_protocol_payload.print();
    RM_logInfo("Timestamp Received restart s: " << timestamp_target_restart.tv_sec << " ns: " << timestamp_target_restart.tv_nsec)
    RM_logInfo("Timestamp Received stop s: " << timestamp_target_stop.tv_sec << " ns: " << timestamp_target_stop.tv_nsec)
    RM_logInfo("Timestamp Received reconfig s: " << timestamp_target_reconfig.tv_sec << " ns: " << timestamp_target_reconfig.tv_nsec)

    return;
}


/*
*
*/
void handle_start_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator)
{
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    received_payload.reset();
    rm_client_protocol_payload.deserialize(&received_payload);

    traffic_generator.notify_generator(THREAD_TRANSMISSION);

    return;
}


/*
*
*/
void handle_stop_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator)
{
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    received_payload.reset();
    rm_client_protocol_payload.deserialize(&received_payload);

    traffic_generator.notify_generator(THREAD_STOP);

    return;
}


/*
*
*/
void handle_exit_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator)
{
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    received_payload.reset();
    rm_client_protocol_payload.deserialize(&received_payload);

    traffic_generator.notify_generator(THREAD_STOP);

    active = false;

    return;
}


/*
*
*/
void handle_pause_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator)
{
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    received_payload.reset();
    rm_client_protocol_payload.deserialize(&received_payload);

    traffic_generator.notify_generator(THREAD_PAUSED);

    return;
}


/*
*
*/
void handle_reconfigure_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator, uint32_t mode)
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

    RM_logInfo("Timestamp Received stop     : " << timestamp_target_stop.tv_sec     << " s, " << timestamp_target_stop.tv_nsec     << " ns")
    RM_logInfo("Timestamp Received reconfig : " << timestamp_target_reconfig.tv_sec << " s, " << timestamp_target_reconfig.tv_nsec << " ns")
    RM_logInfo("Timestamp Received restart  : " << timestamp_target_restart.tv_sec  << " s, " << timestamp_target_restart.tv_nsec  << " ns")

    bool stopped = false;
    bool reconfigured = false;
    bool restarted = false;
    while(true)
    {            
        clock_gettime(CLOCK_REALTIME, &time_now);

        if (stopped == false)
        {
            if (timestamp_target_stop.tv_sec != 0 && timestamp_target_stop.tv_sec >= time_now.tv_sec)
            {
                if (time_now.tv_sec > timestamp_target_stop.tv_sec || (time_now.tv_sec == timestamp_target_stop.tv_sec && time_now.tv_nsec > timestamp_target_stop.tv_nsec))
                {        
                    traffic_generator.notify_generator(THREAD_PAUSED);                   
                    RM_logInfo("process stopped with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    stopped = true;
                }
            }
            else 
            {
                RM_logInfo("timestamps invalid! current time is " << time_now.tv_sec << " s " << time_now.tv_nsec << " ns")
                break;
            }
        }
        
        if (reconfigured == false)
        {
            if (timestamp_target_reconfig.tv_sec != 0 && timestamp_target_reconfig.tv_sec >= time_now.tv_sec)
            {
                if (time_now.tv_sec > timestamp_target_reconfig.tv_sec || (time_now.tv_sec == timestamp_target_reconfig.tv_sec && time_now.tv_nsec > timestamp_target_reconfig.tv_nsec))
                {        
                    traffic_generator.notify_generator_mode_change(THREAD_RECONFIGURE, mode);                   
                    RM_logInfo("process reconfigured with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    reconfigured = true;
                }
            }
            else 
            {
                RM_logInfo("timestamps invalid! current time is " << time_now.tv_sec << " s " << time_now.tv_nsec << " ns")
                break;
            }
        }
        
        if (restarted == false)
        {
            if (timestamp_target_restart.tv_sec != 0 && timestamp_target_restart.tv_sec >= time_now.tv_sec)
            {
                if (time_now.tv_sec > timestamp_target_restart.tv_sec || (time_now.tv_sec == timestamp_target_restart.tv_sec && time_now.tv_nsec > timestamp_target_restart.tv_nsec))
                {        
                    traffic_generator.notify_generator(THREAD_TRANSMISSION);                   
                    RM_logInfo("process started with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    restarted = true;
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
void handle_reconfigure_message_soft(MessageNet_t received_payload, TrafficGenerator &traffic_generator, uint32_t mode)
{
    struct timespec time_now = {0,0};

    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    received_payload.reset();
    rm_client_protocol_payload.deserialize(&received_payload);

    clock_gettime(CLOCK_REALTIME, &time_now);
    //traffic_generator.notify_generator(THREAD_RECONFIGURE);
    traffic_generator.notify_generator_mode_change(THREAD_RECONFIGURE, mode);                   
    RM_logInfo("process reconfigured with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
    return;
}


/*
*
*/
void handle_sync_start_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator)
{
    struct timespec time_now = {0,0};
    struct timespec timestamp_target_restart = {0,0};

    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;     
    rm_client_protocol_payload.deserialize(&received_payload);

    timestamp_target_restart = rm_client_protocol_payload.get_timestamp_start();

    RM_logInfo("Timestamp Received restart  : " << timestamp_target_restart.tv_sec  << " s, " << timestamp_target_restart.tv_nsec  << " ns")
    
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
                    traffic_generator.notify_generator(THREAD_TRANSMISSION);                   
                    clock_gettime(CLOCK_REALTIME, &time_now);
                    RM_logInfo("endnode started with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    //rm_client.send_ack_start();
                    //sendRMMessage(sender_ID, std::ref(socket), destination_endpoint, RECONFDONE, content, mode, iterator);
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
void handle_sync_stop_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator)
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
                    traffic_generator.notify_generator(THREAD_STOP);                   
                    clock_gettime(CLOCK_REALTIME, &time_now);
                    RM_logInfo("endnode stopped with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    //rm_client.send_ack_start();
                    //sendRMMessage(sender_ID, std::ref(socket), destination_endpoint, RECONFDONE, content, mode, iterator);
                    stopped = true;
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
void handle_sync_exit_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator)
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
                    traffic_generator.notify_generator(THREAD_STOP);                   
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
    RM_logInfo("endnode stopped")
    return;
}


/*
*
*/
void handle_sync_pause_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator)
{
    struct timespec time_now = {0,0};
    struct timespec timestamp_target_stop = {0,0};

    rscmng::rm_wired_basic_protocol::RMPayload rm_client_protocol_payload;
    received_payload.reset();
    rm_client_protocol_payload.deserialize(&received_payload);

    timestamp_target_stop = rm_client_protocol_payload.get_timestamp_stop();

    RM_logInfo("Timestamp Received restart s: " << timestamp_target_stop.tv_sec << " ns: " << timestamp_target_stop.tv_nsec)

    bool paused = false;
    while(true)
    {            
        clock_gettime(CLOCK_REALTIME, &time_now);
        
        if (paused == false)
        {
            if (timestamp_target_stop.tv_sec != 0 && timestamp_target_stop.tv_sec >= time_now.tv_sec)
            {
                if (time_now.tv_sec > timestamp_target_stop.tv_sec || (time_now.tv_sec == timestamp_target_stop.tv_sec && time_now.tv_nsec > timestamp_target_stop.tv_nsec))
                {        
                    traffic_generator.notify_generator(THREAD_PAUSED);                  
                    clock_gettime(CLOCK_REALTIME, &time_now);
                    RM_logInfo("endnode paused with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    //rm_client.send_ack_start();
                    //sendRMMessage(sender_ID, std::ref(socket), destination_endpoint, RECONFDONE, content, mode, iterator);
                    paused = true;
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
void handle_sync_reconfigure_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator, uint32_t mode)
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

    RM_logInfo("Timestamp Received stop     : " << timestamp_target_stop.tv_sec     << " s, " << timestamp_target_stop.tv_nsec     << " ns")
    RM_logInfo("Timestamp Received reconfig : " << timestamp_target_reconfig.tv_sec << " s, " << timestamp_target_reconfig.tv_nsec << " ns")
    RM_logInfo("Timestamp Received restart  : " << timestamp_target_restart.tv_sec  << " s, " << timestamp_target_restart.tv_nsec  << " ns")

    bool stopped = false;
    bool reconfigured = false;
    bool restarted = false;
    while(true)
    {            
        clock_gettime(CLOCK_REALTIME, &time_now);

        if (stopped == false)
        {
            if (timestamp_target_stop.tv_sec != 0 && timestamp_target_stop.tv_sec >= time_now.tv_sec)
            {
                if (time_now.tv_sec > timestamp_target_stop.tv_sec || (time_now.tv_sec == timestamp_target_stop.tv_sec && time_now.tv_nsec > timestamp_target_stop.tv_nsec))
                {        
                    traffic_generator.notify_generator(THREAD_PAUSED);                   
                    RM_logInfo("process stopped with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    stopped = true;
                }
            }
            else 
            {
                RM_logInfo("timestamps invalid! current time is " << time_now.tv_sec << " s " << time_now.tv_nsec << " ns")
                break;
            }
        }
        
        if (reconfigured == false)
        {
            if (timestamp_target_reconfig.tv_sec != 0 && timestamp_target_reconfig.tv_sec >= time_now.tv_sec)
            {
                if (time_now.tv_sec > timestamp_target_reconfig.tv_sec || (time_now.tv_sec == timestamp_target_reconfig.tv_sec && time_now.tv_nsec > timestamp_target_reconfig.tv_nsec))
                {        
                    traffic_generator.notify_generator_mode_change(THREAD_RECONFIGURE, mode);                   
                    RM_logInfo("process reconfigured with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    reconfigured = true;
                }
            }
            else 
            {
                RM_logInfo("timestamps invalid! current time is " << time_now.tv_sec << " s " << time_now.tv_nsec << " ns")
                break;
            }
        }
        
        if (restarted == false)
        {
            if (timestamp_target_restart.tv_sec != 0 && timestamp_target_restart.tv_sec >= time_now.tv_sec)
            {
                if (time_now.tv_sec > timestamp_target_restart.tv_sec || (time_now.tv_sec == timestamp_target_restart.tv_sec && time_now.tv_nsec > timestamp_target_restart.tv_nsec))
                {        
                    traffic_generator.notify_generator(THREAD_TRANSMISSION);                   
                    RM_logInfo("process started with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
                    restarted = true;
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
void handle_sync_reconfigure_soft_message(MessageNet_t received_payload, TrafficGenerator &traffic_generator, uint32_t mode)
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

    RM_logInfo("Timestamp Received stop     : " << timestamp_target_stop.tv_sec     << " s, " << timestamp_target_stop.tv_nsec     << " ns")
    RM_logInfo("Timestamp Received reconfig : " << timestamp_target_reconfig.tv_sec << " s, " << timestamp_target_reconfig.tv_nsec << " ns")
    RM_logInfo("Timestamp Received restart  : " << timestamp_target_restart.tv_sec  << " s, " << timestamp_target_restart.tv_nsec  << " ns")

    bool stopped = false;
    bool reconfigured = false;
    bool restarted = false;
    while(true)
    {            
        clock_gettime(CLOCK_REALTIME, &time_now);

        // if (stopped == false)
        // {
        //     if (timestamp_target_stop.tv_sec != 0 && timestamp_target_stop.tv_sec >= time_now.tv_sec)
        //     {
        //         if (time_now.tv_sec > timestamp_target_stop.tv_sec || (time_now.tv_sec == timestamp_target_stop.tv_sec && time_now.tv_nsec > timestamp_target_stop.tv_nsec))
        //         {        
        //             traffic_generator.notify_generator(THREAD_PAUSED);                   
        //             RM_logInfo("process stopped with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
        //             stopped = true;
        //         }
        //     }
        //     else 
        //     {
        //         RM_logInfo("timestamps invalid! current time is " << time_now.tv_sec << " s " << time_now.tv_nsec << " ns")
        //         break;
        //     }
        // }
        
        if (reconfigured == false)
        {
            if (timestamp_target_reconfig.tv_sec != 0 && timestamp_target_reconfig.tv_sec >= time_now.tv_sec)
            {
                if (time_now.tv_sec > timestamp_target_reconfig.tv_sec || (time_now.tv_sec == timestamp_target_reconfig.tv_sec && time_now.tv_nsec > timestamp_target_reconfig.tv_nsec))
                {        
                    traffic_generator.notify_generator_mode_change(THREAD_RECONFIGURE, mode);                   
                    RM_logInfo("process reconfigured with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
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
        
        // if (restarted == false)
        // {
        //     if (timestamp_target_restart.tv_sec != 0 && timestamp_target_restart.tv_sec >= time_now.tv_sec)
        //     {
        //         if (time_now.tv_sec > timestamp_target_restart.tv_sec || (time_now.tv_sec == timestamp_target_restart.tv_sec && time_now.tv_nsec > timestamp_target_restart.tv_nsec))
        //         {        
        //             traffic_generator.notify_generator(THREAD_TRANSMISSION);                   
        //             RM_logInfo("process started with timestamp: " << time_now.tv_sec << " s " << time_now.tv_nsec  << " ns")
        //             restarted = true;
        //             break;
        //         }
        //     }
        //     else 
        //     {
        //         RM_logInfo("timestamps invalid! current time is " << time_now.tv_sec << " s " << time_now.tv_nsec << " ns")
        //         break;
        //     }
        // }
    }

    return;
}
