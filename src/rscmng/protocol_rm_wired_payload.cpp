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


#include <rscmng/protocols/rm_wired_payload.hpp>


/*************************************** RM Protocol **********************************************/
/*
*
*/
rscmng::rm_wired_basic_protocol::RMPayload::RMPayload(): 
    rscmng::basic_protocol::BasicPayload(),
    data_connection_priority(0),
    measurement_id(0),
    rm_client_action(IDLE),
    timestamp_stop({0,0}),
    timestamp_reconfig({0,0}),
    timestamp_start({0,0}),
    resource_request_struct({0,0,0,0,std::chrono::milliseconds(0),{0}})
{}


/*
*
*/
rscmng::rm_wired_basic_protocol::RMPayload::RMPayload(
    uint32_t object_size, 
    std::chrono::nanoseconds object_deadline,
    uint32_t stream_priority
):
    rscmng::basic_protocol::BasicPayload(object_size, object_deadline),
    data_connection_priority(stream_priority)
{}


/*
*
*/
rscmng::rm_wired_basic_protocol::RMPayload::RMPayload(
    uint32_t object_size, 
    std::chrono::nanoseconds object_deadline,
    uint32_t stream_priority,
    uint32_t measurement,
    RMCommand rm_client_action
):
    rscmng::basic_protocol::BasicPayload(object_size, object_deadline),
    data_connection_priority(stream_priority),
    measurement_id(measurement)
{}


/*
*
*/
rscmng::rm_wired_basic_protocol::RMPayload::RMPayload(
    uint32_t object_size, 
    std::chrono::nanoseconds object_deadline,
    uint32_t stream_priority,
    uint32_t measurement,
    RMCommand rm_client_action,
    struct timespec timestamp_stop,
    struct timespec timestamp_reconfig,
    struct timespec timestamp_start,
    struct network_resource_request resource_request_struct
):
    rscmng::basic_protocol::BasicPayload(object_size, object_deadline),
    data_connection_priority(stream_priority),
    measurement_id(measurement),
    rm_client_action(rm_client_action),
    timestamp_stop(timestamp_stop),
    timestamp_reconfig(timestamp_reconfig),
    timestamp_start(timestamp_start),
    resource_request_struct(resource_request_struct)
{}


/*
*
*/
rscmng::rm_wired_basic_protocol::RMPayload::RMPayload(
    RMCommand rm_client_action,
    struct timespec timestamp_stop,
    struct timespec timestamp_reconfig,
    struct timespec timestamp_start
):
    rscmng::basic_protocol::BasicPayload(),
    data_connection_priority(0),
    measurement_id(0),
    rm_client_action(rm_client_action),
    timestamp_stop(timestamp_stop),
    timestamp_reconfig(timestamp_reconfig),
    timestamp_start(timestamp_start)
{}
            

rscmng::rm_wired_basic_protocol::RMPayload::RMPayload(
    RMCommand rm_client_action,
    struct network_resource_request resource_request_struct
): 
    rscmng::basic_protocol::BasicPayload(),
    data_connection_priority(0),
    measurement_id(0),
    rm_client_action(rm_client_action),
    timestamp_stop({0,0}),
    timestamp_reconfig({0,0}),
    timestamp_start({0,0}),
    resource_request_struct(resource_request_struct)
{}

/*
*
*/
void rscmng::rm_wired_basic_protocol::RMPayload::serialize(MessageNet_t* rm_payload_message)
{    
    rscmng::basic_protocol::BasicPayload::serialize(rm_payload_message);
    rm_payload_message->add(&data_connection_priority, sizeof(data_connection_priority));
    rm_payload_message->add(&measurement_id, sizeof(measurement_id));
    rm_payload_message->add(&rm_client_action, sizeof(rm_client_action));
    rm_payload_message->add(&timestamp_stop, sizeof(timestamp_stop));
    rm_payload_message->add(&timestamp_reconfig, sizeof(timestamp_reconfig));
    rm_payload_message->add(&timestamp_start, sizeof(timestamp_start));
    rm_payload_message->add(&resource_request_struct, sizeof(resource_request_struct));
}


/*
*
*/
void rscmng::rm_wired_basic_protocol::RMPayload::deserialize(MessageNet_t* rm_payload_message)
{
    rscmng::basic_protocol::BasicPayload::deserialize(rm_payload_message);
    rm_payload_message->read(&data_connection_priority, sizeof(data_connection_priority));
    rm_payload_message->read(&measurement_id, sizeof(measurement_id));
    rm_payload_message->read(&rm_client_action, sizeof(rm_client_action));
    rm_payload_message->read(&timestamp_stop, sizeof(timestamp_stop));
    rm_payload_message->read(&timestamp_reconfig, sizeof(timestamp_reconfig));
    rm_payload_message->read(&timestamp_start, sizeof(timestamp_start));
    rm_payload_message->read(&resource_request_struct, sizeof(resource_request_struct));
}


/*
*
*/
void rscmng::rm_wired_basic_protocol::RMPayload::print()
{
    rscmng::basic_protocol::BasicPayload::print();
    RM_logInfo("Priority:" << data_connection_priority)
    RM_logInfo("Measurement:" << measurement_id)
    RM_logInfo("Timestamp stop: " << timestamp_stop.tv_sec << " s " << timestamp_stop.tv_nsec << " ns ")
    RM_logInfo("Timestamp reconfig: " << timestamp_reconfig.tv_sec << " s " << timestamp_reconfig.tv_nsec << " ns ")
    RM_logInfo("Timestamp start: " << timestamp_start.tv_sec << " s " << timestamp_start.tv_nsec << " ns ")
    RM_logInfo("Resource Request: " << "client id: " << resource_request_struct.client_id << " " 
                                    << "priority: " << resource_request_struct.priority << " " 
                                    << "bandwidth: " << resource_request_struct.bandwidth << " " )
    RM_logInfo("Deadline: " << resource_request_struct.deadline.count() << " ms");
    RM_logInfo("Data Path: ")
    std::ostringstream oss;
    for (const int& i : resource_request_struct.data_path) 
    {
        oss << i << ", ";
    }
    RM_logInfo(oss.str())
}


// Setter
/*
*
*/
void rscmng::rm_wired_basic_protocol::RMPayload::set_data_connection_priority(uint32_t priority)
{
    data_connection_priority = priority;
}


/*
*
*/
uint32_t rscmng::rm_wired_basic_protocol::RMPayload::get_data_connection_priority()
{
    return data_connection_priority;
}


/*
*
*/
void rscmng::rm_wired_basic_protocol::RMPayload::set_measurement_id(uint32_t measurement_id)
{
    measurement_id = measurement_id;
}


/*
*
*/
uint32_t rscmng::rm_wired_basic_protocol::RMPayload::get_measurement_id()
{
    return measurement_id;
}


/*
*
*/
void rscmng::rm_wired_basic_protocol::RMPayload::set_timestamp_stop(struct timespec timestamp)
{
    timestamp_stop = timestamp;
}


/*
*
*/
struct timespec rscmng::rm_wired_basic_protocol::RMPayload::get_timestamp_stop()
{
    return timestamp_stop;
}


/*
*
*/
void rscmng::rm_wired_basic_protocol::RMPayload::set_timestamp_reconfig(struct timespec timestamp)
{
    timestamp_reconfig = timestamp;
}


/*
*
*/
struct timespec rscmng::rm_wired_basic_protocol::RMPayload::get_timestamp_reconfig()
{
    return timestamp_reconfig;
}


/*
*
*/
void rscmng::rm_wired_basic_protocol::RMPayload::set_timestamp_start(struct timespec timestamp)
{
    timestamp_start = timestamp;
}


/*
*
*/
struct timespec rscmng::rm_wired_basic_protocol::RMPayload::get_timestamp_start()
{
    return timestamp_start;
}

/*
*
*/
void rscmng::rm_wired_basic_protocol::RMPayload::set_resource_request_payload(struct network_resource_request resource_request_struct)
{
    resource_request_struct = resource_request_struct;
}


/*
*
*/
struct rscmng::rm_wired_basic_protocol::RMPayload::network_resource_request rscmng::rm_wired_basic_protocol::RMPayload::get_resource_request_payload()
{
    return resource_request_struct;
}