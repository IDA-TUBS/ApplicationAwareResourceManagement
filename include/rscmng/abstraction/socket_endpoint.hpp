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


#ifndef SOCKET_ENDPOINT_h
#define SOCKET_ENDPOINT_h


#include <string>
#include <chrono>


/**
 * @brief Describes the parameters of an boost asio endpoint. Used for passing endpoints to functions.
 * 
 */
struct socket_endpoint
{
    std::string ip_addr;
    int port;

    socket_endpoint(){};

    socket_endpoint(    
        std::string ip, 
        int p
    ):
        ip_addr(ip),
        port(p)
    {};
};

/**
 * @brief Describes the parameteres of an resource management (rm) endpoint. Every rm endpoint is defined by its own endpoint(tx) and a destination endpoint (rx).
 * The destination endpoint describes the (rm) endpoint in an upper hierarchy level. 
 * Used to define the rm endpoints for the network clients (gateways)
 * 
 */
struct endpoint
{
    std::string rx_ip;
    int rx_port;
    std::string tx_ip;
    int tx_port;

    endpoint(){};

    endpoint(
        std::string r_ip,
        int r_port,
        std::string t_ip,
        int t_port
    ):
        rx_ip(r_ip),
        rx_port(r_port),
        tx_ip(t_ip),
        tx_port(t_port)
    {};
};


/**
 * @brief Describes the network environment for a publishing node. 
 * The local_address endpoint is the nodes own endpoint
 * The target_address enpoint is the destination of the traffic published by the node
 *
 */
struct network_environment_wired
{
    struct socket_endpoint local_address;
    struct socket_endpoint target_address;
    struct socket_endpoint target_alt_address;

    network_environment_wired(){};

    network_environment_wired(
        std::string local_ip_address,
        int local_port,
        std::string target_ip_address,
        int target_port
    ):
        local_address(local_ip_address, local_port),
        target_address(target_ip_address, target_port),
        target_alt_address(target_ip_address, target_port)
    {};

    network_environment_wired(
        std::string local_ip_address,
        int local_port,
        std::string target_ip_address,
        int target_port,
        std::string target_alt_ip_address,
        int target_alt_port
    ):
        local_address(local_ip_address, local_port),
        target_address(target_ip_address, target_port),
        target_alt_address(target_alt_ip_address, target_alt_port)
    {};
};

#endif