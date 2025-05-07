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


#include <rscmng/traffic_sink_statistic.hpp>


using namespace rscmng;
using namespace wired; 
using namespace traffic_statistic;
using namespace boost::placeholders;


/*
*
*/
TrafficSink::TrafficSink(
    std::string local_ip_address,
    std::vector<uint32_t> client_local_port,
    std::condition_variable &traffic_generator_notify
):
    traffic_generator_notify(traffic_generator_notify)
{
    if (local_ip_address == "" || client_local_port.empty())
    {
        RM_logInfo("Traffic sink parameter are not valid!");
        //return;
    }
    else
    {
        for (size_t iterator = 0; iterator < client_local_port.size(); ++iterator)
        {
            traffic_generator_list.emplace_back(&TrafficSink::initialize_sink_port, this, local_ip_address, client_local_port[iterator]);
            RM_logInfo("Traffic sink start initialization: " << local_ip_address << " " << client_local_port[iterator]);
        }
    }

}


/*
*
*/
TrafficSink::~TrafficSink()
{

}

/*
*
*/
void TrafficSink::initialize_sink_port(std::string local_ip_address, uint32_t local_port)
{
    boost::asio::io_context traffic_context;
    udp::endpoint traffic_endpoint_local(boost::asio::ip::address::from_string(local_ip_address), local_port);
    udp::socket traffic_socket(traffic_context);

    try 
    {       
        traffic_socket.open(traffic_endpoint_local.protocol());
        traffic_socket.set_option(boost::asio::socket_base::reuse_address(true));
        traffic_socket.set_option(boost::asio::socket_base::broadcast(true));
        traffic_socket.bind(traffic_endpoint_local);       
    } 
    catch (const boost::system::system_error& e) 
    {
        RM_logError("Error in initialize_sink_port: " << e.what());
        return;
    }

    RM_logInfo("Traffic sink start handle message: " << local_ip_address << " " << local_port);

    //traffic_context.run();
    handle_message(std::ref(traffic_socket));
}

/*
*
*/
void TrafficSink::handle_message(udp::socket &traffic_socket)
{
    DataMessage data_message;
    FILE *fp;
    size_t recv_length;
    long received_object_number = 0;
    long received_packet_number = 0;    
    struct timespec time_received = {0,0}; 
    struct timespec time_start = {0,0};    
    std::chrono::system_clock::time_point time_start_2; 
    std::vector<char> data_message_buffer(demonstrator::MAX_PROTOCOL_MESSAGE_LENGTH);
    size_t message_buffer_size = 0;
    udp::endpoint source_address;

    std::string local_endpoint_str = traffic_socket.local_endpoint().address().to_string();
    std::string filename_str = filename_log + "_" + local_endpoint_str + ".log";
    std::string filename_str_log = filename_log + "_" + local_endpoint_str + "_all.log";


    RM_logInfo("Traffic sink Thread " << thread_id << " is ready to receive on interface ip " << local_endpoint_str << " " <<  traffic_socket.local_endpoint().port()) 

    while (true) 
    {
        message_buffer_size = traffic_socket.receive_from(boost::asio::buffer(data_message_buffer, data_message_buffer.size()), source_address);

        //data_message_buffer.size() = message_buffer_size;
        data_message.netToData(data_message_buffer.data(), data_message_buffer.size());
       
        clock_gettime(CLOCK_REALTIME, &time_received);        
            
        received_object_number = data_message.object_number;
        received_packet_number = data_message.fragment_number;
        serviceID_t service_id = data_message.service_id;

        time_start = data_message.timestamp;
        time_start_2 = data_message.time_stamp;

        if (received_packet_number == 1 && LOGGING)
        {
            //std::cout << "Traffic sink received frame from service: " << service_id << " " << source_address.address().to_string() << " from port " << source_address.port() << " on interface ip: " << local_endpoint_str << " on port: " << traffic_socket.local_endpoint().port() << " packet number: " << received_packet_number << " from Object number: " << received_object_number << " Time: " << time_received.tv_sec << " s "  << time_received.tv_nsec << " ns" << std::endl;
            fp = fopen(filename_str_log.c_str(), "a");
            fprintf(fp,"Traffic sink received frame from service: %li %c from port %i on interface ip: %i on port %i with packet number %li from object %li at time %li s %ld ns", service_id, source_address.address().to_string(), source_address.port(),local_endpoint_str, traffic_socket.local_endpoint().port(), received_packet_number, received_object_number, time_received.tv_sec, time_received.tv_nsec);		
            fclose(fp);      
        }
                
        fp = fopen(filename_str.c_str(), "a");
        fprintf(fp,"Frame started    :%s:%li:%li:%li: %ld secs %ld nsecs\n", local_endpoint_str.c_str(), received_object_number,received_packet_number, service_id, time_start.tv_sec, time_start.tv_nsec);		
        //fprintf(fp,"Frame started 2  :%s:%li:%li: %ld secs \n", local_endpoint_str.c_str(), received_object_number, received_packet_number, time_start_2);		
        fprintf(fp,"Frame received   :%s:%li:%li:%li: %ld secs %ld nsecs\n", local_endpoint_str.c_str(), received_object_number,received_packet_number, service_id, time_received.tv_sec, time_received.tv_nsec);						
        fclose(fp);

        data_message.clear();        
    }
}


/*
*
*/
void TrafficSink::join()
{
    for (auto& thread : traffic_generator_list)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}