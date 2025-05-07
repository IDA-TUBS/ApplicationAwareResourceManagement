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


#ifndef BASIC_PAYLOAD_h
#define BASIC_PAYLOAD_h


#include <chrono>
#include <cstdint>

#include <boost/bind/bind.hpp>

#include <rscmng/data_sharing/message_net.hpp>
#include <rscmng/utils/log.hpp>


namespace rscmng {
    
    namespace basic_protocol {
        
        /**
         * @brief Storage class for basic/minimum BasicPayload parameters
         * 
         */
        class BasicPayload
        {
            public:

            /**
             * @brief Construct a new empty BasicPayload object
             * 
             */
            BasicPayload();

            /**
             * @brief Construct a new BasicPayload object
             * 
             * @param object_size sample size [Byte]
             * @param object_deadline sample deadline [ns]
             */
            BasicPayload(    
                uint32_t object_size, 
                std::chrono::nanoseconds object_deadline
            );
            
            /**
             * @brief Encodes the BasicPayload parameters into a byte stream for transmission
             * 
             * @param rm_payload_message char array for storing the byte stream
             */
            void serialize(MessageNet_t* rm_payload_message);

            /**
             * @brief Decodes a byte stream of BasicPayload parameters 
             *  
             * @param rm_payload_message char array containing the byte stream
             */
            void deserialize(MessageNet_t* rm_payload_message);

            /**
             * @brief Print the BasicPayload parameters using BOOST log
             * 
             */
            void print();

            /**
             * @brief Set the sample size object
             * 
             * @param object_size sample size [Byte]
             */
            void set_object_size(uint32_t object_size);

            /**
             * @brief Set the sample deadline object
             * 
             * @param object_deadline sample deadline [ns]
             */
            void set_object_deadline(std::chrono::nanoseconds object_deadline);

            /**
             * @brief Get the sample size object
             * 
             * @return uint32_t [Byte]
             */
            uint32_t get_sample_size();

            /**
             * @brief Get the sample deadline object
             * 
             * @return std::chrono::nanoseconds  
             */
            std::chrono::nanoseconds get_sample_deadline();

            protected:

            /**
             * @brief sample size attribute [Byte]
             * 
             */
            uint32_t object_size;

            /**
             * @brief sample deadline attribute [ns]
             * 
             */
            std::chrono::nanoseconds object_deadline;
            
        };

        /**
         * @brief Storage class for basic transmission parameters
         * 
         */
        class txParam
        {
            public:

            /**
             * @brief Construct a new empty tx Param object
             * 
             */
            txParam();

            /**
             * @brief Construct a new tx Param object
             * 
             * @param f_size fragment size [Byte]
             */
            txParam(uint32_t f_size);
            
            /**
             * @brief Set the fragment size object
             * 
             * @param f_size [Byte]
             */
            void set_fragment_size(uint32_t f_size);

            /**
             * @brief Get the fragment size object
             * 
             * @return uint32_t 
             */
            uint32_t get_fragment_size();

            /**
             * @brief Encodes the transmission parameters into a byte stream for transmission
             * 
             * @param rm_payload_message char array for storing the byte stream
             */
            void serialize(MessageNet_t* param_stream);

            /**
             * @brief Decodes a byte stream of transmission parameters 
             *  
             * @param rm_payload_message char array containing the byte stream
             */
            void deserialize(MessageNet_t* param_stream);

            /**
             * @brief Decodes a byte stream of updated BasicPayload parameters. Only non-default values are overwritten.
             * 
             * @param rm_payload_message char array containing the byte stream
             */
            void update(MessageNet_t* rm_payload_message);

            /**
             * @brief Print the parameter set
             * 
             */
            void print();


            protected:

            /**
             * @brief fragment size attribute
             * 
             */
            uint32_t fragment_size;
        };

        /**
         * @brief Protocol specific aRM level handler
         * 
         * @param rm_payload_message byte stream containing the requested BasicPayload
         * @return void* pointer to decoded BasicPayload object for storage
         */
        void* aRM_handler(MessageNet_t* rm_payload_message);

        /**
         * @brief Protocol specific nRM level handler
         * 
         * @param rm_payload_message byte stream containing the requested BasicPayload
         * @return void* pointer to decoded BasicPayload object for storage
         */
        void* nRM_handler(MessageNet_t* rm_payload_message);
    };

    namespace basic_periodic_protocol {
        
        /**
         * @brief Storage class for basic periodic application BasicPayload
         * 
         */
        class BasicPayload: public basic_protocol::BasicPayload
        {
            public:

            /**
             * @brief Construct a new empty BasicPayload object
             * 
             */
            BasicPayload();

            /**
             * @brief Construct a new Qo Sobject
             * 
             * @param object_size sample size [Byte]
             * @param object_deadline sample deadline [ns]
             * @param s_period sample period [ns]
             */
            BasicPayload(    uint32_t object_size, 
                    std::chrono::nanoseconds object_deadline, 
                    std::chrono::nanoseconds s_period
                );

            /**
             * @brief Encodes the BasicPayload parameters into a byte stream for transmission
             * 
             * @param rm_payload_message char array for storing the byte stream
             */
            void serialize(MessageNet_t* rm_payload_message);

            /**
             * @brief Decodes a byte stream of BasicPayload parameters 
             *  
             * @param rm_payload_message char array containing the byte stream
             */
            void deserialize(MessageNet_t* rm_payload_message);

            /**
             * @brief Print the BasicPayload parameters using BOOST log
             * 
             */
            void print();

            /**
             * @brief Set the sample period object
             * 
             * @param s_period sample period [ns]
             */
            void set_sample_period(std::chrono::nanoseconds s_period);

            /**
             * @brief Get the sample period object
             * 
             * @return std::chrono::nanoseconds 
             */
            std::chrono::nanoseconds get_sample_period();

            protected:
            /**
             * @brief Sample period attribute [ns]
             * 
             */
            std::chrono::nanoseconds sample_period;
        };

        /**
         * @brief Storage class for basic peridic application transmission parameters
         * 
         */
        class txParam: public basic_protocol::txParam
        {
            public:

            /**
             * @brief Construct a new empty tx Param object
             * 
             */
            txParam();
            
            /**
             * @brief Construct a new tx Param object
             * 
             * @param f_size fragment size [byte]
             * @param t_sh shaping time [ns]
             */
            txParam(    uint32_t f_size, 
                        std::chrono::nanoseconds t_sh
                    );

            /**
             * @brief Encodes the BasicPayload parameters into a byte stream for transmission
             * 
             * @param rm_payload_message char array for storing the byte stream
             */
            void serialize(MessageNet_t* param_stream);

            /**
             * @brief Decodes a byte stream of BasicPayload parameters 
             *  
             * @param rm_payload_message char array containing the byte stream
             */
            void deserialize(MessageNet_t* param_stream);

            /**
             * @brief Decodes a byte stream of updated BasicPayload parameters. Only non-default values are overwritten.
             * 
             * @param rm_payload_message char array containing the byte stream
             */
            void update(MessageNet_t* rm_payload_message);

            /**
             * @brief Print the parameter set
             * 
             */
            void print();

            /**
             * @brief Set the shaping time object
             * 
             * @param t_sh target shaping time [ns]
             */
            void set_shaping_time(std::chrono::nanoseconds t_sh);

            /**
             * @brief Get the shaping time object
             * 
             * @return std::chrono::nanoseconds 
             */
            std::chrono::nanoseconds get_shaping_time();

            protected:
            /**
             * @brief Shaping time attribute [ns]
             * 
             */
            std::chrono::nanoseconds shaping_time;

        };

        /**
         * @brief Protocol specific aRM level handler
         * 
         * @param rm_payload_message byte stream containing the requested BasicPayload
         * @return void* pointer to decoded BasicPayload object for storage
         */
        void* aRM_handler(MessageNet_t* rm_payload_message);

        /**
         * @brief Protocol specific nRM level handler
         * 
         * @param rm_payload_message byte stream containing the requested BasicPayload
         * @return void* pointer to decoded BasicPayload object for storage
         */
        void* nRM_handler(MessageNet_t* rm_payload_message);
    };

};

#endif