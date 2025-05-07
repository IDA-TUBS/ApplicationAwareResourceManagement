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


#ifndef RM_WIRED_BASIC_h
#define RM_WIRED_BASIC_h


#include <chrono>
#include <cstdint>
#include <ctime>

#include <rscmng/data_sharing/message_net.hpp>
#include <rscmng/protocols/basic_payload.hpp>
#include <rscmng/utils/log.hpp>



namespace rscmng {

    enum RMCommand
    {
        IDLE = 0,
        START = 1,
        STOP,
        PAUSE,
        RECONFIGURE,
        SYNC_TIMESTAMP_START,
        SYNC_TIMESTAMP_STOP,
        SYNC_TIMESTAMP_PAUSED,
        SYNC_TIMESTAMP_RECONFGURE,
        SYNC_TIMESTAMP_RECONFGURE_SOFT
    };

    namespace rm_wired_basic_protocol {



        /**
         * @brief Storage class for basic/minimum RM_Payload parameters
         * 
         */
        class RMPayload: public rscmng::basic_protocol::BasicPayload
        {
            public:

            struct network_resource_request
            {
                uint32_t client_id;              
                uint32_t service_id;  
                uint32_t priority;        
                double bandwidth;
                std::chrono::milliseconds deadline;
                std::vector<uint32_t> data_path;
                bool allocated;
            };

            /**
             * @brief Construct a new empty RM_Payload object
             * 
             */
            RMPayload();


            /**
             * @brief Construct a new BasicPayload object
             * 
             * @param object_size object size [Byte]
             * @param object_deadline sample deadline [ns]
             */
            RMPayload(    
                uint32_t object_size, 
                std::chrono::nanoseconds object_deadline,
                uint32_t stream_priority
            );


            /**
             * @brief Construct a new BasicPayload object
             * 
             * @param object_size object size [Byte]
             * @param object_deadline sample deadline [ns]
             */
            RMPayload(    
                uint32_t object_size, 
                std::chrono::nanoseconds object_deadline,
                uint32_t stream_priority,
                uint32_t measurement,
                RMCommand rm_client_action
            );


            /**
             * @brief Construct a new BasicPayload object
             * 
             * @param object_size object size [Byte]
             * @param object_deadline sample deadline [ns]
             */
            RMPayload(    
                uint32_t object_size, 
                std::chrono::nanoseconds object_deadline,
                uint32_t stream_priority,
                uint32_t measurement,
                RMCommand rm_client_action,
                struct timespec timestamp_stop,
                struct timespec timestamp_reconfig,
                struct timespec timestamp_start,
                struct network_resource_request resource_request_struct
            );


            /**
             * @brief Construct a new BasicPayload object
             * 
             * @param timestamp_stop 
             * @param timestamp_reconfig
             * @param timestamp_start
             */
            RMPayload(    
                RMCommand rm_client_action,
                struct timespec timestamp_stop,
                struct timespec timestamp_reconfig,
                struct timespec timestamp_start
            );

            RMPayload(
                RMCommand rm_client_action,
                struct network_resource_request resource_request_struct
            ); 
            

            /**
             * @brief Encodes the RM_Payload parameters into a byte stream for transmission
             * 
             * @param payload char array for storing the byte stream
             */
            void serialize(MessageNet_t* payload);


            /**
             * @brief Decodes a byte stream of RM_Payload parameters 
             *  
             * @param payload char array containing the byte stream
             */
            void deserialize(MessageNet_t* payload);


            /**
             * @brief Print the RM_Payload parameters using BOOST log
             * 
             */
            void print();

            /**
             * @brief 
             * 
             */
            void set_data_connection_priority(uint32_t priority);
            
            /**
             * @brief 
             * 
             */
            uint32_t get_data_connection_priority();

            /**
             * @brief 
             * 
             */
            void set_measurement_id(uint32_t measurement_id);
            
            /**
             * @brief 
             * 
             */
            uint32_t get_measurement_id();


            /**
             * @brief 
             * 
             */
            void set_timestamp_stop(struct timespec timestamp);
            
            /**
             * @brief 
             * 
             */
            struct timespec get_timestamp_stop();

            /**
             * @brief 
             * 
             */
            void set_timestamp_reconfig(struct timespec timestamp);
            
            /**
             * @brief 
             * 
             */
            struct timespec get_timestamp_reconfig();

            /**
             * @brief 
             * 
             */
            void set_timestamp_start(struct timespec timestamp);
 
            /**
             * @brief 
             * 
             */
            struct timespec get_timestamp_start();

            /**
             * @brief 
             * 
             */
            void set_resource_request_payload(struct network_resource_request resource_request_struct);

            /**
             * @brief 
             * 
             */
            struct network_resource_request get_resource_request_payload();


            protected:

            /**
             * @brief priority
             */
            uint32_t data_connection_priority;

            //measurement
            uint32_t measurement_id;

            //timestamp
            struct timespec timestamp_stop = {0,0}; 
            struct timespec timestamp_reconfig = {0,0}; 
            struct timespec timestamp_start = {0,0}; 
            
            //path
            network_resource_request resource_request_struct;
            RMCommand rm_client_action;
            
            
        };

    };

};

#endif