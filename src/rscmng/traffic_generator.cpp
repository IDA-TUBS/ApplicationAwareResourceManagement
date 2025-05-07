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
    TrafficSourceType traffic_type,
    traffic_generator_parameter traffic_pattern
):
    service_settings_struct(service_settings),
    client_configuration_struct(client_configuration),
    traffic_source_type(traffic_type),
    traffic_socket(traffic_context),
    traffic_pattern(traffic_pattern),
    traffic_endpoint_local(udp::endpoint(boost::asio::ip::address::from_string(client_configuration.rm_control_local_ip[0]), 10000)),
    traffic_endpoint_target(udp::endpoint(boost::asio::ip::address::from_string(service_settings[0].ip_address), service_settings[0].port)),
    traffic_endpoint_target_alt(udp::endpoint(boost::asio::ip::address::from_string(service_settings[1].ip_address), service_settings[1].port))
    
{
    traffic_socket.open(traffic_endpoint_local.protocol());
    traffic_socket.set_option(boost::asio::socket_base::reuse_address(true));
    traffic_socket.set_option(boost::asio::socket_base::broadcast(true));
    traffic_socket.bind(traffic_endpoint_local);

    global_mode = 0;

    RM_logInfo("Traffic source constructor done.")
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
    stop_thread = false;
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
            traffic_generator_control = state; // Signal that there is work to do
        }
        generator_conditioning_variable.notify_one();
        RM_logInfo("Traffic generator recieve notification: " << state)

}


/*
*
*/
void TrafficGenerator::notify_generator_mode_change(TrafficSourceControl state, uint32_t mode) 
{
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            //traffic_generator_control = state; // Signal that there is work to do
            //global_mode = mode;
            //open_mc = true;
            if (global_mode == 1)
            {
                RM_logInfo("change destination to IP:" << traffic_endpoint_target.address().to_string() << " port: " << traffic_endpoint_target.port() << "\n")
                global_mode = 0;
            }
            else if (global_mode == 0)
            {            
                RM_logInfo("change destination to IP:" << traffic_endpoint_target_alt.address().to_string() << " port: " << traffic_endpoint_target_alt.port() << "\n")
                global_mode = 1;
            }
            RM_logInfo("Traffic generator recieve mode change: " << state << " global mode " << global_mode)
        }        
        
        generator_conditioning_variable.notify_one();
        
}


/*
*
*/
void TrafficGenerator::preciseSleep(double seconds) 
{
    using namespace std;
    using namespace std::chrono;

    static double estimate = 5e-3;
    static double mean = 5e-3;
    static double m2 = 0;
    static int64_t count = 1;

    while (seconds > estimate) 
    {
        auto start = high_resolution_clock::now();
        this_thread::sleep_for(milliseconds(1));
        auto end = high_resolution_clock::now();

        double observed = (end - start).count() / 1e9;
        seconds -= observed;

        ++count;
        double delta = observed - mean;
        mean += delta / count;
        m2   += delta * (observed - mean);
        double stddev = sqrt(m2 / (count - 1));
        estimate = mean + stddev;
    }

    // spin lock
    auto start = high_resolution_clock::now();
    while ((high_resolution_clock::now() - start).count() / 1e9 < seconds);
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
void TrafficGenerator::sendMessagesObjectsShaped() 
{
    auto wait_for = traffic_pattern.inter_object_gap;
    uint32_t counter = 1;
    uint32_t number_object = 1;

    if (traffic_pattern.object_size_kb != 0)
    {
        traffic_pattern.number_fragments = (traffic_pattern.object_size_kb *1e3 + (demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH) - 1)/(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
    }

    std::chrono::nanoseconds period_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(traffic_pattern.period);
    std::chrono::nanoseconds period_tranmission_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(traffic_pattern.period*traffic_pattern.slack_factor);
    struct timespec time_now = {0,0};

    auto shaping_time = std::chrono::nanoseconds(period_tranmission_ns/traffic_pattern.number_fragments);
    
    // Object level time variables
    auto start = std::chrono::steady_clock::now() - wait_for;
    auto time_stamp = std::chrono::steady_clock::now();
    auto cycle_offset = std::chrono::nanoseconds(0);

    // Fragment level time variables
    auto frag_start = std::chrono::steady_clock::now();
    auto frag_time_stamp = std::chrono::steady_clock::now() + shaping_time;
    auto frag_cycle_offset = std::chrono::nanoseconds(0);

    //BOOST_LOG_TRIVIAL(info) << "Shaping Time: " << shaping_time.count() << " us";
    RM_logInfo("Object Size       : " << traffic_pattern.object_size_kb << " KB" )
    RM_logInfo("Deadline          : " << traffic_pattern.period.count() << " ms" )
    RM_logInfo("Shaping Time      : " << shaping_time.count() << " ns" )
    RM_logInfo("Number of packets : " << traffic_pattern.number_fragments )
    RM_logInfo("Service ID        : " << service_settings_struct[0].service_id)

    // Initialize dummy load
    std::vector<char> dummy_payload(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH, 'A');

    // Buffer for converted dataMessageTimestamp
    std::vector<char> data_message_buffer(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);

    struct timespec time_send = {0,0};

    while (true) 
    {        
        frag_start = std::chrono::steady_clock::now();
        uint32_t object_load = traffic_pattern.object_size_kb;
        uint32_t frag_payload = demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH;

        clock_gettime(CLOCK_REALTIME, &time_now);
        RM_logInfo("Object with number: " << number_object << " start now: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns" )

        for(long loop_iterator = 0; loop_iterator < traffic_pattern.number_fragments; loop_iterator++)
        {
            std::unique_lock<std::mutex> lock(global_mutex);
            auto& process_send_data = (thread_id == 1) ? process_send_data_1 :
                                      (thread_id == 2) ? process_send_data_2 :
                                        process_send_data_3;
            generator_conditioning_variable.wait(lock, [&] { return process_send_data; });
            lock.unlock();

            clock_gettime(CLOCK_REALTIME, &time_send);
            
            DataMessage data_message(
                service_settings_struct[0].service_priority, 
                client_configuration_struct.client_id, 
                service_settings_struct[0].service_id, 
                number_object, 
                loop_iterator + 1, 
                traffic_pattern.number_fragments, 
                dummy_payload.data(),
                dummy_payload.size(), 
                time_send
            );
            data_message.dataToNet(data_message_buffer.data());
            traffic_socket.send_to(boost::asio::buffer(data_message_buffer.data(), data_message.length), traffic_endpoint_target);
            

            if(frag_time_stamp - frag_start > shaping_time)
            {
                frag_cycle_offset = frag_cycle_offset + (time_stamp - start - shaping_time);
            }
            else if(frag_time_stamp - frag_start < shaping_time)
            {
                frag_cycle_offset = frag_cycle_offset + (time_stamp - start - shaping_time);
            }

            frag_start = frag_time_stamp;
            while(frag_time_stamp < (frag_start + shaping_time))
            {
                frag_time_stamp = std::chrono::steady_clock::now();
            }

            // Calculate remaining load
            object_load -= (demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
            if(object_load < frag_payload)
            {
                frag_payload = object_load;
            }   
            counter++;
        }        
        //RM_logInfo("Object with number: " << number_object << " finished")

        if (traffic_pattern.auto_traffic_termination > 0 && number_object >= traffic_pattern.auto_traffic_termination)
        {
            break;
        }

        number_object++;

        // Cycle offset compensation via offset budget
        if(time_stamp - start > wait_for)
        {
            cycle_offset = cycle_offset + (time_stamp - start - wait_for);
        }
        else if(time_stamp - start < wait_for)
        {
            cycle_offset = cycle_offset + (time_stamp - start - wait_for);
        }

        start = time_stamp;
        while(time_stamp < start + wait_for - cycle_offset)
        {
            time_stamp = std::chrono::steady_clock::now();
        }
    }
    traffic_socket.close();
    return;
}


/*
*
*/
void TrafficGenerator::sendMessagesObjectsBurst() 
{
    RM_logInfo("Sending Thread " << thread_id << " ObjectsBurst started")

    auto wait_for = traffic_pattern.period;
    uint32_t counter = 1;
    long number_object = 1;

    if (traffic_pattern.object_size_kb != 0)
    {
        traffic_pattern.number_fragments = (traffic_pattern.object_size_kb *1e3 + (demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH) - 1)/(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
    }

    std::chrono::nanoseconds period_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(traffic_pattern.period);

    // Object level time variables
    auto object_transmission_start = std::chrono::steady_clock::now();
    //auto object_transmission_finish = std::chrono::steady_clock::now();
    struct timespec time_now = {0,0};

    auto current_time = std::chrono::steady_clock::now();
    auto cycle_offset = std::chrono::nanoseconds(0);

    RM_logInfo("Object Size      : " << traffic_pattern.object_size_kb << " KB")
    RM_logInfo("Deadline         : " << traffic_pattern.period.count() << " ms")
    RM_logInfo("Number of pacets : " << traffic_pattern.number_fragments)
    RM_logInfo("Service ID       : " << service_settings_struct[0].service_id)

    // Initialize dummy load
    std::vector<char> dummy_payload(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH, 'A');

    // Buffer for converted dataMessageTimestamp
    std::vector<char> data_message_buffer(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH+50);

    struct timespec time_send = {0,0};
    
    while (true) 
    {        
        object_transmission_start = std::chrono::steady_clock::now();

        clock_gettime(CLOCK_REALTIME, &time_now);
        RM_logInfo("Object with number: " << number_object << " start now: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns")

        uint32_t object_load = traffic_pattern.object_size_kb;
        uint32_t frag_payload = demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH;

        for(long loop_iterator = 0; loop_iterator < traffic_pattern.number_fragments; loop_iterator++)
        {
            std::unique_lock<std::mutex> lock(global_mutex);
            generator_conditioning_variable.wait(
                lock, 
                [&] {return (traffic_generator_control == THREAD_TRANSMISSION) || stop_thread;}
            );
            lock.unlock();       
                    
            clock_gettime(CLOCK_REALTIME, &time_send);

            DataMessage data_message(
                service_settings_struct[0].service_priority, 
                client_configuration_struct.client_id, 
                service_settings_struct[0].service_id, 
                number_object, 
                loop_iterator + 1, 
                traffic_pattern.number_fragments, 
                dummy_payload.data(),
                dummy_payload.size(), 
                time_send
            );
            data_message.dataToNet(data_message_buffer.data());
            traffic_socket.send_to(boost::asio::buffer(data_message_buffer.data(), data_message.length), traffic_endpoint_target);
            
            // Calculate remaining load
            object_load -= (demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
            if(object_load < frag_payload)
            {
                frag_payload = object_load;
            }
            counter++;
        }
        clock_gettime(CLOCK_REALTIME, &time_now);
        //RM_logInfo("Object with number: " << number_object << " finished : " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns") 

        if (traffic_pattern.auto_traffic_termination > 0 && number_object >= traffic_pattern.auto_traffic_termination)
        {
            break;
        }
        
        number_object++;
        //data_message.clear();
        while((object_transmission_start + traffic_pattern.period) > current_time)
        {
            current_time = std::chrono::steady_clock::now();
        }
        clock_gettime(CLOCK_REALTIME, &time_now);
        //RM_logInfo("Object with number: " << number_object << " end turn : " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns")
    }
    
    traffic_socket.close();
}


/*
*
*/
void TrafficGenerator::sendMessagesObjectsBurstShaped() 
{
    RM_logInfo("Sending Thread " << thread_id << " ObjectsBurst started")

    auto wait_for = traffic_pattern.period;
    uint32_t counter = 1;
    long number_object = 1;

    if (traffic_pattern.object_size_kb != 0)
    {
        traffic_pattern.number_fragments = (traffic_pattern.object_size_kb *1e3 + (demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH) - 1)/(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
    }

    std::chrono::nanoseconds period_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(traffic_pattern.period);
    double time_ms = (traffic_pattern.number_fragments * (traffic_pattern.inter_packet_gap.count() / 1e6 +  demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH * 8 / 1e9)) * 1e3;


    // Object level time variables
    auto object_transmission_start = std::chrono::steady_clock::now();
    //auto object_transmission_finish = std::chrono::steady_clock::now();
    struct timespec time_now = {0,0};

    auto current_time = std::chrono::steady_clock::now();
    auto cycle_offset = std::chrono::nanoseconds(0);

    RM_logInfo("Object size       : " << traffic_pattern.object_size_kb << " KB")
    RM_logInfo("Deadline          : " << traffic_pattern.period.count() << " ms")
    RM_logInfo("Number of packets : " << traffic_pattern.number_fragments)
    RM_logInfo("Service ID        : " << service_settings_struct[0].service_id)
    RM_logInfo("Inter packet gap  : " << traffic_pattern.inter_packet_gap.count())
    
    RM_logInfo("Estimated object transmission time : " << time_ms << " ms")

    // Initialize dummy load
    std::vector<char> dummy_payload(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH, 'A');

    // Buffer for converted dataMessageTimestamp
    std::vector<char> data_message_buffer(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH+50);

    struct timespec time_send = {0,0};
    
    while (true) 
    {        
        object_transmission_start = std::chrono::steady_clock::now();

        clock_gettime(CLOCK_REALTIME, &time_now);
        RM_logInfo("Object with number: " << number_object << " start now: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns")

        uint32_t object_load = traffic_pattern.object_size_kb;
        uint32_t frag_payload = demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH;

        for(uint32_t loop_iterator = 0; loop_iterator < traffic_pattern.number_fragments; loop_iterator++)
        {
            std::unique_lock<std::mutex> lock(global_mutex);
            generator_conditioning_variable.wait(
                lock, 
                [&] {return (traffic_generator_control == THREAD_TRANSMISSION) || stop_thread;}
            );
            lock.unlock();       

            clock_gettime(CLOCK_REALTIME, &time_send);

            DataMessage data_message(
                service_settings_struct[0].service_priority, 
                client_configuration_struct.client_id, 
                service_settings_struct[0].service_id, 
                number_object, 
                loop_iterator + 1, 
                traffic_pattern.number_fragments, 
                dummy_payload.data(),
                dummy_payload.size(), 
                time_send
            );
            data_message.dataToNet(data_message_buffer.data());
            traffic_socket.send_to(boost::asio::buffer(data_message_buffer.data(), data_message.length), traffic_endpoint_target);
            
            // Calculate remaining load
            object_load -= (demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
            if(object_load < frag_payload)
            {
                frag_payload = object_load;
            }
            counter++;

            //preciseSleep((traffic_pattern.inter_packet_gap.count() / 1000000)); //25 us
            precise_wait_us((traffic_pattern.inter_packet_gap.count())); //25 us
            

            if (stop_thread) 
            {
                break;
            }
        }
        clock_gettime(CLOCK_REALTIME, &time_now);
        //RM_logInfo("Object with number: " << number_object << " finished : " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns")

        if (traffic_pattern.auto_traffic_termination > 0 && number_object >= traffic_pattern.auto_traffic_termination)
        {
            break;
        }

        number_object++;

        while((object_transmission_start + traffic_pattern.period) > current_time)
        {
            current_time = std::chrono::steady_clock::now();
        }
        clock_gettime(CLOCK_REALTIME, &time_now);
        //RM_logInfo("Object with number: " << number_object << " end turn : " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns")
    }
    
    traffic_socket.close();
}


/*
*
*/
void TrafficGenerator::sendMessagesObjectsBurstShapedIPChange() 
{
    RM_logInfo("Sending Thread " << thread_id << " ObjectsBurstShaped IP started" )

    udp::endpoint current_destination_endpoint = traffic_endpoint_target;

    RM_logInfo("Sending Thread Mode 0, Thread ID " << thread_id << " Destination IP : " << service_settings_struct[0].ip_address << "  " << service_settings_struct[0].port)
    RM_logInfo("Sending Thread Mode 1, Thread ID " << thread_id << " Destination IP : " << service_settings_struct[1].ip_address << "  " << service_settings_struct[1].port)

    auto wait_for = traffic_pattern.period;
    uint32_t counter = 1;
    long number_object = 1;

    if (traffic_pattern.object_size_kb != 0)
    {
        traffic_pattern.number_fragments = (traffic_pattern.object_size_kb *1e3 + (demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH) - 1)/(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
    }

    double time_ms = (traffic_pattern.number_fragments * (traffic_pattern.inter_packet_gap.count() / 1e6 +  demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH * 8 / 1e9)) * 1e3;
    //std::chrono::nanoseconds period_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(traffic_pattern.period);

    // Object level time variables
    auto object_transmission_start = std::chrono::steady_clock::now();
    //auto object_transmission_finish = std::chrono::steady_clock::now();
    struct timespec time_now = {0,0};

    auto current_time = std::chrono::steady_clock::now();
    auto cycle_offset = std::chrono::nanoseconds(0);

    RM_logInfo("Object size       : " << traffic_pattern.object_size_kb << " KB" )
    RM_logInfo("Deadline          : " << traffic_pattern.period.count() << " ms" )
    RM_logInfo("Number of packets : " << traffic_pattern.number_fragments )
    RM_logInfo("Service ID        : " << service_settings_struct[0].service_id)
    RM_logInfo("Inter packet gap  : " << traffic_pattern.inter_packet_gap.count())
    
    RM_logInfo("Estimated object transmission time : " << time_ms << " ms")

    // Initialize dummy load
    std::vector<char> dummy_payload(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH, 'A');

    // Buffer for converted dataMessageTimestamp
    std::vector<char> data_message_buffer(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);

    struct timespec time_send = {0,0};

    int endpoint_number = 0;
    
    while (true) 
    {        
        object_transmission_start = std::chrono::steady_clock::now();

        uint32_t object_load = traffic_pattern.object_size_kb;
        uint32_t frag_payload = demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH;

        if (traffic_pattern.info_flag)
        {
            RM_logInfo("Object with number: " << number_object << " start now: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns" )
        }

        for(uint32_t loop_iterator = 0; loop_iterator < traffic_pattern.number_fragments; loop_iterator++)
        {

            std::unique_lock<std::mutex> lock(global_mutex);
            generator_conditioning_variable.wait(
                lock, 
                [&] {return (traffic_generator_control == THREAD_TRANSMISSION) || (traffic_generator_control == THREAD_RECONFIGURE) || stop_thread;}
            );

            if (traffic_pattern.info_flag)
            {
                clock_gettime(CLOCK_REALTIME, &time_now);
                RM_logInfo("Fragment: " << loop_iterator << " from Object: "<< number_object << " start now: " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns" )
            }
            //if (traffic_generator_control == THREAD_RECONFIGUR &&  )
            //{
            //    RM_logInfo("change destination to IP")
            //}

            /*if (traffic_generator_control == THREAD_RECONFIGURE)
            {
                //RM_logInfo("ip change reconfigure check conditions")
                if (open_mc == true && global_mode == 1)
                {
                    //current_destination_endpoint = traffic_endpoint_target;
                    RM_logInfo("change destination to IP:" << traffic_endpoint_target.address().to_string() << " port: " << traffic_endpoint_target.port() << "\n")
                    open_mc = false;
                    endpoint_number = 0;
                }
                else if (open_mc == true && global_mode == 0)
                {
                    //current_destination_endpoint = traffic_endpoint_target_alt;
                    RM_logInfo("change destination to IP:" << traffic_endpoint_target_alt.address().to_string() << " port: " << traffic_endpoint_target_alt.port() << "\n")
                    open_mc = false;
                    endpoint_number = 1;
                }
                
                traffic_generator_control = THREAD_TRANSMISSION;
                RM_logInfo("Thread reconfigured during tranmitting object: " << number_object << " fragment number: " << loop_iterator)
            }*/
            lock.unlock();

            clock_gettime(CLOCK_REALTIME, &time_send);

            DataMessage data_message(
                service_settings_struct[0].service_priority, 
                client_configuration_struct.client_id, 
                service_settings_struct[0].service_id, 
                number_object, 
                loop_iterator + 1, 
                traffic_pattern.number_fragments, 
                dummy_payload.data(),
                dummy_payload.size(), 
                time_send
            );
            data_message.dataToNet(data_message_buffer.data());

            if (global_mode == 0)
            {
                traffic_socket.send_to(boost::asio::buffer(data_message_buffer.data(), data_message.length), traffic_endpoint_target);
            }
            else if (global_mode == 1) 
            {
                traffic_socket.send_to(boost::asio::buffer(data_message_buffer.data(), data_message.length), traffic_endpoint_target_alt);
            }
            
            // Calculate remaining load
            object_load -= (demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
            if(object_load < frag_payload)
            {
                frag_payload = object_load;
            }
            counter++;

            precise_wait_us((traffic_pattern.inter_packet_gap.count())); //25 us

            if (stop_thread) 
            {
                RM_logInfo("Thread stopped during tranmitting object: " << number_object << " fragment number: " << loop_iterator)
                break;
            }
        }
        if (traffic_pattern.info_flag)
        {
            clock_gettime(CLOCK_REALTIME, &time_now);
            RM_logInfo("Object with number: " << number_object << " finished : " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns")
        }
        
        if (traffic_pattern.auto_traffic_termination > 0 && number_object >= traffic_pattern.auto_traffic_termination)
        {
            break;
        }

        number_object++;

        while((object_transmission_start + traffic_pattern.period) > current_time)
        {
            current_time = std::chrono::steady_clock::now();
        }

        if (traffic_pattern.info_flag)
        {
            clock_gettime(CLOCK_REALTIME, &time_now);
            RM_logInfo("Object with number: " << number_object << " end turn : " << time_now.tv_sec << " s, " << time_now.tv_nsec << " ns")
        }

        if (stop_thread) 
        {
            break;
        }
    }
    
    traffic_socket.close();
}


/*
*
*/
void TrafficGenerator::sendMessagesPerSecond() 
{
    RM_logInfo("Sending Thread " << thread_id << " started")

    if (traffic_pattern.object_size_kb != 0)
    {
        traffic_pattern.number_fragments = (traffic_pattern.object_size_kb *1e3 + (demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH) - 1)/(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
    }

    auto wait_for = traffic_pattern.inter_object_gap;
    uint32_t number_object = 1;

    long load_bit_per_second = traffic_pattern.load_mbit_per_second*1e6;

    if (load_bit_per_second <= 0)
    {
        RM_logInfo("Sending Thread " << thread_id << " stopped")
        return;
    }
    
    uint32_t number_frames_per_second = (load_bit_per_second)/((demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH)*8);
    auto shaping_time = std::chrono::nanoseconds(traffic_pattern.period / number_frames_per_second); 

    auto frag_start = std::chrono::steady_clock::now();
    auto frag_time_stamp = std::chrono::steady_clock::now() + shaping_time;
    auto frag_current_time_stamp = std::chrono::steady_clock::now();

    //BOOST_LOG_TRIVIAL(info) << "Shaping Time: " << shaping_time.count() << " us";
    RM_logInfo("Load bits/s: " << load_bit_per_second)
    RM_logInfo("Number of required frames: " << number_frames_per_second)
    RM_logInfo("Shaping Time in ns: " << shaping_time.count())
    
    // Initialize dummy load
    std::vector<char> dummy_payload(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH, 'A');

    // Buffer for converted dataMessageTimestamp
    std::vector<char> data_message_buffer(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);

    struct timespec time_send = {0,0};
    
    long loop_iterator = 0;

    while (true) 
    {        
        frag_start = std::chrono::steady_clock::now();        

        std::unique_lock<std::mutex> lock(global_mutex);
        generator_conditioning_variable.wait(
            lock, 
            [&] {return (traffic_generator_control == THREAD_TRANSMISSION) || (traffic_generator_control == THREAD_RECONFIGURE) || stop_thread;}
        );     
        lock.unlock();   
           
        clock_gettime(CLOCK_REALTIME, &time_send);

        DataMessage data_message(
            service_settings_struct[0].service_priority, 
            client_configuration_struct.client_id, 
            service_settings_struct[0].service_id, 
            number_object, 
            loop_iterator + 1, 
            traffic_pattern.number_fragments, 
            dummy_payload.data(),
            dummy_payload.size(), 
            time_send
        );
        data_message.dataToNet(data_message_buffer.data());
        traffic_socket.send_to(boost::asio::buffer(data_message_buffer.data(), data_message.length), traffic_endpoint_target);
        
        number_object++;   
        loop_iterator++;

        frag_time_stamp = frag_start + shaping_time; 
        while(frag_current_time_stamp < frag_time_stamp)
        {
            frag_current_time_stamp = std::chrono::steady_clock::now();
        }     
    }
    traffic_socket.close();
}


/*
*
*/
void TrafficGenerator::thread_type_select ()
{
    RM_logInfo("Traffic Generator Thread is starting ..." << "\n")
    switch(traffic_source_type)
    {
        case OBJECT_SHAPED:
           RM_logInfo("OBJECT_SHAPED" << "\n")
            sendMessagesObjectsShaped();
            break;

        case OBJECT_BURST:
            RM_logInfo("OBJECT_BURST" << "\n")
            sendMessagesObjectsBurst();
            break;

        case OBJECT_BURST_SHAPED:
            RM_logInfo("OBJECT_BURST_SHAPED" << "\n")
            sendMessagesObjectsBurstShaped();
            break;

        case OBJECT_BURST_SHAPED_IP:
            RM_logInfo("Sending Thread " << thread_id << " OBJECT_BURST_SHAPED_IP" << "\n")
            sendMessagesObjectsBurstShapedIPChange();
            break;

        case NORMAL_BURST:
            RM_logInfo("NORMAL_BURST" << "\n")
            sendMessagesPerSecond();
            break;
            
        default:
            RM_logInfo("default" << "\n")
            break;

    };        
}


/*
*
*/
void TrafficGenerator::join()
{
    traffic_generator.join();

}