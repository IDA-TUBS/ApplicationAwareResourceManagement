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


#include <rscmng/rm_communication.hpp>


using namespace rscmng;
using namespace wired;


/*
*
*/
RMCommunication::RMCommunication( 
    std::string local_ip,             
    uint32_t local_port
):   
    control_channel_local_endpoint(udp::endpoint(boost::asio::ip::address::from_string(local_ip), local_port)),
    control_channel_socket(rm_context),
    rm_message_len(RMMessage().size()),
    send_lock()
{    
    control_channel_socket.open(control_channel_local_endpoint.protocol());
    control_channel_socket.set_option(boost::asio::socket_base::reuse_address(true));
    control_channel_socket.set_option(boost::asio::socket_base::broadcast(true));
    control_channel_socket.bind(control_channel_local_endpoint);
}


/*
*
*/
RMCommunication::RMCommunication(        
    uint32_t broadcast_port
):   
    control_channel_local_endpoint(udp::endpoint(boost::asio::ip::address_v4::any(), broadcast_port)),
    control_channel_socket(rm_context),
    rm_message_len(RMMessage().size()),
    send_lock()
{    
    control_channel_socket.open(control_channel_local_endpoint.protocol());
    control_channel_socket.set_option(boost::asio::socket_base::reuse_address(true));
    control_channel_socket.set_option(boost::asio::socket_base::broadcast(true));
    control_channel_socket.bind(control_channel_local_endpoint);
}


/*
*
*/
void RMCommunication::sendout_control_message(  
    RMMessage message, 
    udp::endpoint target_address
)
{
    std::lock_guard<std::mutex> lock(send_lock);
    
    MessageNet_t rm_buffer;
    message.rm_to_network(&rm_buffer);      

    control_channel_socket.send_to(boost::asio::buffer(rm_buffer.buffer, rm_buffer.length), target_address);

    //RM_logInfo("Message sent: " << target_address.address() << " " << target_address.port())
}


/*
*
*/
void RMCommunication::send_rm_message(   
    udp::endpoint target_address, 
    uint8_t priority, 
    uint32_t source_id, 
    uint8_t mode,
    serviceID_t service_id,
    ProtocolIDs protocol_id,
    MessageTypes message_type,
    MessageNet_t* payload
)
{
    RMMessage rm_message_out(message_type, priority, source_id, source_id, mode, service_id, protocol_id, payload);

    sendout_control_message(rm_message_out, target_address);

    RM_logInfo("RM MESSAGE sent: " << target_address.address() << " " << target_address.port())
}


/*
*
*/
void RMCommunication::send_start(   
    udp::endpoint target_address, 
    uint32_t source_id, 
    serviceID_t service_id,
    MessageNet_t* payload  
)
{
    RMMessage rm_message_out(RM_CLIENT_START, demonstrator::RM_PRIORITY, source_id, source_id, service_id, 0, RM_BASIC, payload);

    sendout_control_message(rm_message_out, target_address);

    RM_logInfo("START ASYNC sent: " << target_address.address() << " " << target_address.port())
}


/*
*
*/
void RMCommunication::send_start_sync(   
    udp::endpoint target_address, 
    uint32_t source_id, 
    serviceID_t service_id,
    MessageNet_t* payload
)
{
    int mode_id = 0;
    RMMessage rm_message_out(RM_CLIENT_SYNC_TIMESTAMP_START, demonstrator::RM_PRIORITY, source_id, source_id, service_id, mode_id, RM_BASIC, payload);

    sendout_control_message(rm_message_out, target_address);

    RM_logInfo("START SYNC sent: " << target_address.address() << " " << target_address.port())
}


/*
*
*/
void RMCommunication::send_request(   
    udp::endpoint target_address, 
    Participant participant_type, 
    uint8_t priority, 
    uint32_t source_id, 
    uint8_t mode,
    serviceID_t service_id,
    ProtocolIDs protocol_id, 
    MessageNet_t* payload
)
{

    RMMessage rm_message_out(RM_CLIENT_REQUEST, priority, source_id, source_id, service_id, mode, protocol_id, payload);

    sendout_control_message(rm_message_out, target_address);

    RM_logInfo("Request sent: " << target_address.address() << " " << target_address.port())
}
    
/*
*
*/
void RMCommunication::send_sync_request(   
    udp::endpoint target_address, 
    uint32_t source_id, 
    uint8_t mode,
    serviceID_t service_id,
    MessageNet_t* payload
)
{
    RMMessage rm_message_out(RM_CLIENT_SYNC_REQUEST, demonstrator::RM_PRIORITY, source_id, source_id, service_id, mode, RM_BASIC, payload);

    sendout_control_message(rm_message_out, target_address);

    RM_logInfo("REQUEST SYNC sent: " << target_address.address() << " " << target_address.port())
}


/*
*
*/
void RMCommunication::send_sync_receive(   
    udp::endpoint target_address, 
    uint32_t source_id, 
    uint8_t mode,
    serviceID_t service_id,
    MessageNet_t* payload
)
{
    RMMessage rm_message_out(RM_CLIENT_SYNC_RECEIVE, demonstrator::RM_PRIORITY, source_id, source_id, service_id, mode, RM_BASIC, payload);

    sendout_control_message(rm_message_out, target_address);

    RM_logInfo("REQUEST SYNC sent: " << target_address.address() << " " << target_address.port())
}


/*
*
*/
void RMCommunication::send_sync_reconfigure_done(   
    udp::endpoint target_address, 
    uint32_t source_id, 
    uint8_t mode,
    serviceID_t service_id,
    MessageNet_t* payload
)
{
    RMMessage rm_message_out(RM_CLIENT_SYNC_RECONFIGURE_DONE, demonstrator::RM_PRIORITY, source_id, source_id, service_id, mode, RM_BASIC, payload);

    sendout_control_message(rm_message_out, target_address);

    RM_logInfo("REQUEST SYNC sent: " << target_address.address() << " " << target_address.port())
}


/*
*
*/
void RMCommunication::send_reconfig(   
    udp::endpoint target_address, 
    uint32_t source_id, 
    serviceID_t service_id,
    uint32_t mode,
    MessageNet_t* payload
)
{
    RMMessage rm_message_out(RM_CLIENT_RECONFIGURE, demonstrator::RM_PRIORITY, source_id, source_id, service_id, mode, RM_BASIC, payload);

    sendout_control_message(rm_message_out, target_address);

    RM_logInfo("RECONFIG ASYNC sent: " << target_address.address() << " " << target_address.port())
}


/*
*
*/
void RMCommunication::send_sync_reconfig(   
    udp::endpoint target_address, 
    uint32_t source_id, 
    serviceID_t service_id,
    uint32_t mode,
    MessageNet_t* payload
)
{
    RMMessage rm_message_out(RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE, demonstrator::RM_PRIORITY, source_id, source_id, service_id, mode, RM_BASIC, payload);

    sendout_control_message(rm_message_out, target_address);

    RM_logInfo("RECONFIG SYNC sent: " << target_address.address() << " " << target_address.port())
}


/*
*
*/
void RMCommunication::send_sync_stop(   
    udp::endpoint target_address, 
    uint32_t source_id, 
    serviceID_t service_id,
    uint32_t mode,
    MessageNet_t* payload
)
{
    RMMessage rm_message_out(RM_CLIENT_SYNC_TIMESTAMP_STOP, demonstrator::RM_PRIORITY, source_id, source_id, service_id, mode, RM_BASIC, payload);

    sendout_control_message(rm_message_out, target_address);

    RM_logInfo("RECONFIG SYNC sent: " << target_address.address() << " " << target_address.port())
}


/*
*
*/
void RMCommunication::send_stop(   
    udp::endpoint target_address, 
    uint32_t source_id, 
    serviceID_t service_id,
    uint32_t mode,
    MessageNet_t* payload
)
{
    RMMessage rm_message_out(RM_CLIENT_STOP, demonstrator::RM_PRIORITY, source_id, source_id, service_id, mode, RM_BASIC, payload);

    sendout_control_message(rm_message_out, target_address);

    RM_logInfo("RECONFIG SYNC sent: " << target_address.address() << " " << target_address.port())
}



/*
*
*/
udp::endpoint RMCommunication::receive_control_message(RMMessage &message)
{
    MessageNet_t rm_buffer;
    size_t rm_buffer_size = 0;

    // Store endpoint for rm_message reply
    udp::endpoint target_address;

    rm_buffer_size = control_channel_socket.receive_from(boost::asio::buffer(rm_buffer.buffer, rm_buffer.max_size), target_address);

    rm_buffer.length = rm_buffer_size;
    message.network_to_rm(&rm_buffer);

    return target_address;
}


/*
*
*/
udp::endpoint RMCommunication::receive_control_message(
    RMMessage &message, 
    bool &active
)
{
    control_channel_socket.non_blocking(true);
    MessageNet_t rm_buffer;
    size_t rm_message = 0;
    boost::system::error_code error;

    // Store endpoint for rm_message reply
    udp::endpoint target_address;

    while(active && rm_message == 0)
    {
        rm_message = control_channel_socket.receive_from(boost::asio::buffer(rm_buffer.buffer, rm_buffer.max_size), target_address, 0, error);
    }
    
    if(rm_message > 0)
    {
        rm_buffer.length = rm_message;
        message.network_to_rm(&rm_buffer);
    }
    
    error.clear();
    control_channel_socket.non_blocking(false);
    return target_address;
}


/*
*
*/
void RMCommunication::close()
{
    control_channel_socket.close();
}


/*
*
*/
void RMCommunication::send_release(  
    udp::endpoint target_address, 
    Participant participant_type, 
    uint32_t source_id, 
    serviceID_t service_id
)
{  
    RMMessage rm_message_out(RM_CLIENT_RELEASE, RMConfig::RM_PRIORITY, source_id, source_id, service_id);

    sendout_control_message(rm_message_out, target_address); 
  
    RM_logInfo("Release sent: " << target_address.address() << " " << target_address.port())
}


/*
*
*/
void RMCommunication::send_ack(   
    udp::endpoint target_address, 
    uint32_t source_id,
    uint32_t destination_id,
    uint8_t mode,
    serviceID_t service_id
)
{
    RMMessage rm_message_out(ACK, RMConfig::RM_PRIORITY, source_id, destination_id, service_id, mode);
    sendout_control_message(rm_message_out, target_address);
}


/*
*
*/
void RMCommunication::send_nack(  
    udp::endpoint target_address, 
    uint32_t source_id,
    uint32_t destination_id, 
    serviceID_t service_id
)
{
    RMMessage rm_message_out(NACK, RMConfig::RM_PRIORITY, source_id, destination_id, service_id);
    sendout_control_message(rm_message_out, target_address);
}