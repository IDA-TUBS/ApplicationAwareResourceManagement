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


#ifndef DEMONSTRATOR_CONFIG_h
#define DEMONSTRATOR_CONFIG_h


#include <string>

#include <rscmng/attributes/uuid.hpp>

namespace demonstrator {

    const std::string CLIENT_SG1_IP      = "192.168.1.11";
    const std::string CLIENT_SG2_IP      = "192.168.1.12";
    const std::string CLIENT_SG3_IP      = "192.168.1.13";
    const std::string CLIENT_SG4_IP      = "192.168.1.14";

    const std::string CLIENT_SW1_IP      = "192.168.1.21";
    const std::string CLIENT_SW1_IP_ALT  = "192.168.1.31";
    const std::string CLIENT_SW1_IP_RM   = "192.168.1.21";

    const std::string CLIENT_SW2_IP      = "192.168.1.22";
    const std::string CLIENT_SW2_IP_ALT  = "192.168.1.32";
    const std::string CLIENT_SW2_IP_RM   = "192.168.1.62";

    const std::string CLIENT_SW3_IP      = "192.168.1.23";
    const std::string CLIENT_SW3_IP_ALT  = "192.168.1.33";
    const std::string CLIENT_SW3_IP_RM   = "192.168.1.63";

    const std::string CLIENT_SW4_IP      = "192.168.1.24";
    const std::string CLIENT_SW4_IP_ALT  = "192.168.1.34";
    const std::string CLIENT_SW4_IP_RM   = "192.168.1.34";

    const std::string CLIENT_SF1_IP      = "127.0.0.1";

    const std::string RM_DEFAULT         = "192.168.1.230";
    const std::string RM_ALT_1           = "192.168.1.231";
    const std::string RM_ALT_2           = "192.168.1.232";

    const std::string CLIENT_GW1_IP      = "192.168.1.90";
    const std::string CLIENT_GW2_IP      = "192.168.1.91";

    const long PORT_CTRL_LAYER     = 5000;
    const long PORT_CTRL_LAYER_ACK = 10000;

    const int RM_PRIORITY = 0;
    const int RM_ID = 99;
    const unsigned char rm_guid[] = {0x01,0xDA,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    const rscmng::UUID_t RM_UUID(rm_guid);

    const int NUMBER_PRIORITY_LEVEL = 4;

    const int MAX_PROTOCOL_MESSAGE_LENGTH = 1458;
    
};

namespace RMConfig {

    /* demonstrator configuration */
    const int RM_PRIORITY = demonstrator::RM_PRIORITY;

    // Control Messages
    // Max byte size for payload parameter stream
    const uint16_t max_payload = 1024;    
    
}

namespace rscmng 
{
    #ifndef MAX_MSG_LENGTH
    #define MAX_MSG_LENGTH
    enum { max_length = 1472 };
    #endif
    
    typedef uint64_t serviceID_t;    
};

#endif
