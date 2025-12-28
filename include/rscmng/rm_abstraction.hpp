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


#ifndef RM_ABSTRACTION_h
#define RM_ABSTRACTION_h


#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <ctime>


#include <rscmng/utils/log.hpp>
#include <rscmng/utils/config_reader.hpp>
#include <rscmng/abstraction/socket_endpoint.hpp>
#include <rscmng/messages.hpp>
#include <rscmng/rm_communication.hpp>
#include <rscmng/data_sharing/safe_map.hpp>
#include <rscmng/data_sharing/safe_queue.hpp>
#include <rscmng/data_sharing/message_net.hpp>
#include <rscmng/protocols/rm_wired_payload.hpp>


using boost::asio::ip::udp;


namespace rscmng {

    namespace wired {

        struct heartbeat_config
        {
            socket_endpoint socket;
            std::chrono::nanoseconds slack;
            int loss;

            heartbeat_config(){};

            heartbeat_config(
                std::string ip_addr,
                int port,
                std::chrono::nanoseconds hb_slack,
                int hb_loss
            ):
                socket(ip_addr, port),
                slack(hb_slack),
                loss(hb_loss)
            {};
        };


        struct application_identification
        {
            uint16_t client_id;
            //uint32_t endnode_id;
            uint8_t priority;            
            serviceID_t service_id;
            ProtocolIDs protocol_id;
            Participant entity_type;

            application_identification()
            : 
                //endnode_id(0x00000000),
                priority(0xFF),
                client_id(),
                service_id(0xFFFFFFFFFFFFFFFF),
                protocol_id(NONE),
                entity_type(NOT_DEFINED)
            {};

            application_identification(    
                //uint32_t endnode_id,
                uint8_t priority, 
                uint16_t client_id, 
                serviceID_t service_id, 
                ProtocolIDs protocol_id, 
                Participant participant_type
            ):
                //endnode_id(endnode_id),
                priority(priority),
                client_id(client_id),
                service_id(service_id),
                protocol_id(protocol_id),
                entity_type(participant_type)
            {};
        };


        class RMAbstraction
        {
            public:
            
            enum ControlState
            {
                IDLE,       // Initial State
                REQUEST,    // Request sent state
                HOLD,       // App Setup received state   
                ALLOCATION, // App Grant received state
                INIT,       // Grant received
                INIT_ADAPT, // Adapt received
                RUNNING,    // Sync Activation complete
                FALLBACK,   // Fallback
                STARTED,    // Receiving Handler started
                START_ARRIVED,
                STOP_ARRIVED,       
                PAUSE_ARRIVED,
                RECONFIGURE_ARRIVED,
                SYNC_TIMESTAMP_START_ARRIVED,
                SYNC_TIMESTAMP_STOP_ARRIVED,
                SYNC_TIMESTAMP_PAUSE_ARRIVED,
                SYNC_TIMESTAMP_RECONFIGURE_ARRIVED,
                RM_CLIENT_SYNC_REQUEST_ARRIVED,
                SYNC_TIMESTAMP_ARRIVED,
                SYNC_TIMESTAMP_REQUEST_ARRIVED,
                SYNC_TIMESTAMP_REQUEST_SEND,
            };


            /**
             * @brief Construct a new App Control object
             * 
             * @param network_environment Network Parameters
             * @param app_meta Application meta data
             * @param rm_payload MessageNet_t struct for protocol specific parameter parsing
             * @param rm_notify condition_variable to trigger state changes 
             * @param fallback flag to indicate whether the fallback monitor shall be active
             */
            RMAbstraction(
                struct rscmng::config::unit_settings &client_configuration,
                //MessageNet_t &rm_payload, 
                std::condition_variable &rm_notify
            );


            /**
             * @brief Destroy the App Control object
             * 
             */
            ~RMAbstraction();


            // Methods
            /**
             * @brief Start rm reciving handler
             * 
             * @param  
             */
            void rm_handler_start();


            /**
             * @brief Sends a request message to the RM
             * 
             * @param rm_payload requested rm_payload 
             */
            void sync_request(MessageNet_t *rm_payload, serviceID_t service_id);


            /**
             * @brief Sends a request message to the RM
             * 
             * @param rm_payload requested rm_payload 
             * @param service_id
             */
            void sync_ack_receive(MessageNet_t *rm_payload, serviceID_t service_id);


            /**
             * @brief Sends a request message to the RM
             * 
             * @param rm_payload requested rm_payload 
             * @param service_id
             */
            void sync_ack_reconfigure_done(MessageNet_t *rm_payload, serviceID_t service_id);


            /**
             * @brief Sends a release message to the RM
             * 
             * @param app_meta Application meta data
             */
            void resource_release(serviceID_t service_id);


            /**
             * @brief Send acknowledgement for resource management messages
             * 
             * @param message the message to be acknowledge
             * @param endpoint_sender the source endpoint of the message
             */
            void send_ack(RMMessage message, application_identification app_meta, udp::endpoint endpoint_sender);


            /*
            *
            */
            void send_error(uint32_t interface_id, uint32_t interface_speed);


            /**
             * @brief Generic Handler for resource management messages. To be used after successfull allocation
             * 
             */
            void rm_handler();


            /**
             * @brief 
             * 
             */
            bool check_next_control_message_aviable();


            /**
             * @brief 
             * 
             */
            rscmng::wired::RMMessage get_next_control_message();


            /**
             * @brief Wrapper for RMComm close. Closes control layer sockets
             * 
             */
            void close();


            private:

            uint32_t rm_client_id;

            struct rscmng::config::unit_settings rm_client_configuration;

            udp::endpoint central_rm_address; 
            
            rscmng::wired::RMCommunication control_channel;

            std::condition_variable &rm_notification;

            SafeQueue<rscmng::wired::RMMessage> rm_client_out_queue;

            std::thread rm_receiver;

            bool rm_handler_running;


        };

    };

};

#endif