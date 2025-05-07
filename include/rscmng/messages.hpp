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


#ifndef MESSAGES_h
#define MESSAGES_h


#include <string>
#include <chrono>

#include <rscmng/data_sharing/message_net.hpp>
#include <rscmng/protocols/protocols.hpp>
#include <rscmng/attributes/uuid.hpp>
#include <rscmng/utils/log.hpp>
#include <rscmng/attributes/demonstrator_config.hpp>

namespace rscmng {

    /**
     * @brief RM message types
     * 
     */
    enum MessageTypes 
    { 
        NOOP = 0,

        //Timestamp
        SYNC_TIMESTAMP,
        RM_CLIENT_START,
        RM_CLIENT_STOP,
        RM_CLIENT_PAUSE,
        RM_CLIENT_RECONFIGURE,
        RM_CLIENT_SYNC_TIMESTAMP_START,
        RM_CLIENT_SYNC_TIMESTAMP_STOP,
        RM_CLIENT_SYNC_TIMESTAMP_PAUSE,
        RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE,
        RM_CLIENT_SYNC_TIMESTAMP_RECONFIGURE_SOFT,
        RM_CLIENT_SYNC_REQUEST,
        RM_CLIENT_SYNC_RECEIVE,
        RM_CLIENT_SYNC_RECONFIGURE_DONE,

        RM_CLIENT_REQUEST,
        RM_CLIENT_RELEASE,

        ACK,
        NACK

    };


    namespace wired {
        
        /**
         * @brief resource management messages (aRM layer)
         * 
         */
        class RMMessage
        {
            public:
            enum MessageTypes message_type;
            uint8_t priority;
            uint32_t source_id;
            uint32_t destination_id;
            serviceID_t service_id;            
            uint8_t mode;            
            std::chrono::system_clock::time_point time_stamp;
            ProtocolIDs protocol_id;
            uint16_t payload_size;
            static const uint16_t max_payload =  RMConfig::max_payload;
            char payload[RMConfig::max_payload];

            uint32_t length =    sizeof(message_type) + 
                            sizeof(priority) + 
                            sizeof(source_id) +
                            sizeof(destination_id) + 
                            sizeof(mode) +
                            sizeof(service_id) +
                            sizeof(time_stamp) + 
                            sizeof(protocol_id) + 
                            payload_size;

            /**
             * @brief Construct a new (empty) rm Message object
             * 
             */
            RMMessage();

            /**
             * @brief Construct a new rm Message object
             * 
             * @param message_type message_type of the message
             * @param priority priority of the traffic 
             * @param source_id ID of the request
             * @param destination_id ID of the application sending the message (source)
             * @param req_app ID of the receiving application (destination)
             */
            RMMessage(  
                MessageTypes message_type, 
                uint8_t priority, 
                uint32_t source_id,
                uint32_t destination_id,
                serviceID_t req_app
            );

            /**
             * @brief Construct a new rm Message object
             * 
             * @param message_type message_type of the message
             * @param priority priority of the traffic 
             * @param source_id ID of the request
             * @param destination_id ID of the application sending the message (source)
             * @param req_app ID of the receiving application (destination)
             * @param mode The mode corresponding to the supplied payload
             */
            RMMessage(  
                MessageTypes message_type, 
                uint8_t priority, 
                uint32_t source_id,
                uint32_t destination_id,
                serviceID_t req_app,
                uint8_t mode
            );

            /**
             * @brief Construct a new rm Message object
             * 
             * @param message_type message_type of the message
             * @param priority priority of the traffic 
             * @param source_id ID of the request
             * @param destination_id ID of the application sending the message (source)
             * @param mode The mode corresponding to the supplied payload
             * @param req_app ID of the receiving application (destination)
             * @param protocol_id ID of the transmission protocol used (reference for payload decoding)
             * @param rm_payload_message byte stream of the supplied payload parameters
             */
            RMMessage(  
                MessageTypes message_type, 
                uint8_t priority, 
                uint32_t source_id,
                uint32_t destination_id,
                serviceID_t req_app,
                uint8_t mode,
                ProtocolIDs protocol_id, 
                MessageNet_t* rm_payload_message
            );

            /**
             * @brief Destroy the rm Message object
             * 
             */
            ~RMMessage();

            /**
             * @brief Convert rm message to char array
             * 
             * @param message char array to store the byte stream
             */
            void rm_to_network(MessageNet_t* message);

            // Convert char array back to rm message struct
            /**
             * @brief Convert char array (byte stream) back to rm message object
             * 
             * @param message char array holding the byte stream
             */
            void network_to_rm(MessageNet_t* message);

            /**
             * @brief Change the ID of the sendig application (used for forwarding in gateways)
             * 
             * @param new_id Source ID in the next network segment
             */
            void change_id(uint32_t new_id);
            
            /**
             * @brief Change the ID of the receiving application (used for forwarding in gateways)
             * 
             * @param new_id Destination ID in the next network segment
             */
            void change_service(serviceID_t new_id);

            /**
             * @brief Set the timestamp object (for adapt)
             * 
             */
            void set_timestamp(std::chrono::system_clock::time_point tp);

            /**
             * @brief Get the size of the RMMessage
             * 
             * @return size_t 
             */
            size_t size();

            /**
             * @brief print the message attributes 
             * 
             */
            void print();

            /**
             * @brief clear the message attributes
             * 
             */
            void clear();
        };

    };


    /**
     * @brief Message object for sensor data
     * 
     */
    class DataMessage
    {
        public:
        uint8_t priority;
        uint32_t source_id;
        serviceID_t service_id;
        uint32_t object_number;
        uint32_t fragment_number;
        uint32_t total_fragments;
        struct timespec timestamp;
        std::chrono::system_clock::time_point time_stamp;
        uint32_t payload_length;
        uint32_t header_length = sizeof(priority) + 
                            sizeof(source_id) + 
                            sizeof(service_id) + 
                            sizeof(object_number) + 
                            sizeof(fragment_number) + 
                            sizeof(total_fragments) +
                            sizeof(timestamp) +
                            sizeof(time_stamp);
        uint32_t length = header_length + payload_length;
        char payload[rscmng::max_length-21];



        
        /**
         * @brief Convert data message to char array (byte stream)
         * 
         * @param message char array to store the byte stream
         */
        void dataToNet(char* message);

        /**
         * @brief Convert byte stream to data message object
         * 
         * @param message char array containing the byte stream
         * @param size byte stream length
         */
        void netToData(char* message, size_t size);
        
        /**
         * @brief Set the payload of the data message
         * 
         * @param frag_num sequence number of the fragment
         * @param total_num total fragments per sample
         * @param payload byte stream containing the data to be sent
         * @param payload_size size of the payload byte stream
         */
        void set_payload(   
            uint32_t sample_num, 
            uint32_t frag_num, 
            uint32_t total_num, 
            char* payload, 
            size_t payload_size
        );


        /**
         * @brief Set the payload of the data message
         * 
         * @param frag_num sequence number of the fragment
         * @param total_num total fragments per sample
         * @param payload byte stream containing the data to be sent
         * @param payload_size size of the payload byte stream
         * @param timestamp
         */
        void set_payload(   
            uint32_t sample_num, 
            uint32_t frag_num, 
            uint32_t total_num, 
            char* payload, 
            size_t payload_size,
            struct timespec timestamp
        );


        /**
         * @brief Change source ID of the data message. Used for gateway forwarding
         * 
         * @param new_id translated ID for the next network segment
         */
        void change_source_id(uint32_t new_id);

        /**
         * @brief Change destination ID of the data message. Used for gateway forwarding
         * 
         * @param new_id translated ID for the next network segment
         */
        void change_service(serviceID_t new_id);

        /**
         * @brief Get the size of the DataMessage
         * 
         * @return size_t 
         */
        size_t size();

        /**
         * @brief Get the size of the DataMessage header
         * 
         * @return size_t 
         */
        size_t header_size();

        /**
         * @brief print the data message attributes
         * 
         */
        void print();

        /**
         * @brief print the data message attributes with delimiter
         * 
         * @param len number of payload bytes to be printed
         */
        void print(size_t len);

        /**
         * @brief print the header of the data message object
         * 
         */
        void print_header();

        /**
         * @brief clear the data message object
         * 
         */
        void clear();

        /**
         * @brief Construct a new (empty) data Message object
         * 
         */
        DataMessage();

        /**
         * @brief Construct a new data Message object
         * 
         * @param priority traffic priority
         * @param client_id ID of the transmitting application
         * @param service_id ID of the application
         * @param sample_num sequence number of the sample
         * @param frag_num sequence number of the fragment
         * @param total_num total fragments per sample
         * @param payload payload byte stream (char array)
         * @param payload_size payload length
         */
        DataMessage(    
            uint8_t priority, 
            uint32_t client_id, 
            serviceID_t service_id, 
            uint32_t sample_num, 
            uint32_t frag_num, 
            uint32_t total_num, 
            char* payload, 
            size_t payload_size
        );
        
        
        /**
         * @brief Construct a new data Message object
         * 
         * @param priority traffic priority
         * @param client_id ID of the transmitting application
         * @param service_id ID of the application
         * @param sample_num sequence number of the sample
         * @param frag_num sequence number of the fragment
         * @param total_num total fragments per sample
         * @param payload payload byte stream (char array)
         * @param payload_size payload length
         * @param timestamp
         */
        DataMessage(    
            uint8_t priority, 
            uint32_t client_id, 
            serviceID_t service_id, 
            uint32_t sample_num, 
            uint32_t frag_num, 
            uint32_t total_num, 
            char* payload, 
            size_t payload_size,
            struct timespec timestamp
        );
    };


    /**
     * @brief Data structure for nRM mesages (allocation request, deallocation request, slot adaption request)
     * 
     */
    struct nRM_request
    {
        enum MessageTypes message_type;
        uint8_t priority;
        UUID_t source_id;
        std::chrono::system_clock::time_point time_stamp;
        serviceID_t service_id;
        ProtocolIDs protocol;
        char payload[RMConfig::max_payload];
        uint16_t payload_size;
        static const uint16_t max_payload =  RMConfig::max_payload;

        nRM_request()
        {}

        // Constructor for nRM initiated ADAPT request
        nRM_request(MessageTypes r_type, UUID_t r_id, uint8_t priority)
        {
            message_type = r_type;
            source_id = r_id;
            time_stamp = std::chrono::system_clock::now();
            service_id = 0x00;
            priority = priority;
            protocol = NONE;
            payload_size = 0;
        }

        void print()
        {
            RM_logInfo("Type: " << message_type)                 
            RM_logInfo("ID: " << source_id)
            RM_logInfo("Service: " << service_id)
            RM_logInfo("Priority: " << unsigned(priority))
            RM_logInfo("Protocol: " << protocol)
        }

        void clear()
        {
            message_type = NOOP;
            priority = 0xFF;
            memset(source_id.value, 0xFF, source_id.size);
            service_id = 0xFFFFFFFFFFFFFFFF;    
            protocol = NONE;
            memset(payload, 0, RMConfig::max_payload);
            payload_size = 0;
        }


    };


    /**
     * @brief Data structure for nRM reply messages
     * 
     * @tparam T the allocated or deallocated network load. Unit depends on the underlying network.
     */
    template<typename T>
    struct nRM_reply
    {
        // TODO: Add Constructor and methods for reply creation
        
        enum MessageTypes message_type;
        UUID_t source_id;
        uint8_t mode;
        std::chrono::system_clock::time_point change_tp;
        serviceID_t service_id;
        ProtocolIDs protocol;
        T network_load;
        char payload[RMConfig::max_payload];
        uint16_t payload_size;

        void set_change_tp(std::chrono::system_clock::time_point c_tp)
        {
            change_tp = c_tp;
        }

    };

    
};

#endif