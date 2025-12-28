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


#include <rscmng/traffic_generator.hpp>


using namespace rscmng;
using namespace wired; 
using namespace traffic_generator;
using namespace boost::placeholders;


/*
*
*/
TrafficGenerator::TrafficGenerator(
    std::map<uint32_t, struct rscmng::config::service_settings> service_settings,
    struct rscmng::config::unit_settings client_configuration,
    struct rscmng::config::experiment_parameter experiment_parameter,
    TrafficSourceType traffic_type,
    traffic_generator_parameter traffic_pattern
):
    service_settings_struct(service_settings),
    client_configuration_struct(client_configuration),
    experiment_parameter_struct(experiment_parameter),
    traffic_source_type(traffic_type),
    traffic_socket(traffic_context),
    traffic_pattern(traffic_pattern),
    traffic_endpoint_local(udp::endpoint(boost::asio::ip::address::from_string(client_configuration.rm_control_local_ip[0]), 10000))   
{
    traffic_socket.open(traffic_endpoint_local.protocol());
    traffic_socket.set_option(boost::asio::socket_base::reuse_address(true));
    traffic_socket.set_option(boost::asio::socket_base::broadcast(true));
    traffic_socket.bind(traffic_endpoint_local);

    mode_global = 0;
    stop_thread = false;
    global_period_timestamp = {0,0};
    global_update_period = false;

    RM_logInfo("Traffic Generator constructor done.")
}


/*
*
*/
TrafficGenerator::~TrafficGenerator()
{

}


/*
*
*/
void TrafficGenerator::start() 
{
    traffic_generator = std::thread{&TrafficGenerator::thread_type_select, this};
}


/*
*
*/
void TrafficGenerator::stop() 
{
    {
        std::lock_guard<std::mutex> lock(global_mutex);
        traffic_generator_control = THREAD_STOP;
        stop_thread = true;
    }
    generator_conditioning_variable.notify_one();
    traffic_generator.join();
}


/*
*
*/
void TrafficGenerator::notify_generator(TrafficSourceControl state) 
{
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            traffic_generator_control = state;
        }
        generator_conditioning_variable.notify_one();
        RM_logInfo("Traffic Generator recieve notification: " << state)
}


/*
*change for not sync case
*/
void TrafficGenerator::notify_generator_mode_change(TrafficSourceControl state, uint32_t mode) 
{
        {
            std::lock_guard<std::mutex> lock(global_mutex);            
            mode_global = mode;
            traffic_generator_control = state;         
        }                
        generator_conditioning_variable.notify_one();
        RM_logInfo("Traffic Generator Reconfiguration with new mode: " << std::to_string(mode))
        
}


/*
* used for sync mode change
*/
void TrafficGenerator::notify_generator_timestamp(TrafficSourceControl state, struct timespec timestamp, uint32_t mode, bool update_period)
{
        {
            std::lock_guard<std::mutex> lock(global_mutex);            
            mode_global = mode;
            global_update_period = update_period;
            global_period_timestamp = timestamp;
            traffic_generator_control = state;         
        }                
        generator_conditioning_variable.notify_one();
        RM_logInfo("Traffic Generator Reconfiguration with new mode: " << std::to_string(mode) << " at timestamp " << timestamp.tv_sec  << " s, " << timestamp.tv_nsec << " ns")
}


/*
*
*/
void TrafficGenerator::precise_wait_us(double microseconds) 
{
    auto start = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::micro>(microseconds);
    
    while (std::chrono::high_resolution_clock::now() - start < duration);
}


/*
*
*/
void TrafficGenerator::send_objects_dynamic_change()
{
    RM_logInfo("Traffic Generator Sending Thread " << std::to_string(thread_id) << " ObjectsBurstShaped IP started");

    // Time vars
    struct timespec time_now = {0,0};
    struct timespec time_send = {0,0};
    auto object_transmission_start = std::chrono::steady_clock::now();
    auto current_time = std::chrono::steady_clock::now();
    auto local_timepoint = std::chrono::steady_clock::now();
    struct timespec local_timestamp;

    long number_object = 1;
    boost::asio::io_context io_context;
    boost::asio::ip::udp::resolver resolver(io_context);
    boost::asio::ip::udp::endpoint traffic_endpoint_target_new;

    // Per-thread buffers (reserve once)
    std::vector<char> dummy_payload(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH, 'A');
    std::vector<char> data_message_buffer;
    data_message_buffer.reserve(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);

    // Log available modes
    for (auto& setting_iterator : service_settings_struct)
    {
        uint32_t mode_id = setting_iterator.first;
        auto& service_struct_iterator = setting_iterator.second;

        service_struct_iterator.number_packets = static_cast<uint32_t>((service_struct_iterator.object_size * 1024 + demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH - 1) / demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
        service_struct_iterator.estimated_transmission_time_ms = service_struct_iterator.number_packets * (service_struct_iterator.inter_packet_gap.count() / 1e6 + demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH * 8.0 / 1e9) * 1e3;
               
        RM_logInfo("########");
        RM_logInfo("# Sending Mode      : " << std::to_string(mode_id));
        RM_logInfo("# Sending Thread Mode, Thread ID " << std::to_string(thread_id) << " Destination IP : " << service_struct_iterator.ip_address << "  " << service_struct_iterator.port);
        RM_logInfo("# Service ID        : " << service_struct_iterator.service_id);
        RM_logInfo("# Deadline          : " << service_struct_iterator.deadline << " ms");
        RM_logInfo("# Object size       : " << service_struct_iterator.object_size << " KB");
        RM_logInfo("# Number of packets : " << service_struct_iterator.number_packets);
        RM_logInfo("# Inter packet gap  : " << service_struct_iterator.inter_packet_gap.count());
        RM_logInfo("# Slot offset       : " << service_struct_iterator.slot_offset);
        RM_logInfo("# Slot length       : " << service_struct_iterator.slot_length);
        RM_logInfo("# Estimated object transmission time : " << service_struct_iterator.estimated_transmission_time_ms << " ms");
        RM_logInfo("########");
    }

    // Validate initial mode_global
    uint32_t current_mode = -1;
    rscmng::config::service_settings current_settings;
    const rscmng::config::service_settings* current_settings_ptr = nullptr;

    traffic_pattern.info_flag = false;
    RM_logInfo("Traffic Generator ready for loop!") 
    while (true)
    {
        // Wait for permission to send
        std::unique_lock<std::mutex> lock(global_mutex);
        generator_conditioning_variable.wait(lock, [&] {
            return (traffic_generator_control == THREAD_TRANSMISSION);
        });
        lock.unlock();


        if (current_mode != mode_global)
        {
            RM_logInfo("Traffic Generator change mode to: " << std::to_string(mode_global));

            current_mode = mode_global;
            auto iterator = service_settings_struct.find(current_mode);
            if (iterator != service_settings_struct.end())
            {
                current_settings_ptr = &iterator->second;
            }
            else
            {
                RM_logInfo("Traffic Generator mode_global points to unknown setting: " << std::to_string(current_mode));
                break;
            }

            const auto& settings = *current_settings_ptr;
            RM_logInfo("########");
            RM_logInfo("# Sending Mode      : " << std::to_string(iterator->first));
            RM_logInfo("# Sending Thread Mode, Thread ID " << std::to_string(thread_id) << " Destination IP : " << settings.ip_address << "  " << settings.port);
            RM_logInfo("# Service ID        : " << settings.service_id);
            RM_logInfo("# Deadline          : " << settings.deadline << " ms");
            RM_logInfo("# Object size       : " << settings.object_size << " KB");
            RM_logInfo("# Number of packets : " << settings.number_packets);
            RM_logInfo("# Inter packet gap  : " << settings.inter_packet_gap.count());
            RM_logInfo("# Slot offset       : " << settings.slot_offset);
            RM_logInfo("# Slot length       : " << settings.slot_length);
            RM_logInfo("# Estimated object transmission time : " << settings.estimated_transmission_time_ms << " ms");
            RM_logInfo("########");

            // Rebuild the endpoint dynamically
            auto results = resolver.resolve(
                boost::asio::ip::udp::v4(),
                settings.ip_address,
                std::to_string(settings.port)
            );

            // Pick first resolved address
            traffic_endpoint_target_new = *results.begin();
            //update period
            if (global_update_period)
            {
                local_timestamp = global_period_timestamp;
                local_timepoint = std::chrono::steady_clock::time_point{
                std::chrono::seconds(global_period_timestamp.tv_sec) + std::chrono::nanoseconds(global_period_timestamp.tv_nsec)
                };
                global_update_period = false;
            }
            
            RM_logInfo("Traffic Generator Updated endpoint to " << traffic_endpoint_target_new.address().to_string() << ":" << traffic_endpoint_target_new.port())
            RM_logInfo("Traffic Generator Updated period   to " << local_timestamp.tv_sec << " s " << local_timestamp.tv_nsec << " ns")
        }
        
        // Convert sizes to bytes and compute fragments
        uint64_t remaining_bytes = current_settings_ptr->object_size * 1024;
        size_t payload_size = static_cast<size_t>(std::min<uint64_t>(remaining_bytes, demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH));
        uint32_t number_packet = 0;

        DataMessage data_message(
            current_settings_ptr->service_priority,
            client_configuration_struct.client_id,
            current_settings_ptr->service_id,
            number_object,
            number_packet,
            current_settings_ptr->number_packets,
            dummy_payload.data(),    // pointer to payload; ensure you only read payload_size bytes
            payload_size,
            time_send
        );

        // Ensure data_message_buffer is large enough
        if (data_message.length > data_message_buffer.size())
        {
            data_message_buffer.resize(data_message.length);
        }
 
        //uint8_t interface_swap = 0;
        clock_gettime(CLOCK_REALTIME, &time_now);
        RM_logInfo("Traffic Genarator Object: " << number_object << " transmission start now: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns");
        
        while(number_packet < current_settings_ptr->number_packets)
        {
            // Wait for permission to send
            std::unique_lock<std::mutex> lock(global_mutex);
            generator_conditioning_variable.wait(lock, [&] {
                return (traffic_generator_control == THREAD_TRANSMISSION) || 
                        traffic_generator_control == THREAD_TRANSMISSION_FINISH_OBJECT;
            });
            lock.unlock();

            clock_gettime(CLOCK_REALTIME, &time_send);
            data_message.set_timestamp(time_send);            

            // Sendout
            data_message.dataToNet(data_message_buffer.data());
            traffic_socket.send_to(boost::asio::buffer(data_message_buffer.data(), data_message.length), traffic_endpoint_target_new);


            ++number_packet;
            payload_size = static_cast<size_t>(std::min<uint64_t>(remaining_bytes, demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH));
            data_message.set_payload_size(payload_size);
            data_message.set_packet_number(number_packet);

            // Ensure data_message_buffer is large enough
            if (data_message.length > data_message_buffer.size())
            {
                data_message_buffer.resize(data_message.length);
            }
            // Update remaining bytes
            if (remaining_bytes > payload_size)
            {
                remaining_bytes -= payload_size;
            }
            else
            {
                remaining_bytes = 0;
            }


            if (traffic_pattern.info_flag)
            {
                clock_gettime(CLOCK_REALTIME, &time_now);
                RM_logInfo("Mode " << std::to_string(current_mode) << " Fragment " << number_packet << " from Object: " << number_object << " start now: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns");
            }

            if (stop_thread)
            {
                RM_logInfo("Traffic Generator Thread stopped during transmitting object: " << number_object << " fragment number: " << number_packet);
                break;
            }
            // Pace
            precise_wait_us(traffic_pattern.inter_packet_gap.count());
        }   
        
        if (traffic_pattern.auto_traffic_termination > 0 && number_object >= traffic_pattern.auto_traffic_termination)
        {
            break;
        }      

        number_object++;

        if (stop_thread)
        {
            break;
        }

        struct timespec target_time = local_timestamp;
        target_time.tv_nsec += current_settings_ptr->deadline * 1000000L;
        if (target_time.tv_nsec >= 1000000000L) 
        {
            target_time.tv_sec += target_time.tv_nsec / 1000000000L;
            target_time.tv_nsec %= 1000000000L;
        }
        if (traffic_pattern.info_flag)
        {
            RM_logInfo("target time before waiting : " << target_time.tv_sec << " s, " << target_time.tv_nsec << " ns");
            clock_gettime(CLOCK_REALTIME, &time_now);
            RM_logInfo("current_time before waiting: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns");
        }

        while (true)
        {
            if (traffic_generator_control == THREAD_TRANSMISSION_FINISH_OBJECT)
            {
                RM_logInfo("Traffic Generator Thread interrupted by change");
                break;
            }

            clock_gettime(CLOCK_REALTIME, &time_now);

            if ((time_now.tv_sec > target_time.tv_sec) ||
                (time_now.tv_sec == target_time.tv_sec && time_now.tv_nsec >= target_time.tv_nsec))
            {
                break;
            }
        }

        if (traffic_pattern.info_flag)
        {
            clock_gettime(CLOCK_REALTIME, &time_now);
            RM_logInfo("current_time after waiting : " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns");
        }

        local_timestamp = target_time;

    } // while(true)

    traffic_socket.close();
    RM_logInfo("Traffic Generator Closing of sending thread: " << thread_id)
}


/*
*
*/
void TrafficGenerator::send_objects_dynamic_change_asynchron()
{
    RM_logInfo("Traffic Generator Sending Thread " << std::to_string(thread_id) << " ObjectsBurstShaped IP started");

    // Time vars
    struct timespec time_now = {0,0};
    struct timespec time_send = {0,0};
    auto object_transmission_start = std::chrono::steady_clock::now();
    auto current_time = std::chrono::steady_clock::now();
    auto local_timepoint = std::chrono::steady_clock::now();
    struct timespec local_timestamp;

    long number_object = 1;
    boost::asio::io_context io_context;
    boost::asio::ip::udp::resolver resolver(io_context);
    boost::asio::ip::udp::endpoint traffic_endpoint_target_new;

    // Per-thread buffers (reserve once)
    std::vector<char> dummy_payload(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH, 'A');
    std::vector<char> data_message_buffer;
    data_message_buffer.reserve(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);

    // Log available modes
    for (auto& setting_iterator : service_settings_struct)
    {
        uint32_t mode_id = setting_iterator.first;
        auto& service_struct_iterator = setting_iterator.second;

        service_struct_iterator.number_packets = static_cast<uint32_t>((service_struct_iterator.object_size * 1024 + demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH - 1) / demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
        service_struct_iterator.estimated_transmission_time_ms = service_struct_iterator.number_packets * (service_struct_iterator.inter_packet_gap.count() / 1e6 + demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH * 8.0 / 1e9) * 1e3;
               
        RM_logInfo("########");
        RM_logInfo("# Sending Mode      : " << std::to_string(mode_id));
        RM_logInfo("# Sending Thread Mode, Thread ID " << std::to_string(thread_id) << " Destination IP : " << service_struct_iterator.ip_address << "  " << service_struct_iterator.port);
        RM_logInfo("# Service ID        : " << service_struct_iterator.service_id);
        RM_logInfo("# Deadline          : " << service_struct_iterator.deadline << " ms");
        RM_logInfo("# Object size       : " << service_struct_iterator.object_size << " KB");
        RM_logInfo("# Number of packets : " << service_struct_iterator.number_packets);
        RM_logInfo("# Inter packet gap  : " << service_struct_iterator.inter_packet_gap.count());
        RM_logInfo("# Slot offset       : " << service_struct_iterator.slot_offset);
        RM_logInfo("# Slot length       : " << service_struct_iterator.slot_length);
        RM_logInfo("# Estimated object transmission time : " << service_struct_iterator.estimated_transmission_time_ms << " ms");
        RM_logInfo("########");
    }

    // Validate initial mode_global
    uint32_t current_mode = -1;
    rscmng::config::service_settings current_settings;
    const rscmng::config::service_settings* current_settings_ptr = nullptr;

    traffic_pattern.info_flag = false;
    RM_logInfo("Traffic Generator ready for loop!") 
    while (true)
    {
        // Wait for permission to send
        std::unique_lock<std::mutex> lock(global_mutex);
        generator_conditioning_variable.wait(lock, [&] {
            return (traffic_generator_control == THREAD_TRANSMISSION);
        });
        lock.unlock();
       
        if (current_mode != mode_global)
        {
            RM_logInfo("Traffic Generator change mode to: " << std::to_string(mode_global));

            current_mode = mode_global;
            auto iterator = service_settings_struct.find(current_mode);
            if (iterator != service_settings_struct.end())
            {
                current_settings_ptr = &iterator->second;
            }
            else
            {
                RM_logInfo("Traffic Generator mode_global points to unknown setting: " << std::to_string(current_mode));
                break;
            }

            const auto& settings = *current_settings_ptr;
            RM_logInfo("########");
            RM_logInfo("# Sending Mode      : " << std::to_string(iterator->first));
            RM_logInfo("# Sending Thread Mode, Thread ID " << std::to_string(thread_id) << " Destination IP : " << settings.ip_address << "  " << settings.port);
            RM_logInfo("# Service ID        : " << settings.service_id);
            RM_logInfo("# Deadline          : " << settings.deadline << " ms");
            RM_logInfo("# Object size       : " << settings.object_size << " KB");
            RM_logInfo("# Number of packets : " << settings.number_packets);
            RM_logInfo("# Inter packet gap  : " << settings.inter_packet_gap.count());
            RM_logInfo("# Slot offset       : " << settings.slot_offset);
            RM_logInfo("# Slot length       : " << settings.slot_length);
            RM_logInfo("# Estimated object transmission time : " << settings.estimated_transmission_time_ms << " ms");
            RM_logInfo("########");

            // Rebuild the endpoint dynamically
            auto results = resolver.resolve(
                boost::asio::ip::udp::v4(),
                settings.ip_address,
                std::to_string(settings.port)
            );

            // Pick first resolved address
            traffic_endpoint_target_new = *results.begin();
            //update period
            if (global_update_period)
            {
                local_timestamp = global_period_timestamp;
                local_timepoint = std::chrono::steady_clock::time_point{
                std::chrono::seconds(global_period_timestamp.tv_sec) + std::chrono::nanoseconds(global_period_timestamp.tv_nsec)
                };
                global_update_period = false;
            }
            
            RM_logInfo("Traffic Generator Updated endpoint to " << traffic_endpoint_target_new.address().to_string() << ":" << traffic_endpoint_target_new.port())
            RM_logInfo("Traffic Generator Updated period   to " << local_timestamp.tv_sec << " s " << local_timestamp.tv_nsec << " ns")
        }

        
        // Convert sizes to bytes and compute fragments
        uint64_t remaining_bytes = current_settings_ptr->object_size * 1024;
        size_t payload_size = static_cast<size_t>(std::min<uint64_t>(remaining_bytes, demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH));
        uint32_t number_packet = 0;

        DataMessage data_message(
            current_settings_ptr->service_priority,
            client_configuration_struct.client_id,
            current_settings_ptr->service_id,
            number_object,
            number_packet,
            current_settings_ptr->number_packets,
            dummy_payload.data(),    // pointer to payload; ensure you only read payload_size bytes
            payload_size,
            time_send
        );

        // Ensure data_message_buffer is large enough
        if (data_message.length > data_message_buffer.size())
        {
            data_message_buffer.resize(data_message.length);
        }
 
        //uint8_t interface_swap = 0;
        clock_gettime(CLOCK_REALTIME, &time_now);
        RM_logInfo("Traffic Genarator Object: " << number_object << " transmission start now: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns");
        
        while(number_packet < current_settings_ptr->number_packets)
        {
            if (current_mode != mode_global)
            {
                RM_logInfo("Traffic Generator change mode to: " << std::to_string(mode_global));

                current_mode = mode_global;
                auto iterator = service_settings_struct.find(current_mode);
                if (iterator != service_settings_struct.end())
                {
                    current_settings_ptr = &iterator->second;
                }
                else
                {
                    RM_logInfo("Traffic Generator mode_global points to unknown setting: " << std::to_string(current_mode));
                    break;
                }

                const auto& settings = *current_settings_ptr;
                RM_logInfo("########");
                RM_logInfo("# Sending Mode      : " << std::to_string(iterator->first));
                RM_logInfo("# Sending Thread Mode, Thread ID " << std::to_string(thread_id) << " Destination IP : " << settings.ip_address << "  " << settings.port);
                RM_logInfo("# Service ID        : " << settings.service_id);
                RM_logInfo("# Deadline          : " << settings.deadline << " ms");
                RM_logInfo("# Object size       : " << settings.object_size << " KB");
                RM_logInfo("# Number of packets : " << settings.number_packets);
                RM_logInfo("# Inter packet gap  : " << settings.inter_packet_gap.count());
                RM_logInfo("# Slot offset       : " << settings.slot_offset);
                RM_logInfo("# Slot length       : " << settings.slot_length);
                RM_logInfo("# Estimated object transmission time : " << settings.estimated_transmission_time_ms << " ms");
                RM_logInfo("########");

                // Rebuild the endpoint dynamically
                auto results = resolver.resolve(
                    boost::asio::ip::udp::v4(),
                    settings.ip_address,
                    std::to_string(settings.port)
                );

                traffic_endpoint_target_new = *results.begin();                
                if (global_update_period)
                {
                    local_timestamp = global_period_timestamp;
                    local_timepoint = std::chrono::steady_clock::time_point{
                    std::chrono::seconds(global_period_timestamp.tv_sec) + std::chrono::nanoseconds(global_period_timestamp.tv_nsec)
                    };
                    global_update_period = false;
                }
                
                RM_logInfo("Traffic Generator Updated endpoint to " << traffic_endpoint_target_new.address().to_string() << ":" << traffic_endpoint_target_new.port())
                RM_logInfo("Traffic Generator Updated period   to " << local_timestamp.tv_sec << " s " << local_timestamp.tv_nsec << " ns")
            }

            // Wait for permission to send
            std::unique_lock<std::mutex> lock(global_mutex);
            generator_conditioning_variable.wait(lock, [&] {
                return (traffic_generator_control == THREAD_TRANSMISSION) || 
                        traffic_generator_control == THREAD_TRANSMISSION_FINISH_OBJECT;
            });
            lock.unlock();

            clock_gettime(CLOCK_REALTIME, &time_send);
            data_message.set_timestamp(time_send);            

            // Sendout
            data_message.dataToNet(data_message_buffer.data());
            traffic_socket.send_to(boost::asio::buffer(data_message_buffer.data(), data_message.length), traffic_endpoint_target_new);


            ++number_packet;
            payload_size = static_cast<size_t>(std::min<uint64_t>(remaining_bytes, demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH));
            data_message.set_payload_size(payload_size);
            data_message.set_packet_number(number_packet);

            // Ensure data_message_buffer is large enough
            if (data_message.length > data_message_buffer.size())
            {
                data_message_buffer.resize(data_message.length);
            }
            // Update remaining bytes
            if (remaining_bytes > payload_size)
            {
                remaining_bytes -= payload_size;
            }
            else
            {
                remaining_bytes = 0;
            }


            if (traffic_pattern.info_flag)
            {
                clock_gettime(CLOCK_REALTIME, &time_now);
                RM_logInfo("Mode " << std::to_string(current_mode) << " Fragment " << number_packet << " from Object: " << number_object << " start now: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns");
            }

            if (stop_thread)
            {
                RM_logInfo("Traffic Generator Thread stopped during transmitting object: " << number_object << " fragment number: " << number_packet);
                break;
            }
            // Pace
            precise_wait_us(traffic_pattern.inter_packet_gap.count());
        }   
        
        if (traffic_pattern.auto_traffic_termination > 0 && number_object >= traffic_pattern.auto_traffic_termination)
        {
            break;
        }      

        number_object++;

        if (stop_thread)
        {
            break;
        }

        struct timespec target_time = local_timestamp;
        target_time.tv_nsec += current_settings_ptr->deadline * 1000000L;
        if (target_time.tv_nsec >= 1000000000L) 
        {
            target_time.tv_sec += target_time.tv_nsec / 1000000000L;
            target_time.tv_nsec %= 1000000000L;
        }
        if (traffic_pattern.info_flag)
        {
            RM_logInfo("target time before waiting : " << target_time.tv_sec << " s, " << target_time.tv_nsec << " ns");
            clock_gettime(CLOCK_REALTIME, &time_now);
            RM_logInfo("current_time before waiting: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns");
        }

        while (true)
        {
            if (traffic_generator_control == THREAD_TRANSMISSION_FINISH_OBJECT)
            {
                RM_logInfo("Traffic Generator Thread interrupted by change");
                break;
            }

            clock_gettime(CLOCK_REALTIME, &time_now);

            if ((time_now.tv_sec > target_time.tv_sec) ||
                (time_now.tv_sec == target_time.tv_sec && time_now.tv_nsec >= target_time.tv_nsec))
            {
                break;
            }
        }

        if (traffic_pattern.info_flag)
        {
            clock_gettime(CLOCK_REALTIME, &time_now);
            RM_logInfo("current_time after waiting : " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns");
        }

        local_timestamp = target_time;

    } // while(true)

    traffic_socket.close();
    RM_logInfo("Traffic Generator Closing of sending thread: " << thread_id)
}


/*
*
*/
void TrafficGenerator::thread_type_select ()
{
    RM_logInfo("Traffic Generator Thread is starting ..." << "\n")
    if (experiment_parameter_struct.synchronous_start_mode == true)
    {
        RM_logInfo("Sending Thread " << thread_id << " OBJECT_BURST_DYNAMIC_CHANGE" << " SYNCHRONOUS\n")
        send_objects_dynamic_change();
    }
    else if (experiment_parameter_struct.synchronous_start_mode == false)
    {
        RM_logInfo("Sending Thread " << thread_id << " OBJECT_BURST_DYNAMIC_CHANGE" << " ASYNCHRONOUS\n")
        send_objects_dynamic_change_asynchron();
    }  
}


/*
*
*/
void TrafficGenerator::join()
{
    traffic_generator.join();

}
