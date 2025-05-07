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


#include <rscmng/rm_abstraction.hpp>


using namespace boost::placeholders;
using namespace rscmng;
using namespace wired;
using namespace basic_protocol;


/*
*
*/
RMAbstraction::RMAbstraction(
    struct rscmng::config::unit_settings &client_configuration,
    std::condition_variable &rm_notify
):
    rm_client_id(client_configuration.client_id),    
    rm_notification(rm_notify),
    rm_handler_running(false),   
    rm_client_configuration(client_configuration),
    control_channel(client_configuration.rm_control_local_ip[0], client_configuration.rm_control_local_port),
    central_rm_address(boost::asio::ip::address::from_string(client_configuration.rm_control_rm_ip[0]), client_configuration.rm_control_rm_port)
{

    RM_logInfo("RM client constructor done.")
}


/*
*
*/
RMAbstraction::~RMAbstraction()
{
    control_channel.close();
}


/*
*
*/
void RMAbstraction::rm_handler_start()
{   
    if(!rm_handler_running)
    {
        rm_receiver = std::thread{&RMAbstraction::rm_handler, this};
        rm_handler_running = true;
        RM_logInfo("RM client listen on: " << rm_client_configuration.rm_control_local_ip[0] << " " << rm_client_configuration.rm_control_local_port)
    }
}


/*
*
*/
void RMAbstraction::sync_request(MessageNet_t *rm_payload, serviceID_t service_id)
{   
    control_channel.send_sync_request(  
        central_rm_address,
        rm_client_id, 
        0,
        service_id,
        rm_payload
    );
}


/*
*
*/
void RMAbstraction::sync_ack_receive(MessageNet_t *rm_payload, serviceID_t service_id)
{   
    control_channel.send_sync_receive(  
        central_rm_address,
        rm_client_id, 
        0,
        service_id,
        rm_payload
    );
    RM_logInfo("RM client send ack")
}


/*
*
*/
void RMAbstraction::sync_ack_reconfigure_done(MessageNet_t *rm_payload, serviceID_t service_id)
{   
    control_channel.send_sync_reconfigure_done(  
        central_rm_address,
        rm_client_id, 
        0,
        service_id,
        rm_payload
    );
    RM_logInfo("RM client send ack")
}


/*
*
*/
void RMAbstraction::resource_request(MessageNet_t *rm_payload, application_identification app_meta, uint8_t message_mode)
{   
    control_channel.send_request(  
        central_rm_address,
        app_meta.entity_type, 
        app_meta.priority, 
        app_meta.client_id, 
        message_mode,
        app_meta.service_id,
        app_meta.protocol_id,
        rm_payload
    );

    if(!rm_handler_running)
    {
        rm_receiver = std::thread{&RMAbstraction::rm_handler, this};
        rm_handler_running = true;
    }
}


/*
*
*/
void RMAbstraction::resource_release(serviceID_t service_id)
{
    control_channel.send_release(
        central_rm_address,  
        RM_CLIENT_APP,
        rm_client_id, 
        service_id
    );
}


/*
*
*/
void RMAbstraction::send_ack(RMMessage message, application_identification app_meta, udp::endpoint endpoint_sender)
{
    control_channel.send_ack(endpoint_sender, message.source_id, app_meta.client_id, message.mode, message.service_id);
}


/*
*
*/
void RMAbstraction::rm_handler()
{
    rscmng::wired::RMMessage rm_message;
    udp::endpoint sender_endpoint;

    RM_logInfo("RM client wait for message ....")

    while(true)
    {
        sender_endpoint = control_channel.receive_control_message(rm_message);
        RM_logInfo("RM client message received")

        rm_client_out_queue.enqueue(rm_message);
        rm_notification.notify_one();
    }
}


/*
*
*/
bool RMAbstraction::check_next_control_message_aviable()
{
    bool is_message_left;
    if (rm_client_out_queue.empty() == true)
    {
        is_message_left == false;
    }
    else
    {
        is_message_left == true;
    }

    return is_message_left;
}


/*
*
*/
rscmng::wired::RMMessage RMAbstraction::get_next_control_message()
{
    //rscmng::wired::RMMessage message = rm_client_out_queue.dequeue();
    //message.print();
    return rm_client_out_queue.dequeue();
}


/*
*
*/
void RMAbstraction::close()
{
    control_channel.close();
}
