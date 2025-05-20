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


#ifndef RM_COMMUNICATION_h
#define RM_COMMUNICATION_h


#include <boost/asio.hpp>
#include <string>
#include <chrono>
#include <mutex>
#include <iostream>
#include <bitset>

#include <rscmng/utils/log.hpp>
#include <rscmng/messages.hpp>
#include <rscmng/attributes/uuid.hpp>
#include <rscmng/attributes/demonstrator_config.hpp>
#include <rscmng/abstraction/socket_endpoint.hpp>
#include <rscmng/messages.hpp>
#include <rscmng/data_sharing/message_net.hpp>


using boost::asio::ip::udp;

namespace rscmng {

    namespace wired {

        /**
         * @brief Distinction between publisher and subscriber for request/release messages.
         * 
         */
        enum Participant 
        { 
            NOT_DEFINED = 0,
            SUBSCRIBER,
            PUBLISHER,
            RM_CLIENT_APP,
            RM_CLIENT_SWITCH,
            CENTRAL_RM
        };
        

        /**
         * @brief Provides the communication layer for messages regarding resource management. Based on BOOST ASIO
         * 
         */
        class RMCommunication
        {
            private:
            boost::asio::io_context rm_context;
            

            public:
            /**     
             * @brief Construct a new RMCommunication::RMCommunication object for loopback communication only
             * 
             * @param local_ip ip for control channel socket
             * @param local_port port for control channel socket
             */
            RMCommunication( 
                std::string local_ip,             
                uint32_t local_port
            );   


            /**     
             * @brief Construct a new RMCommunication::RMCommunication object for loopback communication only
             * 
             * @param broadcast_port port for control channel socket
             */
            RMCommunication(        
                uint32_t broadcast_port
            ); 


            boost::asio::ip::address address()
            {
                return control_channel_local_endpoint.address();
            }


            uint32_t port()
            {
                return control_channel_local_endpoint.port();
            }

            
            /**
             * @brief Sends a RMMessage via the objects control channel socket
             * 
             * @param message RMMessage object to transmit
             * @param target message receiver
             */
            void sendout_control_message(      
                RMMessage message, 
                udp::endpoint target_address
            );


            /**
             * @brief Sends a sync request message via the control channel 
             * 
             * @param target_address Target endpoint
             * @param source_id sender source_id 
             * @param mode
             * @param service_id
             * @param payload
             * 
             */
            void send_sync_request(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                uint8_t mode,
                serviceID_t service_id,
                MessageNet_t* payload
            );


            /*
            *
            */
            void send_sync_receive(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                uint8_t mode,
                serviceID_t service_id,
                MessageNet_t* payload
            );


            /*
            *
            */
            void send_sync_reconfigure_done(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                uint8_t mode,
                serviceID_t service_id,
                MessageNet_t* payload
            );


            /**
             * 
             */
            void send_request(RMMessage message);


            /**
             * @brief Sends a resource request to the resource manager (rm_endpoint). The function call blocks until either a grant, deny or app deny message has been received.
             * 
             * @param participant_type message_type of the application (publisher/subscriber)
             * @param priority network priority of the applications traffic
             * @param source_id source_id of the requesting application
             * @param service_id source_id of the requested service
             * @param protocol_id source_id of the employed protocol
             * @param payload defines which payload the application requests (an application might allow multiple payload)
             */
            void send_request(   
                udp::endpoint target_address, 
                Participant participant_type, 
                uint8_t priority,
                uint32_t source_id, 
                uint8_t mode,
                serviceID_t service_id,
                ProtocolIDs protocol_id, 
                MessageNet_t* payload
            );


            /**
             * @brief Sends a sync request to the resource manager (rm_endpoint). 
             * 
             * @param priority network priority of the applications traffic
             * @param source_id source_id of the requesting application
             * @param service_id source_id of the requested service
             * @param protocol_id source_id of the employed protocol
             * @param MessageTypes identifier for type of message
             * @param payload defines which payload the application requests (an application might allow multiple payload)
             */
            void send_rm_message(   
                udp::endpoint target_address, 
                uint8_t priority, 
                uint32_t source_id, 
                uint8_t mode,
                serviceID_t service_id,
                ProtocolIDs protocol_id,
                MessageTypes message_type,
                MessageNet_t* payload
            );


            /**
             * @brief Sends a sync request to the resource manager (rm_endpoint). 
             * 
             * @param target_address destination address
             * @param source_id source_id of the requesting application
             * @param service_id source_id of the requested service
             * @param payload defines which payload the application requests (an application might allow multiple payload)
             */       
            void send_start_sync(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                serviceID_t service_id,
                MessageNet_t* payload
            );


            /**
             * @brief Sends a sync request to the resource manager (rm_endpoint). 
             * 
             * @param target_address destination address
             * @param source_id source_id of the requesting application
             * @param service_id source_id of the requested service
             * @param payload defines which payload the application requests (an application might allow multiple payload)
             */       
            void send_start(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                serviceID_t service_id,
                MessageNet_t* payload
            );

            /**
             * @brief Sends a sync request to the resource manager (rm_endpoint). 
             * 
             * @param target_address destination address
             * @param source_id source_id of the requesting application
             * @param service_id source_id of the requested service
             * @param payload defines which payload the application requests (an application might allow multiple payload)
             */  
            void send_reconfig(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                serviceID_t service_id,
                uint32_t mode,
                MessageNet_t* payload
            );

            /**
             * @brief Sends a sync request to the resource manager (rm_endpoint). 
             * 
             * @param target_address destination address
             * @param source_id source_id of the requesting application
             * @param service_id source_id of the requested service
             * @param payload defines which payload the application requests (an application might allow multiple payload)
             */  
            void send_sync_reconfig(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                serviceID_t service_id,
                uint32_t mode,
                MessageNet_t* payload
            );

            /**
             * @brief Sends a sync request to the resource manager (rm_endpoint). 
             * 
             * @param target_address destination address
             * @param source_id source_id of the requesting application
             * @param service_id source_id of the requested service
             * @param payload defines which payload the application requests (an application might allow multiple payload)
             */  
            void send_sync_stop(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                serviceID_t service_id,
                uint32_t mode,
                MessageNet_t* payload
            );

            /**
             * @brief Sends a sync request to the resource manager (rm_endpoint). 
             * 
             * @param target_address destination address
             * @param source_id source_id of the requesting application
             * @param service_id source_id of the requested service
             * @param payload defines which payload the application requests (an application might allow multiple payload)
             */  
            void send_sync_stop_exit(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                serviceID_t service_id,
                uint32_t mode,
                MessageNet_t* payload
            );

            /**
             * @brief Sends a sync request to the resource manager (rm_endpoint). 
             * 
             * @param target_address destination address
             * @param source_id source_id of the requesting application
             * @param service_id source_id of the requested service
             * @param payload defines which payload the application requests (an application might allow multiple payload)
             */  
            void send_stop(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                serviceID_t service_id,
                uint32_t mode,
                MessageNet_t* payload
            );
            
            /**
             * @brief Sends a sync request to the resource manager (rm_endpoint). 
             * 
             * @param target_address destination address
             * @param source_id source_id of the requesting application
             * @param service_id source_id of the requested service
             * @param payload defines which payload the application requests (an application might allow multiple payload)
             */  
            void send_stop_exit(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                serviceID_t service_id,
                uint32_t mode,
                MessageNet_t* payload
            );

            /**
             * @brief Sends a sync request to the resource manager (rm_endpoint). 
             * 
             * @param priority network priority of the applications traffic
             * @param source_id source_id of the requesting application
             * @param service_id source_id of the requested service
             * @param protocol_id source_id of the employed protocol
             * @param MessageTypes identifier for type of message
             * @param payload defines which payload the application requests (an application might allow multiple payload)
             */ 
            void send_timestamp(   
                udp::endpoint target_address, 
                uint32_t source_id, 
                serviceID_t service_id,
                MessageNet_t* payload,
                MessageTypes message_type
            );

            /**
             * @brief Release to be used by the applications (publisher or subscriber)
             * 
             * @param participant_type message_type of the application which initiates the release
             * @param source_id ID of the application which initiates the release
             * @param request_id ID of the application which is to be informed about the release
             */
            void send_release(   
                udp::endpoint target_address, 
                Participant participant_type, 
                uint32_t source_id, 
                serviceID_t service_id
            );          

            /**
             * @brief Acknowledge the reception of resource manager messages. Implementation incomplete. Currently sparsely used.
             * 
             * @param sender_endpoint the address of ACK recipient
             * @param source_id source_id of the application sending the ACK
             * @param destination_id ID of the acknowleding entity
             * @param service_id ID ot the requested service
             */
            void send_ack(       
                udp::endpoint sender_endpoint, 
                uint32_t source_id,
                uint32_t destination_id,
                uint8_t mode,
                serviceID_t service_id
            );

            /**
             * @brief For future use
             * 
             * @param sender_endpoint 
             * @param source_id source_id of the application sending the NACK
             * @param destination_id ID of the acknowleding entity
             * @param service_id ID ot the requested service
             */
            void send_nack(  
                udp::endpoint sender_endpoint, 
                uint32_t source_id,
                uint32_t destination_id,
                serviceID_t service_id
            );

            /**
             * @brief receive resource management messages (from requesting application or the resource manager)
             * 
             * @param message RMMessage object used to store the received message
             * @return udp::endpoint 
             */
            udp::endpoint receive_control_message(struct RMMessage &message);

            /**
             * @brief receive resource management messages (from requesting application or the resource manager). Non blocking overload
             * 
             * @param message RMMessage object used to store the received message
             * @param active flag indicating whether the thread executing the recv function shall terminate
             * @return udp::endpoint 
             */
            udp::endpoint receive_control_message(   
                RMMessage &message, 
                bool &active
            );

            /**
             * @brief close the socket used for resource management
             * 
             */
            void close();

            uint32_t source_id;

            /**
            * @brief socket for sending rm control channel messages
            */
            udp::socket control_channel_socket;

            /**
            * @brief endpoint for sending rm control channel messages
            */
            udp::endpoint control_channel_local_endpoint;


            udp::endpoint broadcast;
            size_t rm_message_len;      
            std::mutex send_lock;
        };

    };

};

#endif