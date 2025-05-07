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


using namespace rscmng;
using namespace wired;
using namespace rm_wired_basic_protocol;
using namespace boost::placeholders;


/*
*
*/
template<class T>
NetLayerRM<T>::NetLayerRM
(
    UUID_t id, 
    const uint32_t priority_levels,
    struct rscmng::config::unit_settings local_rm_configuration,
    struct rscmng::network_device_configs network_configuration,
    struct rscmng::config::experiment_parameter experiment_parameter,
    SafeQueue<rscmng::wired::RMMessage> &nRM_in_queue,
    SafeQueue<rscmng::wired::RMMessage> &nRM_out_queue
):
    rm_id(id),
    prio_levels(priority_levels),
    control_channel("0.0.0.0", local_rm_configuration.rm_control_local_port),
    network_environment_config(network_configuration),
    experiment_parameter_struct(experiment_parameter),
    nRM_out_queue(nRM_out_queue),
    nRM_in_queue(nRM_in_queue),
    nRM_in_handle_queue(),
    start_timer(),
    recving_thread{},
    handler_thread{},
    rm_client_config_map(network_configuration.endnode_configuration_map),
    rm_client_service_map(network_configuration.service_configuration_map)
{
    RM_logInfo("NetRM Initializing started...")
    //rscmng::init_rm_file_log(std::string("rm_"), rm_id.get_value());

    recving_thread = std::thread{&NetLayerRM::receive_message, this};
    handler_thread = std::thread{&NetLayerRM::handle_message, this};


    hyperperiod_duration = experiment_parameter_struct.hyperperiod_duration;
    number_slots_per_hyperperiod = experiment_parameter_struct.hyperperiod_slots;
    slot_duration = std::chrono::milliseconds(hyperperiod_duration.count()/number_slots_per_hyperperiod);
    RM_logInfo("NetRM hyperperiod_duration  : " << hyperperiod_duration.count() << " ms")
    RM_logInfo("NetRM number slots          : " << number_slots_per_hyperperiod)
    RM_logInfo("NetRM slot duration         : " << slot_duration.count() << " ms")


    start_timer.start();
    struct timespec time_now = {0,0};
    clock_gettime(CLOCK_REALTIME, &time_now);
    RM_logInfo("Timestamp now                 : " << time_now.tv_sec << "s " << time_now.tv_nsec << " ns")

    std::chrono::system_clock::time_point start_trigger = std::chrono::system_clock::now() + experiment_parameter_struct.client_init_time;
    start_timer.registerTimer(rm_id, start_trigger, false, &NetLayerRM<T>::initial_start_handler, this);
    RM_logInfo("NetRM Initializied.")
}


/*
*/
template<class T>
void NetLayerRM<T>::receive_message()
{
    RM_logInfo("NetRM Receiving Handler active: " << control_channel.address()  << " " << control_channel.port())

    // AppAbstraction app_attr;
    rscmng::wired::RMMessage received_message;
    
    // Store endpoint for request reply
    udp::endpoint sender_endpoint;
    
    // Pair for request cache
    std::pair<rscmng::wired::RMMessage, udp::endpoint> received_request;
   

    while(true)
    {      
        sender_endpoint = control_channel.receive_control_message(received_message);        

        RM_logInfo("message received: " << sender_endpoint.address()  << " " << sender_endpoint.port() << " Type: " << static_cast<uint32_t>(received_message.message_type))

        received_request = {received_message, sender_endpoint};
        nRM_in_handle_queue.enqueue(received_request);
    }
}


/*
*/
template<class T>
void NetLayerRM<T>::handle_message()
{    
    RM_logInfo("NetRM Message Handler active");

    // Pair for request cache
    std::pair<rscmng::wired::RMMessage, udp::endpoint> dequeued_message_tupel;
    rscmng::wired::RMMessage dequeued_message;

    rscmng::basic_protocol::BasicPayload rm_payload_from_client;
    MessageNet_t rm_payload_in;

    FILE *fp;
    std::string filename_str_log = "rm_net_layer.log";
    struct timespec time_received = {0,0};

    while(true)
    {      
        
        dequeued_message_tupel = nRM_in_handle_queue.dequeue();
        dequeued_message = dequeued_message_tupel.first;

        dequeued_message.network_to_rm(&rm_payload_in);
        rm_payload_from_client.deserialize(&rm_payload_in);
                
        switch(dequeued_message.message_type)
        {            
            case RM_CLIENT_SYNC_REQUEST:
                RM_logInfo("RM_CLIENT_SYNC_REQUEST message has arrived: " << dequeued_message.message_type << " from client: " << std::hex << dequeued_message.source_id);
                handle_sync_messages(dequeued_message);                
                break;
            
            case RM_CLIENT_SYNC_RECEIVE:
                RM_logInfo("RM_CLIENT_SYNC_RECEIVE") 
                clock_gettime(CLOCK_REALTIME, &time_received);
                fp = fopen(filename_str_log.c_str(), "a");
                fprintf(fp,"RM_CLIENT_SYNC_RECEIVE received           :%X:%ld:%ld secs %ld nsecs\n", dequeued_message.source_id, dequeued_message.service_id, time_received.tv_sec, time_received.tv_nsec);	
                //fprintf(fp,"RM_CLIENT_SYNC_RECEIVE received           :%ld secs %ld nsecs\n", time_received.tv_sec, time_received.tv_nsec);	             
                fclose(fp);
                break;

            case RM_CLIENT_SYNC_RECONFIGURE_DONE:
                RM_logInfo("RM_CLIENT_SYNC_RECONFIGURE_DONE")
                clock_gettime(CLOCK_REALTIME, &time_received);
                fp = fopen(filename_str_log.c_str(), "a");
                fprintf(fp,"RM_CLIENT_SYNC_RECONFIGURE_DONE received  :%X:%ld:%ld secs %ld nsecs\n", dequeued_message.source_id, dequeued_message.service_id, time_received.tv_sec, time_received.tv_nsec);		             
                fclose(fp);
                break;
            
            default:
                break;
        }
        RM_logInfo("Message handled !");
    }
}


/*
*
*/
template<class T>
void NetLayerRM<T>::handle_sync_messages(rscmng::wired::RMMessage message)
{
    MessageNet_t rm_payload_in;
    rscmng::rm_wired_basic_protocol::RMPayload rm_client_resource_request;
    
    serviceID_t service_id = message.service_id;
    message.network_to_rm(&rm_payload_in);
    rm_client_resource_request.deserialize(&rm_payload_in);

    //rm_client_resource_request.print();
    RM_logInfo("Request sync from client " << std::hex << message.source_id << std::dec << " for service: " << service_id)

    // Check if service_id is already active
    auto service_entry = service_list.find(service_id);
    if(service_entry != service_list.end())
    {
        RM_logInfo("ID is already active");
    }
    else
    {
        service_list.insert(std::make_pair(service_id, message));
        RM_logInfo("New ID is active");
    }
    return;
}


/*
*
*/
template<class T>
void NetLayerRM<T>::initial_start_handler()
{
    RM_logInfo(" ")
    RM_logInfo("NetRM initial start triggered.")

    if (experiment_parameter_struct.synchronous_start_mode)
    {
        RM_logInfo("NetRM start synchronous.")
        start_network_traffic_synchronous();
    }
    else
    {
        RM_logInfo("NetRM start asynchronous.")
        start_network_traffic_asynchronous();
    }    

    std::chrono::system_clock::time_point start_trigger = std::chrono::system_clock::now() + experiment_parameter_struct.experiment_begin_offset;
    RM_logInfo("NetRM start timer for experiment begin. ")
    start_timer.registerTimer(rm_id, start_trigger, false, &NetLayerRM<T>::experiment_mode_change, this);

}


/*
*
*/
template<class T>
void NetLayerRM<T>::start_network_traffic_synchronous()
{
    struct timespec starting_timestamp = get_round_timestamp();
    std::chrono::milliseconds client_start_offset = experiment_parameter_struct.mc_distribution_phase_duration;
    struct timespec target_time_client_start = prepare_timestamp(starting_timestamp, client_start_offset);
    struct timespec time_now = {0,0};
    clock_gettime(CLOCK_REALTIME, &time_now);
    RM_logInfo("Timestamp now               : " << time_now.tv_sec << "s " << time_now.tv_nsec << " ns")
    RM_logInfo("Timestamp for sync start is : " << target_time_client_start.tv_sec << "s " << target_time_client_start.tv_nsec << " ns")
    rm_active_timestamp_last = target_time_client_start;

    rscmng::wired::RMMessage data_message;
    auto iterator = service_list.begin();

    while (iterator != service_list.end()) 
    {
        serviceID_t service_id = iterator->first;        
        data_message = iterator->second;
        ++iterator;

        uint32_t service_id_int =  static_cast<uint32_t>(service_id);
        std::chrono::milliseconds service_offset =  std::chrono::milliseconds(rm_client_service_map[service_id_int][0].offset);
        struct timespec target_time_start_offset = prepare_timestamp(target_time_client_start, service_offset);
        rm_active_timestamp_last = target_time_start_offset;
     
        udp::endpoint target_address = get_client_address(data_message.source_id);

        RM_logInfo("Client IDs             : " << std::hex << data_message.source_id << std::dec << " with target address: " << target_address.address().to_string() << " " << target_address.port() << " and offset: " << service_offset.count() << " ms")
        RM_logInfo("Timestamp for rm client: " << std::hex << data_message.source_id << std::dec << " is: " << target_time_start_offset.tv_sec << "s " << target_time_start_offset.tv_nsec << " ns")

        send_synchronous_start(target_time_start_offset, target_address, service_id);
    }
    return;
}


/*
*
*/
template<class T>
void NetLayerRM<T>::start_network_traffic_asynchronous()
{
    rscmng::wired::RMMessage data_message;
    auto iterator = service_list.begin();

    while (iterator != service_list.end()) 
    {
        serviceID_t service_id = iterator->first;
        data_message = iterator->second;
        ++iterator;
    
        udp::endpoint target_address = get_client_address(data_message.source_id);
        RM_logInfo("Client IDs: " << data_message.source_id <<  " with target address: " << target_address.address().to_string() << " " << target_address.port())

        MessageNet_t rm_message;    
        control_channel.send_start(target_address, demonstrator::RM_ID, service_id, &rm_message);
    }
    return;
}


/*
*
*/
template<class T>
udp::endpoint NetLayerRM<T>::get_client_address(uint32_t endnode_id)
{
    auto it = rm_client_config_map.find(endnode_id);
    if (it != rm_client_config_map.end()) 
    {
        udp::endpoint client_endpoint(boost::asio::ip::address::from_string(it->second.rm_control_local_ip[0]), it->second.rm_control_local_port);
        return client_endpoint;
    } 
    else 
    {
        RM_logInfo("RM nRM Requestet client config not found: " << endnode_id)
        udp::endpoint client_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 25525);
        return client_endpoint;
    }      
}


/*
*
*/
template<class T>
struct timespec NetLayerRM<T>::prepare_timestamp(struct timespec time_stamp, std::chrono::milliseconds delay)
{
    struct timespec target_time = {0, 0};

    // Convert delay to nanoseconds
    auto delay_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(delay);

    // Get the count of nanoseconds as an integer
    long delay_ns_count = delay_ns.count();

    if (delay_ns_count / 1000000000L < 1)
    {
        target_time.tv_sec = time_stamp.tv_sec;
        target_time.tv_nsec = delay_ns_count;
    }
    else
    {
        target_time.tv_sec = time_stamp.tv_sec + (delay_ns_count / 1000000000L);
        target_time.tv_nsec = delay_ns_count % 1000000000L; // Remainder for nanoseconds
    }

    return target_time;
}


/*
*
*/
template<class T>
struct timespec NetLayerRM<T>::get_round_timestamp()
{
    struct timespec current_time = {0, 0};
    clock_gettime(CLOCK_REALTIME, &current_time);

    if (current_time.tv_nsec > 0) 
    {
        current_time.tv_sec += 1;
        current_time.tv_nsec = 0;
    }

    return current_time;
}


/*
*
*/
template<class T>
void NetLayerRM<T>::experiment_mode_change()
{
    if (experiment_parameter_struct.synchronous_mode and experiment_parameter_struct.synchronous_start_mode)
    {
        RM_logInfo("NetRM start experiments synchronous.")   
        experiment_mode_change_synchronous();    
    }
    else if (experiment_parameter_struct.synchronous_mode and experiment_parameter_struct.synchronous_start_mode == false)
    {
        RM_logInfo("NetRM start experiments synchronous.")   
        experiment_mode_change_synchronous_asynchonous_objects();
    }
    else
    {
        RM_logInfo("NetRM start experiments asynchronous.")
        experiment_mode_change_asynchronous();
    }   
}


/*
*
*/
template<class T>
struct timespec NetLayerRM<T>::subtract_timespec(struct timespec end, struct timespec start) 
{
    struct timespec result;
    if ((end.tv_nsec - start.tv_nsec) < 0)
     {
        result.tv_sec = end.tv_sec - start.tv_sec - 1;
        result.tv_nsec = end.tv_nsec - start.tv_nsec + 1000000000;
    } 
    else 
    {
        result.tv_sec = end.tv_sec - start.tv_sec;
        result.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return result;
}


/*
*
*/
template<class T>
void NetLayerRM<T>::experiment_mode_change_synchronous()
{
    RM_logInfo(" ")
    RM_logInfo("NetRM experiment start triggered.") 
    struct timespec time_now = {0,0};
    clock_gettime(CLOCK_REALTIME, &time_now);
    RM_logInfo("Timestamp now : " << time_now.tv_sec << "s " << time_now.tv_nsec << " ns") 
    
    rscmng::wired::RMMessage data_message;
       
    uint32_t network_mode = 0;
    uint32_t configuration_mode = 1;
    
    for(uint8_t iterator = 0; iterator < experiment_parameter_struct.experiment_iterations; iterator++)
    {
        //mode iteration
        for (const auto& mode_order_iterator : experiment_parameter_struct.reconfiguration_order) 
        {
            network_mode = mode_order_iterator;
            // calculate next reconfiguration window
            struct timespec current_time = {0, 0};
            clock_gettime(CLOCK_REALTIME, &current_time);
            
            struct timespec time_diff = subtract_timespec(current_time, rm_active_timestamp_last);

            // Calculate number of hyperperiods
            uint32_t number_hyperperiods_past = (time_diff.tv_sec * 1000 + time_diff.tv_nsec / 1000000) / hyperperiod_duration.count() + 1; //ms
            uint32_t number_hyperperiod_distribution = experiment_parameter_struct.mc_distribution_phase_duration.count() / hyperperiod_duration.count() + 1; //ms

            // Update target_hyperperiod based on hyperperiods
            struct timespec target_hyperperiod;
            target_hyperperiod.tv_sec  = rm_active_timestamp_last.tv_sec  + ((number_hyperperiods_past + number_hyperperiod_distribution) * 100) / 1000;
            target_hyperperiod.tv_nsec = rm_active_timestamp_last.tv_nsec + ((number_hyperperiods_past + number_hyperperiod_distribution) * 100) % 1000 * 1000000;

            rm_active_timestamp_last = target_hyperperiod;

            uint8_t slot_number = calculate_service_slot(network_mode, 100);

            struct timespec target_hyperperiod_mc_begin;
            uint32_t slot_offset_mc_begin = 0;
            if (slot_number + 1 >= number_slots_per_hyperperiod)
            {
                slot_offset_mc_begin =  hyperperiod_duration.count(); //ms
            }
            else
            {
                slot_offset_mc_begin =  (slot_number) * slot_duration.count(); //ms
            }
            
            target_hyperperiod_mc_begin.tv_sec  = target_hyperperiod.tv_sec  + (slot_offset_mc_begin) / 1000;
            target_hyperperiod_mc_begin.tv_nsec = target_hyperperiod.tv_nsec + (slot_offset_mc_begin) % 1000 * 1000000;

            RM_logInfo("NetRM slot offset: " << slot_offset_mc_begin << " new timestamp mode change begin " << target_hyperperiod_mc_begin.tv_sec << " s " << target_hyperperiod_mc_begin.tv_nsec << " ns" ) 

            struct timespec target_time_stop_offset     = prepare_timestamp(target_hyperperiod_mc_begin, experiment_parameter_struct.mc_client_stop_offset);
            struct timespec target_time_reconfig_offset = prepare_timestamp(target_hyperperiod_mc_begin, experiment_parameter_struct.mc_client_reconfig_offset);
            struct timespec target_time_start_offset    = prepare_timestamp(target_hyperperiod_mc_begin, experiment_parameter_struct.mc_client_start_offset);

                 
            RM_logInfo("NetRM experiment network_mode: " << network_mode)   
        
            auto list_iterator = service_list.begin();
            while (list_iterator != service_list.end()) 
            {
                serviceID_t service_id = list_iterator->first;        
                data_message = list_iterator->second;
                ++list_iterator;

                uint32_t client_id = data_message.source_id;        
                auto network_mode_it = experiment_parameter_struct.reconfiguration_map.find(network_mode);
                if (network_mode_it != experiment_parameter_struct.reconfiguration_map.end()) 
                {
                    auto client_id_it = network_mode_it->second.find(client_id);
                    if (client_id_it != network_mode_it->second.end()) 
                    {
                        configuration_mode = client_id_it->second;
                        RM_logInfo("Found Mode for client: " << std::hex << client_id << std::dec << " " << configuration_mode)

                         udp::endpoint target_address = get_client_address(data_message.source_id);

                        struct timespec time_now = {0,0};
                        clock_gettime(CLOCK_REALTIME, &time_now);
                        RM_logInfo("Timestamp now                 : " << time_now.tv_sec << "s " << time_now.tv_nsec << " ns")
                        RM_logInfo("Timestamp stop for rm client  : " << std::hex << data_message.source_id << std::dec << " is: " << target_time_stop_offset.tv_sec << "s " << target_time_stop_offset.tv_nsec << " ns")
                        RM_logInfo("Timestamp reconf for rm client: " << std::hex << data_message.source_id << std::dec << " is: " << target_time_reconfig_offset.tv_sec << "s " << target_time_reconfig_offset.tv_nsec << " ns")
                        RM_logInfo("Timestamp start for rm client : " << std::hex << data_message.source_id << std::dec << " is: " << target_time_start_offset.tv_sec << "s " << target_time_start_offset.tv_nsec << " ns")

                        send_synchronous_reconfigure(target_time_stop_offset, target_time_reconfig_offset, target_time_start_offset, target_address, service_id, configuration_mode);
                        
                    }         
                }     
            }
            std::chrono::milliseconds random_ms = random_duration(experiment_parameter_struct.inter_mc_gap_min , experiment_parameter_struct.inter_mc_gap_max);
            RM_logInfo("Wait for next mode change : " << random_ms.count()/1000 << " s")
            sleep(random_ms.count()/1000);
        }        
    }

    std::chrono::system_clock::time_point start_trigger = std::chrono::system_clock::now() + experiment_parameter_struct.experiment_end_offset;
    RM_logInfo("NetRM start timer for experiment end. ")
    start_timer.registerTimer(rm_id, start_trigger, false, &NetLayerRM<T>::stopping_experiment_synchronous, this);
}


/*
*
*/
template<class T>
void NetLayerRM<T>::experiment_mode_change_synchronous_asynchonous_objects()
{
    RM_logInfo(" ")
    RM_logInfo("NetRM async_sync experiment start triggered.") 
    struct timespec time_now = {0,0};
    clock_gettime(CLOCK_REALTIME, &time_now);
    RM_logInfo("Timestamp now : " << time_now.tv_sec << "s " << time_now.tv_nsec << " ns") 
    
    rscmng::wired::RMMessage data_message;
       
    uint32_t network_mode = 0;
    uint32_t configuration_mode = 1;
    
    for(uint8_t iterator = 0; iterator < experiment_parameter_struct.experiment_iterations; iterator++)
    {
        //mode iteration
        for (const auto& mode_order_iterator : experiment_parameter_struct.reconfiguration_order) 
        {
            network_mode = mode_order_iterator;
            // calculate next reconfiguration window
            struct timespec current_time = {0, 0};
            struct timespec target_mc_begin = {0, 0};
            clock_gettime(CLOCK_REALTIME, &current_time);
                       
            target_mc_begin = current_time;
            target_mc_begin.tv_sec = target_mc_begin.tv_sec + (experiment_parameter_struct.mc_distribution_phase_duration.count()/1000);

            RM_logInfo("NetRM new timestamp mode change begin " << target_mc_begin.tv_sec << " s " << target_mc_begin.tv_nsec << " ns" ) 

            struct timespec target_time_stop_offset     = prepare_timestamp(target_mc_begin, experiment_parameter_struct.mc_client_stop_offset);
            struct timespec target_time_reconfig_offset = prepare_timestamp(target_mc_begin, experiment_parameter_struct.mc_client_reconfig_offset + experiment_parameter_struct.mc_client_stop_offset);
            struct timespec target_time_start_offset    = prepare_timestamp(target_mc_begin, experiment_parameter_struct.mc_client_start_offset + experiment_parameter_struct.mc_client_reconfig_offset + experiment_parameter_struct.mc_client_stop_offset);

                 
            RM_logInfo("NetRM experiment network_mode: " << network_mode)   
        
            auto list_iterator = service_list.begin();
            while (list_iterator != service_list.end()) 
            {
                serviceID_t service_id = list_iterator->first;        
                data_message = list_iterator->second;
                ++list_iterator;

                uint32_t client_id = data_message.source_id;        
                auto network_mode_it = experiment_parameter_struct.reconfiguration_map.find(network_mode);
                if (network_mode_it != experiment_parameter_struct.reconfiguration_map.end()) 
                {
                    auto client_id_it = network_mode_it->second.find(client_id);
                    if (client_id_it != network_mode_it->second.end()) 
                    {
                        configuration_mode = client_id_it->second;
                        RM_logInfo("Found Mode for client: " << std::hex << client_id << std::dec << " " << configuration_mode)

                         udp::endpoint target_address = get_client_address(data_message.source_id);

                        struct timespec time_now = {0,0};
                        clock_gettime(CLOCK_REALTIME, &time_now);
                        RM_logInfo("Timestamp now                 : " << time_now.tv_sec << "s " << time_now.tv_nsec << " ns")
                        RM_logInfo("Timestamp stop for rm client  : " << std::hex << data_message.source_id << std::dec << " is: " << target_time_stop_offset.tv_sec << "s " << target_time_stop_offset.tv_nsec << " ns")
                        RM_logInfo("Timestamp reconf for rm client: " << std::hex << data_message.source_id << std::dec << " is: " << target_time_reconfig_offset.tv_sec << "s " << target_time_reconfig_offset.tv_nsec << " ns")
                        RM_logInfo("Timestamp start for rm client : " << std::hex << data_message.source_id << std::dec << " is: " << target_time_start_offset.tv_sec << "s " << target_time_start_offset.tv_nsec << " ns")

                        send_synchronous_reconfigure(target_time_stop_offset, target_time_reconfig_offset, target_time_start_offset, target_address, service_id, configuration_mode);
                        
                    }         
                }     
            }
            std::chrono::milliseconds random_ms = random_duration(experiment_parameter_struct.inter_mc_gap_min , experiment_parameter_struct.inter_mc_gap_max);
            RM_logInfo("Wait for next mode change : " << random_ms.count()/1000 << " s")
            sleep(random_ms.count()/1000);
        }        
    }

    std::chrono::system_clock::time_point start_trigger = std::chrono::system_clock::now() + experiment_parameter_struct.experiment_end_offset;
    RM_logInfo("NetRM start timer for experiment end. ")
    start_timer.registerTimer(rm_id, start_trigger, false, &NetLayerRM<T>::stopping_experiment_synchronous, this);
}


/*
*
*/
template<class T>
uint32_t NetLayerRM<T>::calculate_service_slot(uint32_t network_mode, serviceID_t service_id)
{
    // check service slot
    std::chrono::milliseconds service_offset = std::chrono::milliseconds(rm_client_service_map[service_id][0].offset);
    service_offset =  std::chrono::milliseconds(rm_client_service_map[service_id][network_mode].offset);       
   
    uint32_t slot_number = 1;
    std::chrono::milliseconds test_slot_duration = slot_duration;
    while(service_offset >= test_slot_duration)
    {
        test_slot_duration = test_slot_duration + slot_duration;
        slot_number = slot_number +1;
    }
    RM_logInfo("NetRM service id: " << service_id << " with service start offset: " << service_offset.count() << " ms is in slot: " << slot_number+1000 << " of hyperperiod " << std::dec << hyperperiod_duration.count() << " ms")   

    return slot_number;
}

/*
*
*/
template<class T>
void NetLayerRM<T>::stopping_experiment_synchronous()
{
    //Stopping experiment
    struct timespec starting_timestamp = get_round_timestamp();
    std::chrono::milliseconds mc_begin_offset = experiment_parameter_struct.mc_distribution_phase_duration;
    struct timespec target_time_mc_begin = prepare_timestamp(starting_timestamp, mc_begin_offset);
    uint32_t network_mode = 10;
    rscmng::wired::RMMessage data_message;
    RM_logInfo("NetRM experiment network_mode STOP.")

    auto list_iterator = service_list.begin();
    list_iterator = service_list.begin();
    while (list_iterator != service_list.end()) 
    {
        serviceID_t service_id = list_iterator->first;        
        data_message = list_iterator->second;
        ++list_iterator;

        struct timespec target_time_stop_offset     = prepare_timestamp(target_time_mc_begin, std::chrono::milliseconds(10));
        struct timespec target_time_reconfig_offset = prepare_timestamp(target_time_mc_begin, std::chrono::milliseconds(20));
        struct timespec target_time_start_offset    = prepare_timestamp(target_time_mc_begin, std::chrono::milliseconds(20));
    
        udp::endpoint target_address = get_client_address(data_message.source_id);

        RM_logInfo("Timestamp stop for rm client  : " << std::hex << data_message.source_id << std::dec << " is: " << target_time_stop_offset.tv_sec << "s " << target_time_stop_offset.tv_nsec << " ns")
        RM_logInfo("Timestamp reconf for rm client: " << std::hex << data_message.source_id << std::dec << " is: " << target_time_reconfig_offset.tv_sec << "s " << target_time_reconfig_offset.tv_nsec << " ns")
        RM_logInfo("Timestamp start for rm client : " << std::hex << data_message.source_id << std::dec << " is: " << target_time_start_offset.tv_sec << "s " << target_time_start_offset.tv_nsec << " ns")

        send_synchronous_stop(target_time_stop_offset, target_time_reconfig_offset, target_time_start_offset, target_address, service_id, network_mode);
    }
}


/*
*
*/
template<class T>
void NetLayerRM<T>::experiment_mode_change_asynchronous()
{
    RM_logInfo(" ")
    RM_logInfo("NetRM experiment start triggered.")
    
    rscmng::wired::RMMessage data_message;
    
    uint32_t network_mode = 0;
    uint32_t configuration_mode = 1;
    
    for(uint8_t iterator = 0; iterator < experiment_parameter_struct.experiment_iterations; iterator++)
    {
        for (const auto& mode_order_iterator : experiment_parameter_struct.reconfiguration_order) 
        {
            network_mode = mode_order_iterator;     
            RM_logInfo("NetRM experiment network_mode: " << network_mode)   
        
            auto list_iterator = service_list.begin();
            while (list_iterator != service_list.end()) 
            {
                serviceID_t service_id = list_iterator->first;        
                data_message = list_iterator->second;
                ++list_iterator;

                uint32_t client_id = data_message.source_id;        
                auto network_mode_it = experiment_parameter_struct.reconfiguration_map.find(network_mode);
                if (network_mode_it != experiment_parameter_struct.reconfiguration_map.end()) 
                {
                    auto client_id_it = network_mode_it->second.find(client_id);
                    if (client_id_it != network_mode_it->second.end()) 
                    {
                        configuration_mode = client_id_it->second;
                        RM_logInfo("Found Mode for client: " << std::hex << client_id << std::dec << " " << configuration_mode)
                                            
                        udp::endpoint target_address = get_client_address(data_message.source_id);
                    
                        send_asynchronous_reconfigure(target_address, service_id, configuration_mode);
                    }             
                } 
            }
            std::chrono::milliseconds random_ms = random_duration(experiment_parameter_struct.inter_mc_gap_min , experiment_parameter_struct.inter_mc_gap_max);
            RM_logInfo("Wait for next mode change : " << random_ms.count()/1000 << " s")
            sleep(random_ms.count()/1000);
        }        
    }

    //Stopping experiment
    std::chrono::system_clock::time_point start_trigger = std::chrono::system_clock::now() + experiment_parameter_struct.experiment_end_offset;
    RM_logInfo("NetRM start timer for experiment end. ")
    start_timer.registerTimer(rm_id, start_trigger, false, &NetLayerRM<T>::stopping_experiment_asynchronous, this);
}


/*
*
*/
template<class T>
void NetLayerRM<T>::stopping_experiment_asynchronous()
{
    uint32_t network_mode = 10;
    rscmng::wired::RMMessage data_message;

    RM_logInfo("NetRM experiment network_mode STOP.")

    auto list_iterator = service_list.begin();
    list_iterator = service_list.begin();
    while (list_iterator != service_list.end()) 
    {
        serviceID_t service_id = list_iterator->first;        
        data_message = list_iterator->second;
        ++list_iterator;
           
        udp::endpoint target_address = get_client_address(data_message.source_id);
        send_asynchronous_stop(target_address, service_id, network_mode);
    }
}


/*
*
*/
template<class T>
void NetLayerRM<T>::send_synchronous_reconfigure(struct timespec stop_time, struct timespec recofig_time, struct timespec start_time, udp::endpoint target_address, serviceID_t service_id, uint32_t network_mode)
{
    MessageNet_t rm_message;
    rscmng::rm_wired_basic_protocol::RMPayload rm_protocol_payload(RMCommand::SYNC_TIMESTAMP_RECONFGURE, stop_time, recofig_time, start_time);

    rm_protocol_payload.serialize(&rm_message);   

    control_channel.send_sync_reconfig(target_address, demonstrator::RM_ID, service_id, network_mode, &rm_message);
}


/*
*
*/
template<class T>
void NetLayerRM<T>::send_asynchronous_reconfigure(udp::endpoint target_address, serviceID_t service_id, uint32_t network_mode)
{
    MessageNet_t rm_message;

    control_channel.send_reconfig(target_address, demonstrator::RM_ID, service_id, network_mode, &rm_message);
}

/*
*
*/
template<class T>
void NetLayerRM<T>::send_synchronous_start(struct timespec target_time, udp::endpoint target_address, serviceID_t service_id)
{
    MessageNet_t rm_message;
    rscmng::rm_wired_basic_protocol::RMPayload rm_protocol_payload(RMCommand::SYNC_TIMESTAMP_START,{0,0}, {0,0}, target_time);

    rm_protocol_payload.serialize(&rm_message);   

    control_channel.send_start_sync(target_address, demonstrator::RM_ID, service_id, &rm_message);
}


/*
*
*/
template<class T>
void NetLayerRM<T>::send_synchronous_stop(struct timespec stop_time, struct timespec recofig_time, struct timespec start_time, udp::endpoint target_address, serviceID_t service_id, uint32_t network_mode)
{
    MessageNet_t rm_message;
    rscmng::rm_wired_basic_protocol::RMPayload rm_protocol_payload(RMCommand::SYNC_TIMESTAMP_RECONFGURE, stop_time, recofig_time, start_time);

    rm_protocol_payload.serialize(&rm_message);   

    control_channel.send_sync_stop(target_address, demonstrator::RM_ID, service_id, network_mode, &rm_message);
}


/*
*
*/
template<class T>
void NetLayerRM<T>::send_asynchronous_stop(udp::endpoint target_address, serviceID_t service_id, uint32_t network_mode)
{
    MessageNet_t rm_message;

    control_channel.send_stop(target_address, demonstrator::RM_ID, service_id, network_mode, &rm_message);
}


/*
*
*/
template<class T>
void NetLayerRM<T>::join()
{
    recving_thread.join();
    handler_thread.join();
    start_timer.stop();
}



/*
*
*/
template<class T>
std::chrono::milliseconds NetLayerRM<T>::random_duration(std::chrono::milliseconds min_value, std::chrono::milliseconds max_value) 
{
    // Seed with a real random value, if available
    std::random_device random;
    std::mt19937 generated_number(random());

    // Define the distribution within the millisecond range
    std::uniform_int_distribution<int64_t> distribution(min_value.count(), max_value.count());

    // Generate the random duration and return as milliseconds
    return std::chrono::milliseconds(distribution(generated_number));
}