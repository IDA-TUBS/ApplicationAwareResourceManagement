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


#ifndef NET_LAYER_RM_h
#define NET_LAYER_RM_h


#include <thread>
#include <queue>
#include <array>
#include <vector>
#include <random>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include <rscmng/utils/log.hpp>
#include <rscmng/attributes/demonstrator_config.hpp>
#include <rscmng/attributes/uuid.hpp>
#include <rscmng/utils/config_reader.hpp>
#include <rscmng/utils/timer_events.hpp>
#include <rscmng/messages.hpp>
#include <rscmng/rm_communication.hpp>
#include <rscmng/abstraction/socket_endpoint.hpp>
#include <rscmng/data_sharing/safe_map.hpp>
#include <rscmng/data_sharing/safe_queue.hpp>
#include <rscmng/protocols/rm_wired_payload.hpp>


using boost::asio::ip::udp;


namespace rscmng {

void distribution_handler(rscmng::wired::RMMessage message);

struct network_link_resources
{
    uint32_t segment_id;
    uint32_t link_id;
    std::vector<uint32_t> client_list;
    uint32_t aggregate_bandwidth;
};

struct network_device_configs
{
    struct rscmng::config::unit_settings local_rm_configuration;
    std::map<uint32_t, std::map<uint32_t, struct rscmng::config::service_settings>> service_configuration_map;
    std::map<uint32_t, struct rscmng::config::unit_settings> endnode_configuration_map;
    uint32_t scenario_config;
};

template<class T>
class NetLayerRM
{

    /**
     * @brief Restrict allowed template type
     * 
     */
    static_assert(  std::is_same<float, T>::value ||
                    std::is_same<double, T>::value ||
                    std::is_same<uint32_t, T>::value ||
                    std::is_same<std::chrono::microseconds, T>::value ||
                    std::is_same<std::chrono::milliseconds, T>::value ||
                    std::is_same<std::chrono::nanoseconds, T>::value,
                    "Supplied template type not specified for class AppLinkList");

    public:

    /**
     * @brief Enumeration for tuple indicies of the RM cache entry
     * 
     */
    enum PubSubEntry
    { 
        NW_ENDPOINT = 0,
        TOPIC,
        PARTICIPANT,
        QOS,
        LINKED
    };


    /**
     * @brief Construct a new App Layer R M object
     * 
     * @param id ID of the created resource manager
     * @param priority_levels Number of priority levels for the priority based message queues
     * @param source the socket endpoint 
     * for the resource manager (aRM level)
     * param nRM_in_queue Queue for the nRM messages
     * 
     * +
     * +@param nRM_out_queue Queue for the replies of the nRM     
     */
    NetLayerRM( 
        UUID_t id, 
        const uint32_t priority_levels,
        struct rscmng::config::unit_settings local_rm_configuration,
        struct network_device_configs network_configuration,
        struct rscmng::config::experiment_parameter experiment_parameter,
        SafeQueue<rscmng::wired::RMMessage> &nRM_in_queue,
        SafeQueue<rscmng::wired::RMMessage> &nRM_out_queue
    );


    /**
     * @brief receives resource management messages and enqueus them according to their priority in the receive queue (infinite loop)
     * 
     */
    void receive_message();


    /**
     * @brief Dequeue resource management messages - priority based - and trigger the respective handler (according to the message type). 
     * Infinite loop
     * 
     */
    void handle_message();


    /**
     * @brief 
     * 
     */
    void handle_sync_messages(rscmng::wired::RMMessage message);

    /**
     * @brief 
     * 
     */
    void initial_start_handler();

    /**
     * @brief 
     * 
     */
    void start_network_traffic_synchronous();
    
    /**
     * @brief 
     * 
     */
    void start_network_traffic_asynchronous();

    /**
     * @brief 
     * 
     */
    udp::endpoint get_client_address(uint32_t endnode_id);
   
    /**
     * @brief 
     * 
     */
    struct timespec prepare_timestamp(struct timespec time_stamp, std::chrono::milliseconds delay);

    /**
     * @brief 
     * 
     */
    struct timespec get_round_timestamp();

    /**
     * @brief 
     * 
     */
    void experiment_mode_change();

    /**
     * @brief 
     * 
     */
    void experiment_mode_change_synchronous();

    /**
     * @brief 
     * 
     */
    void experiment_mode_change_asynchronous();

    /**
     * @brief 
     * 
     */
    void experiment_mode_change_synchronous_asynchonous_objects();

    /**
     * @brief 
     * 
     */
    uint32_t calculate_service_slot(uint32_t network_mode, serviceID_t service_id);

    /**
     * @brief 
     * 
     */
    void stopping_experiment_synchronous();

    /**
     * @brief 
     * 
     */
    void stopping_experiment_asynchronous();

    /**
     * @brief 
     * 
     */
    void send_synchronous_reconfigure(struct timespec stop_time, struct timespec recofig_time, struct timespec start_time, udp::endpoint target_address, serviceID_t service_id, uint32_t mode);

    /**
     * @brief 
     * 
     */
    void send_asynchronous_reconfigure(udp::endpoint target_address, serviceID_t service_id, uint32_t network_mode);

    /**
     * @brief 
     * 
     */
    void send_synchronous_start(struct timespec target_time, udp::endpoint target_address, serviceID_t service_id);

    /**
     * @brief 
     * 
     */
    void send_synchronous_stop(struct timespec stop_time, struct timespec recofig_time, struct timespec start_time, udp::endpoint target_address, serviceID_t service_id, uint32_t mode);

    /**
     * @brief 
     * 
     */
    void send_asynchronous_stop(udp::endpoint target_address, serviceID_t service_id, uint32_t network_mode);

    /**
     * @brief 
     * 
     */
    std::chrono::milliseconds random_duration(std::chrono::milliseconds min_value, std::chrono::milliseconds max_value);

    struct timespec subtract_timespec(struct timespec end, struct timespec start);



    /**
     * @brief Wrapper for std::thread::join. Contains all threads created by this class.
     * 
     */
    void join();

    
    private:

    /**
     * @brief control socket
     * 
     */
    rscmng::wired::RMCommunication control_channel;

    /**
     * @brief ID of the resource manager. Used for distinction between the network segments
     * 
     */
    UUID_t rm_id;
    
    /**
     * @brief Number of available priority levels
     * 
     */
    const uint32_t prio_levels;

    /**
     * @brief Queueu for incomming messages
     * 
     */
    SafeQueue<rscmng::wired::RMMessage> &nRM_in_queue;
    
    /**
     * @brief Queueu for incomming messages
     * 
     */
    SafeQueue<rscmng::wired::RMMessage> &nRM_out_queue;

    /**
     * @brief Queueu for incomming messages
     * 
     */
    SafeQueue<std::pair<rscmng::wired::RMMessage, udp::endpoint>> nRM_in_handle_queue;

    /**
     * @brief Thread for the receiving messages
     * 
     */
    std::thread recving_thread;

    /**
     * @brief Thread for handling messages
     * 
     */
    std::thread handler_thread;

    /**
     * @brief Timer manager for internal synchronized mode change
     * 
     */
    TimerManager<std::chrono::system_clock> start_timer;

    SharedMap<serviceID_t, rscmng::wired::RMMessage> service_list;

    std::map<uint32_t, struct rscmng::config::unit_settings> rm_client_config_map;

    std::map<uint32_t, std::map<uint32_t, struct rscmng::config::service_settings>> rm_client_service_map;

    struct network_device_configs network_environment_config;

    struct rscmng::config::experiment_parameter experiment_parameter_struct;

    struct timespec rm_active_timestamp_start;
    
    struct timespec rm_active_timestamp_last;

    std::chrono::milliseconds hyperperiod_duration;
    uint32_t number_slots_per_hyperperiod;
    std::chrono::milliseconds slot_duration;

    
};


#include <rscmng/rm/net_layer_rm.tpp>

};

#endif