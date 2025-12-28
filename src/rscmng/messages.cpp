// Copyright (C) 2025 IDA 
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


#include <rscmng/messages.hpp>


using namespace rscmng;


// ----------------------------- RMMessage -------------------------------------------------------------------------
// Constructor for empty RMMessage without app attributes
rscmng::wired::RMMessage::RMMessage()
:   message_type(NOOP),
    priority(0xFF),
    source_id(),
    destination_id(),    
    service_id(0xFFFFFFFFFFFFFFFF),
    mode(0xFF),
    time_stamp(),
    protocol_id(rscmng::NONE),
    payload(),
    payload_size(0)
{}

// Constructor for basic RMMessage
rscmng::wired::RMMessage::RMMessage(   
    MessageTypes message_type, 
    uint8_t priority, 
    uint32_t source_id,
    uint32_t destination_id, 
    serviceID_t req_app
):   
    message_type(message_type),
    priority(priority),
    source_id(source_id),
    destination_id(destination_id),    
    service_id(req_app),
    mode(0xFF),
    time_stamp(),
    protocol_id(rscmng::NONE),
    payload(),
    payload_size(0)
{
    time_stamp = std::chrono::system_clock::now();
}

// Constructor for basic RMMessage
rscmng::wired::RMMessage::RMMessage(   
    MessageTypes message_type, 
    uint8_t priority, 
    uint32_t source_id,
    uint32_t destination_id,    
    serviceID_t req_app,
    uint8_t mode
):   
    message_type(message_type),
    priority(priority),
    source_id(source_id),
    destination_id(destination_id),
    service_id(req_app),
    mode(mode),
    time_stamp(),
    protocol_id(rscmng::NONE),
    payload(),
    payload_size(0)
{
    time_stamp = std::chrono::system_clock::now();
}

// Constructor for specific RMMessage 
rscmng::wired::RMMessage::RMMessage(   
    MessageTypes message_type, 
    uint8_t priority,
    uint32_t source_id, 
    uint32_t destination_id,     
    serviceID_t req_app,
    uint8_t mode,
    ProtocolIDs protocol_id, 
    MessageNet_t* rm_payload_message
):   
    message_type(message_type),
    priority(priority),
    source_id(source_id),
    destination_id(destination_id),    
    service_id(req_app),
    mode(mode),
    time_stamp(),
    protocol_id(protocol_id),
    payload(),
    payload_size(rm_payload_message->length)
{ 
    memcpy(payload, rm_payload_message->buffer, payload_size);

    time_stamp = std::chrono::system_clock::now();
}

// Deconstructor
rscmng::wired::RMMessage::~RMMessage()
{}

// Convert rm message struct to char array
void rscmng::wired::RMMessage::rm_to_network(MessageNet_t* message)
{
    message->add(&message_type, sizeof(message_type));
    message->add(&priority, sizeof(priority));
    message->add(&source_id, sizeof(source_id));
    message->add(&destination_id, sizeof(destination_id));
    message->add(&service_id, sizeof(service_id));    
    message->add(&mode, sizeof(mode));
    message->add(&time_stamp, sizeof(time_stamp));
    message->add(&protocol_id, sizeof(protocol_id));
    message->add(payload, payload_size);
}

// Convert char array back to rm message struct
void rscmng::wired::RMMessage::network_to_rm(MessageNet_t* message)
{
    message->read(&message_type, sizeof(message_type));
    message->read(&priority, sizeof(priority));
    message->read(&source_id, sizeof(source_id));
    message->read(&destination_id, sizeof(destination_id));
    message->read(&service_id, sizeof(service_id));        
    message->read(&mode, sizeof(mode));
    message->read(&time_stamp, sizeof(time_stamp));
    message->read(&protocol_id, sizeof(protocol_id));
    payload_size = message->length-message->pos;
    message->read(payload, payload_size);
}

void rscmng::wired::RMMessage::change_id(uint32_t new_id)
{
    destination_id = new_id;
}

void rscmng::wired::RMMessage::change_service(serviceID_t new_id)
{
    service_id = new_id;
}

void rscmng::wired::RMMessage::set_timestamp(std::chrono::system_clock::time_point tp)
{
    time_stamp = tp;
}

size_t rscmng::wired::RMMessage::size()
{
    return length;
}

void rscmng::wired::RMMessage::print()
{
    RM_logInfo("Type: " << message_type << " "
                            << "priority: " << unsigned(priority) << " "
                            << "message ID: " << std::hex << source_id << std::dec <<" "
                            << "ID: " << std::hex << destination_id << std::dec <<" "
                            << "Mode: " << mode << " " 
                            << "Service ID: " << std::hex << service_id << std::dec << " "
                            << "Protocol ID: " << protocol_id << " "
                            << "Payload size:" << payload_size << " ")
}

void rscmng::wired::RMMessage::clear()
{
    message_type = NOOP;
    priority = 0xFF;
    source_id = 0xFFFF;
    destination_id = 0xFFFF;
    mode = 0xFF;
    service_id = 0xFFFFFFFFFFFFFFFF;    
    time_stamp = std::chrono::time_point<std::chrono::system_clock>();
    protocol_id = NONE;
    memset(payload, 0, RMConfig::max_payload);
    payload_size = 0;
}

// -------------------------------------------------------------------------------------------------------------------

// ----------------------------- DataMessage -------------------------------------------------------------------------

DataMessage::DataMessage()
:
    priority(0xFF),
    source_id(),
    service_id(0xFFFFFFFFFFFFFFFF),
    fragment_number(0xFFFFFFFF),
    total_fragments(0xFFFFFFFF),
    payload("\0"),
    payload_length(0),
    timestamp({0,0})
{}

DataMessage::DataMessage(   
    uint8_t priority, 
    uint32_t client_id, 
    serviceID_t service_id, 
    uint32_t object_number, 
    uint32_t fragment_number, 
    uint32_t total_number, 
    char* payload, 
    size_t payload_size
):
    priority(priority),
    source_id(client_id),
    service_id(service_id),  
    object_number(object_number),
    fragment_number(fragment_number),
    total_fragments(total_number),
    payload(),
    payload_length(payload_size),
    timestamp({0,0})
{
    length = sizeof(priority) + sizeof(source_id) + sizeof(service_id) + sizeof(object_number) + sizeof(fragment_number) + sizeof(total_fragments) + payload_length + sizeof(timestamp);
    header_length = sizeof(priority) + sizeof(source_id) + sizeof(service_id) + sizeof(object_number) + sizeof(fragment_number) + sizeof(total_fragments) + sizeof(timestamp);
    memcpy(payload, payload, payload_size);
    time_stamp = std::chrono::system_clock::now();
}


DataMessage::DataMessage(   
    uint8_t priority, 
    uint32_t client_id, 
    serviceID_t service_id, 
    uint32_t object_number, 
    uint32_t fragment_number, 
    uint32_t total_number, 
    char* payload, 
    size_t payload_size,
    struct timespec timestamp
):
    priority(priority),
    source_id(client_id),
    service_id(service_id),  
    object_number(object_number),
    fragment_number(fragment_number),
    total_fragments(total_number),
    payload(),
    payload_length(payload_size),
    timestamp(timestamp)
{
    length = sizeof(priority) + sizeof(source_id) + sizeof(service_id) + sizeof(object_number) + sizeof(fragment_number) + sizeof(total_fragments) + payload_length + sizeof(timestamp);
    header_length = sizeof(priority) + sizeof(source_id) + sizeof(service_id) + sizeof(object_number) + sizeof(fragment_number) + sizeof(total_fragments) + sizeof(timestamp);
    memcpy(payload, payload, payload_size);
    time_stamp = std::chrono::system_clock::now();
}


void DataMessage::dataToNet(char* message)
{  
    message[0] = priority & 0xFF;
    memcpy(message+sizeof(priority), &source_id, sizeof(source_id));
    memcpy((message+sizeof(priority)+sizeof(source_id)), &service_id, sizeof(service_id));
    memcpy((message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)), &object_number, sizeof(object_number));
    memcpy((message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)+sizeof(object_number)), &fragment_number, sizeof(fragment_number));
    memcpy((message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)+sizeof(object_number)+sizeof(fragment_number)), &total_fragments, sizeof(total_fragments));
    memcpy((message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)+sizeof(object_number)+sizeof(fragment_number)+sizeof(total_fragments)), &timestamp, sizeof(timestamp));
    memcpy((message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)+sizeof(object_number)+sizeof(fragment_number)+sizeof(total_fragments)+sizeof(timestamp)), &time_stamp, sizeof(time_stamp));
    
    memcpy((message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)+sizeof(object_number)+sizeof(fragment_number)+sizeof(total_fragments)+sizeof(timestamp)+sizeof(time_stamp)), payload, payload_length);

}

void DataMessage::netToData(char* message, size_t size)
{
    length = size;
    payload_length = length - header_length;
    
    priority = (uint8_t)(message[0]);
    memcpy(&source_id, (message+sizeof(priority)), sizeof(source_id));
    memcpy(&service_id, (message+sizeof(priority)+sizeof(source_id)), sizeof(service_id));
    memcpy(&object_number, (message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)), sizeof(object_number));
    memcpy(&fragment_number, (message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)+sizeof(object_number)), sizeof(fragment_number));
    memcpy(&total_fragments, (message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)+sizeof(object_number)+sizeof(fragment_number)), sizeof(total_fragments));
    memcpy(&timestamp,  (message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)+sizeof(object_number)+sizeof(fragment_number)+sizeof(total_fragments)), sizeof(timestamp));
    memcpy(&time_stamp, (message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)+sizeof(object_number)+sizeof(fragment_number)+sizeof(total_fragments)+sizeof(timestamp)), sizeof(time_stamp));

    memcpy(payload, (message+sizeof(priority)+sizeof(source_id)+sizeof(service_id)+sizeof(object_number)+sizeof(fragment_number)+sizeof(total_fragments)+sizeof(timestamp)+sizeof(time_stamp)), payload_length);
    
}


/*
*
*/
void DataMessage::set_payload(uint32_t object_number, uint32_t fragment_number, uint32_t total_number, char* payload, size_t size)
{
    payload_length = size;
    length  = sizeof(priority) + sizeof(source_id) + sizeof(service_id) + sizeof(object_number) + sizeof(fragment_number) + sizeof(total_fragments) + payload_length;
    object_number = object_number;
    fragment_number = fragment_number;
    total_fragments = total_number;
    memcpy(payload, payload, size);
}


/*
*
*/
void DataMessage::set_payload(uint32_t object_number, uint32_t fragment_number, uint32_t total_number, char* payload, size_t size, struct timespec timestamp)
{
    payload_length = size;
    length  = sizeof(priority) + sizeof(source_id) + sizeof(service_id) + sizeof(object_number) + sizeof(fragment_number) + sizeof(total_fragments) + payload_length;
    object_number = object_number;
    fragment_number = fragment_number;
    total_fragments = total_number;
    timestamp = timestamp;
    memcpy(payload, payload, size);    
}

/*
*
*/
void DataMessage::set_timestamp( struct timespec new_timestamp)
{
    timestamp = new_timestamp;
}

/*
*
*/
void DataMessage::set_packet_number(uint32_t packet_number)
{
    fragment_number = packet_number; 
}

/*
*
*/
void DataMessage::set_payload_size( size_t payload_size)
{
    payload_length = payload_size;
}


void DataMessage::change_source_id(uint32_t new_id)
{
    source_id = new_id;
} 

void DataMessage::change_service(serviceID_t new_id)
{
    service_id = new_id;
}

size_t DataMessage::size()
{
    return length;
}

size_t DataMessage::header_size()
{
    return header_length;
}

void DataMessage::print()
{
    RM_logInfo("priority: " << priority)
    RM_logInfo("Source ID: " << source_id)
    RM_logInfo("Target APP: " << service_id)
    RM_logInfo("Fragment Number: " << fragment_number)
    RM_logInfo("Total Fragments: " << total_fragments)
    RM_logInfo("Timestamp: " << timestamp.tv_sec << " " << timestamp.tv_nsec)
    std::time_t time_t_time_stamp = std::chrono::system_clock::to_time_t(time_stamp);
    RM_logInfo("Time_stamp: " << std::ctime(&time_t_time_stamp))
    RM_logInfo("Payload: ")
    for(auto&& c : payload)
    {
        RM_logInfo(std::hex << (uint32_t)c)
    }
}

void DataMessage::print(size_t len)
{
    RM_logInfo("priority: " << priority)
    RM_logInfo("Source ID: " << source_id)
    RM_logInfo("Target APP: " << service_id)
    RM_logInfo("Fragment Number: " << fragment_number)
    RM_logInfo("Total Fragments: " << total_fragments)
    RM_logInfo("Payload: ")
    for(size_t i = 0 ; i<len; i++)
    {
        RM_logInfo(std::hex << (uint32_t)payload[i])
    }
    RM_logInfo("Payload length: " << payload_length)
    RM_logInfo("message length: " << length)
}

void DataMessage::print_header()
{
    std::time_t time_t_time_stamp = std::chrono::system_clock::to_time_t(time_stamp);
    RM_logInfo("priority: " << priority << " "
            << "Source ID: " << source_id << " "
            << "Target ID: " << service_id << " "
            << "Fragment Number: " << fragment_number << " "
            << "Total Fragments: " << total_fragments << " "
            << "Timestamp: " << timestamp.tv_sec << " " << timestamp.tv_nsec << " "
            << "Time_stamp: " << std::ctime(&time_t_time_stamp)
            )
}

void DataMessage::clear()
{
    priority = 0xFF;
    source_id = 0xFFFF;    
    service_id = 0xFFFFFFFFFFFFFFFF;
    object_number = 0xFFFFFFFF;
    fragment_number = 0xFFFFFFFF;
    total_fragments = 0xFFFFFFFF;
    payload[0] = '\0';
    payload_length = 0;
    length = header_length;
}

// -------------------------------------------------------------------------------------------------------------------