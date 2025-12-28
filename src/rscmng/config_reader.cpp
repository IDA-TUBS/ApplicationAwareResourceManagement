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


#include <rscmng/utils/config_reader.hpp>

using namespace boost::placeholders;
using namespace rscmng;
using namespace config;


/*
*
*/
ConfigReader::ConfigReader()
{

};


/*
*
*/
ConfigReader::~ConfigReader()
{

};


/*
*
*/
std::map<uint32_t, struct service_settings> ConfigReader::load_service_settings(uint32_t service_id, std::string config_file_path)
{
    boost::property_tree::ptree config_parsed_file;
    std::ifstream file_check(config_file_path);

    if (!file_check.good()) 
    {
        RM_logError("Error: File " << config_file_path << " does not exist.")
        throw std::invalid_argument("file" + config_file_path + " not found");
    }

    boost::property_tree::read_json(config_file_path, config_parsed_file);     
    rm_client_service_map = check_service_settings(config_parsed_file);

    std::map<uint32_t, struct service_settings> rm_client_service_map_item = rm_client_service_map[service_id];
    print_service(rm_client_service_map_item);

    return rm_client_service_map_item;
}


/*
*
*/
struct unit_settings ConfigReader::load_unit_settings(std::string client_name, std::string config_file_path)
{
    boost::property_tree::ptree config_parsed_file;
    std::ifstream file_check(config_file_path);

    if (!file_check.good()) 
    {
        RM_logError("Error: File " << config_file_path << " does not exist.") 
        throw std::invalid_argument("file" + config_file_path + " not found");
    }

    boost::property_tree::read_json(config_file_path, config_parsed_file);     
    rm_client_unit_string_map = check_unit_settings_string(config_parsed_file);

    print_unit(rm_client_unit_string_map[client_name]);

    return rm_client_unit_string_map[client_name];
}


/*
*
*/
std::map<uint32_t, struct unit_settings> ConfigReader::load_rm_client_addresses(std::string config_file_path)
{
    boost::property_tree::ptree config_parsed_file;    
    std::ifstream file_check(config_file_path);

    if (!file_check.good()) 
    {
        RM_logError("Error: File " << config_file_path << " does not exist.") 
        throw std::invalid_argument("file" + config_file_path + " not found");
    }

    boost::property_tree::read_json(config_file_path, config_parsed_file);     

    std::map<uint32_t, struct unit_settings> rm_client_address_map = check_unit_settings(config_parsed_file);

    print_rm_clients(rm_client_address_map);

    return rm_client_address_map;
}


/*
*
*/
std::map<uint32_t, std::map<uint32_t, struct rscmng::config::service_settings>> ConfigReader::load_rm_client_services(std::string config_file_path)
{
    boost::property_tree::ptree config_parsed_file;    
    std::ifstream file_check(config_file_path);

    if (!file_check.good()) 
    {
        RM_logError("Error: File " << config_file_path << " does not exist.") 
        throw std::invalid_argument("file" + config_file_path + " not found");
    }

    boost::property_tree::read_json(config_file_path, config_parsed_file);     

    std::map<uint32_t, std::map<uint32_t, struct service_settings>> rm_client_address_map = check_service_settings(config_parsed_file);

    print_all_services(rm_client_address_map);

    return rm_client_address_map;
}


/*
*
*/
struct experiment_parameter ConfigReader::load_experiment_settings(std::string config_file_path) 
{
    boost::property_tree::ptree config_parsed_file;
    std::ifstream file_check(config_file_path);

    if (!file_check.good()) 
    {
        RM_logError("Error: File " << config_file_path << " does not exist.") 
        throw std::invalid_argument("file" + config_file_path + " not found");
    }

    boost::property_tree::read_json(config_file_path, config_parsed_file);   
    struct experiment_parameter experiment_parameter = check_experiment_settings(config_parsed_file);
    print_experiment_settings(experiment_parameter);


    return experiment_parameter;
}

/*
*
*/
void ConfigReader::print_all_services(std::map<uint32_t, std::map<uint32_t, struct service_settings>> service)
{
    RM_logInfo("#---------- Service Configuration -------------#")

    // Iterate through each SERVICE_ID
    for (const auto& service_entry : service)
    {
        uint32_t service_ID = service_entry.first;
        const std::map<uint32_t, struct service_settings>& mode_map = service_entry.second;

        // Iterate through each mode for this SERVICE_ID
        for (const auto& mode_entry : mode_map)
        {
            uint32_t mode = mode_entry.first;
            const struct service_settings& service = mode_entry.second;

            RM_logInfo("# Service ID                : " << service.service_id)
            RM_logInfo("# Mode                      : " << mode)
            RM_logInfo("# Service target            : " << service.ip_address)
            RM_logInfo("# Service target port       : " << service.port)
            std::ostringstream oss;
            for (const auto &path : service.data_path) 
            {
                oss << path << " ";   
            }
            RM_logInfo("# Service data path         : " << oss.str())
            //RM_logInfo("# Service source type       : " << service.source_type)
            RM_logInfo("# Service object_size       : " << service.object_size << " KB")
            RM_logInfo("# Service deadline          : " << service.deadline << " ms")
            RM_logInfo("# Service priority          : " << service.service_priority)
            RM_logInfo("# Service slot_offset            : " << service.slot_offset << " ms")
            RM_logInfo("#----------------------------------------------#")
        }
    }
}


/*
*
*/
void ConfigReader::print_service(std::map<uint32_t, struct service_settings> service)
{
    RM_logInfo("#---------- Service Configuration -------------#")
    for (const auto& mode_entry : service)
    {
        uint32_t mode = mode_entry.first;
        const struct service_settings& service = mode_entry.second;

        RM_logInfo("# Service ID                : " << service.service_id)
        RM_logInfo("# Mode                      : " << mode)
        RM_logInfo("# Service target            : " << service.ip_address)
        RM_logInfo("# Service target port       : " << service.port)
        std::ostringstream oss;
        for (const auto &path : service.data_path) 
        {
            oss << path << " ";   
        }
        RM_logInfo("# Service data path         : " << oss.str())
        RM_logInfo("# Service object_size       : " << service.object_size << " KB")
        RM_logInfo("# Service deadline          : " << service.deadline << " ms")
        RM_logInfo("# Service priority          : " << service.service_priority)
        RM_logInfo("# Service slot_offset       : " << service.slot_offset << " ms")
        RM_logInfo("# Service slot_length       : " << service.slot_length << " ms")
        RM_logInfo("# Service inter packet gap  : " << service.inter_packet_gap.count() << " us")
        RM_logInfo("# Service inter object gap  : " << service.inter_object_gap.count() << " us")
        RM_logInfo("#----------------------------------------------#")
    }
    
}


/*
*
*/
void ConfigReader::print_unit(struct unit_settings unit)
{
    RM_logInfo("#---------- Unit Configuration ----------------#")
    RM_logInfo("# Host name                 : " << unit.host_name)
    RM_logInfo("# RM Client ID              : " << std::hex << unit.client_id)

    std::ostringstream oss;
    for (const auto &ip : unit.rm_control_local_ip) 
    {
         oss << ip << " ";   
    }
    RM_logInfo("# RM Client local IP        : " << oss.str())
    RM_logInfo("# RM Client local port      : " << unit.rm_control_local_port)

    std::ostringstream oss3;
    for (const auto &ip : unit.rm_control_rm_ip) 
    {
         oss3 << ip << " ";   
    }
    RM_logInfo("# RM Client RM IP           : " << oss3.str())
    RM_logInfo("# RM Client RM port         : " << unit.rm_control_rm_port)

    std::ostringstream oss4;
    for (const auto &ip : unit.service_local_ip) 
    {
         oss4 << ip << " ";   
    }
    RM_logInfo("# Service local IP          : " << oss4.str())
    std::ostringstream oss5;
    for (const auto &port : unit.service_local_port) 
    {
         oss5 << port << " ";   
    }
    RM_logInfo("# Service local port        : " << oss5.str())

    RM_logInfo("# RM Client priority        : " << unit.client_priority)
    RM_logInfo("#----------------------------------------------#")
}


/*
*
*/
void ConfigReader::print_rm_clients(std::map<uint32_t, struct unit_settings> rm_client_adress_map)
{
    RM_logInfo("#---------- RM Client Adresses ----------------#")
    for (const auto& entry : rm_client_adress_map) 
    {
        uint32_t client_ID = entry.first;                     // CLIENT_ID (key)
        struct unit_settings endnode_parameters = entry.second;  // Tuple (value)

        RM_logInfo("# RM Client ID              : " << client_ID)
        std::ostringstream oss;
        for (const auto &ip : endnode_parameters.rm_control_local_ip) 
        {
            oss << ip << " ";   
        }
        RM_logInfo("# RM Client local IP        : " << oss.str())
        RM_logInfo("# RM Client local port      : " << endnode_parameters.rm_control_local_port)
        oss.clear();
    }
    RM_logInfo("#----------------------------------------------#")
}


/*
*
*/
void ConfigReader::print_experiment_settings(struct experiment_parameter& experiment_settings) 
{
    RM_logInfo("#---------- Experiment Configuration -------------#");
    RM_logInfo("# Experiment Number          : " << experiment_settings.experiment_number);
    RM_logInfo("# Client Init time           : " << experiment_settings.client_init_time.count() << " ms");
    RM_logInfo("# Experiment begin slot_offset    : " << experiment_settings.experiment_begin_offset.count() << " ms");
    RM_logInfo("# Experiment Iterations      : " << experiment_settings.experiment_iterations);
    //RM_logInfo("# Synchronous Mode           : " << (experiment_settings.synchronous_mode ? "True" : "False"));
    RM_logInfo("# Mode Distribution Time     : " << experiment_settings.mc_distribution_phase_duration.count() << " ms");
    RM_logInfo("# Stop Offset                : " << experiment_settings.mc_client_stop_offset.count() << " ms");
    RM_logInfo("# Reconfiguration Offset     : " << experiment_settings.mc_client_reconfig_offset.count() << " ms");
    RM_logInfo("# Start Offset               : " << experiment_settings.mc_client_start_offset.count() << " ms");
    RM_logInfo("# Inter-MC Gap Min           : " << experiment_settings.inter_mc_gap_min.count() << " ms");
    RM_logInfo("# Inter-MC Gap Max           : " << experiment_settings.inter_mc_gap_max.count() << " ms");
    RM_logInfo("# Hyperperiod Duration       : " << experiment_settings.hyperperiod_duration.count() << " ms");
    //RM_logInfo("# Hyperperiod Slots          : " << experiment_settings.hyperperiod_slots << " ms");
    RM_logInfo("# Startup mode               : " << experiment_settings.startup_mode);
    
    RM_logInfo("# Startup mode Map          :");
    for (const auto& outer_entry : experiment_settings.startup_mode_map) 
    {
        uint32_t mode = outer_entry.first;
        RM_logInfo("#  Mode " << mode << ":");
        for (const auto& inner_entry : outer_entry.second) 
        {
            RM_logInfo("    - ID: " << inner_entry.first << ", Value: " << inner_entry.second);
        }
    }
    std::ostringstream oss;
    for (const auto& order : experiment_settings.reconfiguration_order) 
    {
        oss << order << " ";
    }
    RM_logInfo("# Reconfiguration Order      : " << oss.str());

    RM_logInfo("# Reconfiguration Map        :");
    for (const auto& outer_entry : experiment_settings.reconfiguration_map) 
    {
        uint32_t mode = outer_entry.first;
        RM_logInfo("#  Mode " << mode << ":");
        for (const auto& inner_entry : outer_entry.second) 
        {
            RM_logInfo("    - ID: " << inner_entry.first << ", Value: " << inner_entry.second);
        }
    }
    RM_logInfo("#----------------------------------------------#");
}

/*------------------------------------- Private -----------------------------------------*/
/*
*
*/
std::map<uint32_t, std::map<uint32_t, struct service_settings>> ConfigReader::check_service_settings(boost::property_tree::ptree config_parsed_file)
{
    std::map<uint32_t, std::map<uint32_t, struct service_settings>> rm_client_service_map;
   
    for (const auto& service_tree : config_parsed_file.get_child(SERVICE_SETTINGS)) 
    {
        std::string service_ID_str = service_tree.first;
        uint32_t service_ID = std::stoi(service_ID_str);
        
        for (const auto& mode_tree : service_tree.second) 
        {
            std::string mode_key = mode_tree.first;

            boost::property_tree::ptree service_instance_tree = mode_tree.second;

            // Parse the service settings for this mode
            struct service_settings service_parameters = parse_service_settings(service_instance_tree);

            uint32_t mode = std::stoi(mode_key); // Convert the mode key to integer
            rm_client_service_map[service_ID][mode] = service_parameters;
        }
    }

    return rm_client_service_map;
}


/*
 * 
 */
struct service_settings ConfigReader::parse_service_settings(boost::property_tree::ptree service_tree)
{
    struct service_settings service_settings_struct;

    service_settings_struct.service_id       = service_tree.get<serviceID_t>(SERVICE_ID);
    service_settings_struct.ip_address       = service_tree.get<std::string>(SERVICE_IP);
    service_settings_struct.port             = service_tree.get<uint32_t>(SERVICE_PORT);
    for (const auto &path : service_tree.get_child(DATA_PATH)) 
    {
        service_settings_struct.data_path.push_back(path.second.get_value<uint32_t>());
    }
    //service_settings_struct.source_type      = service_tree.get<std::string>(SOURCE_TYPE);
    service_settings_struct.object_size      = service_tree.get<uint32_t>(OBJECTSIZE);
    service_settings_struct.deadline         = service_tree.get<uint32_t>(DEADLINE);
    service_settings_struct.service_priority = service_tree.get<uint32_t>(PRIORITY);
    service_settings_struct.slot_offset      = service_tree.get<uint32_t>(SLOT_OFFSET);
    service_settings_struct.slot_length      = service_tree.get<uint32_t>(SLOT_LENGTH);
    service_settings_struct.inter_packet_gap = std::chrono::microseconds(service_tree.get<uint32_t>(INTER_PACKET_GAP));
    service_settings_struct.inter_object_gap = std::chrono::microseconds(service_tree.get<uint32_t>(INTER_OBJECT_GAP));
    
    return service_settings_struct;
}


/*
*
*/
std::map<uint32_t, struct unit_settings> ConfigReader::check_unit_settings(boost::property_tree::ptree config_parsed_file)
{
    std::map<uint32_t, struct unit_settings> rm_client_unit_map;

    for (const auto& unit_tree : config_parsed_file.get_child(UNIT_SETTINGS)) 
    {
        std::string name_ID = unit_tree.first; 
        boost::property_tree::ptree unit_instance_tree = unit_tree.second;

        uint32_t client_ID = unit_instance_tree.get<uint32_t>(CLIENT_ID);

        struct unit_settings unit_parameters = parse_unit_settings(unit_instance_tree);
        unit_parameters.host_name = name_ID;

        rm_client_unit_map[client_ID] = unit_parameters;
    }

    return rm_client_unit_map;
}


/*
*
*/
std::map<std::string, struct unit_settings> ConfigReader::check_unit_settings_string(boost::property_tree::ptree config_parsed_file)
{
    std::map<std::string, struct unit_settings> rm_client_unit_map;

    for (const auto& unit_tree : config_parsed_file.get_child(UNIT_SETTINGS)) 
    {
        std::string name_ID = unit_tree.first; 
        boost::property_tree::ptree unit_instance_tree = unit_tree.second;

        uint32_t client_ID = unit_instance_tree.get<uint32_t>(CLIENT_ID); 

        struct unit_settings unit_parameters = parse_unit_settings(unit_instance_tree);
        unit_parameters.host_name = name_ID;

        rm_client_unit_map[name_ID] = unit_parameters;
    }

    return rm_client_unit_map;
}


/**
 * 
 */
struct unit_settings ConfigReader::parse_unit_settings(boost::property_tree::ptree unit_tree)
{
    struct unit_settings unit_settings_struct;
        
    unit_settings_struct.client_id = unit_tree.get<uint32_t>(CLIENT_ID);

    // Extract the array of IPs
    //
    for (const auto &ip : unit_tree.get_child(RM_CONTROL_LOCAL_IP)) 
    {
        unit_settings_struct.rm_control_local_ip.push_back(ip.second.get_value<std::string>());
    }            
    unit_settings_struct.rm_control_local_port = unit_tree.get<uint32_t>(RM_CONTROL_LOCAL_PORT);
    //
    for (const auto &ip : unit_tree.get_child(RM_CONTROL_RM_IP)) 
    {
        unit_settings_struct.rm_control_rm_ip.push_back(ip.second.get_value<std::string>());
    }
    unit_settings_struct.rm_control_rm_port = unit_tree.get<uint32_t>(RM_CONTROL_RM_PORT);
    //
    for (const auto &ip : unit_tree.get_child(SERVICE_LOCAL_IP)) 
    {
        unit_settings_struct.service_local_ip.push_back(ip.second.get_value<std::string>());
    }
    for (const auto &port : unit_tree.get_child(SERVICE_LOCAL_PORT)) 
    {
        unit_settings_struct.service_local_port.push_back(port.second.get_value<uint32_t>());
    }
    //
    unit_settings_struct.client_priority = unit_tree.get<uint32_t>(CLIENT_PRIORITY);

    /*
    std::string host_id = unit_tree.get<std::string>(HOST_ID);    
    if (host_id.find("0x") == 0 || host_id.find("0X") == 0) 
    {
        host_id = host_id.substr(2);
    }
    UUID_t uuid(host_id);
    unit_settings_struct.client_uuid = uuid;        
    */
    return unit_settings_struct;
}


/*
*
*/
struct experiment_parameter ConfigReader::check_experiment_settings(boost::property_tree::ptree config_parsed_file)
{
    struct experiment_parameter experiment_settings;

    boost::property_tree::ptree experiment_tree = config_parsed_file.get_child(EXPERIMENT_SETTINGS);

    experiment_settings.experiment_number               = experiment_tree.get<uint32_t>(EXPERIMENT_NUMBER);
    experiment_settings.client_init_time                = std::chrono::milliseconds(experiment_tree.get<uint32_t>(CLIENT_INIT_TIME));
    experiment_settings.experiment_begin_offset         = std::chrono::milliseconds(experiment_tree.get<uint32_t>(EXPERIMENT_BEGIN_OFFSET));
    experiment_settings.experiment_end_offset           = std::chrono::milliseconds(experiment_tree.get<uint32_t>(EXPERIMENT_END_OFFSET));
    experiment_settings.experiment_iterations           = experiment_tree.get<uint32_t>(EXPERIMENT_ITERATIONS);
    //experiment_settings.synchronous_mode                 = experiment_tree.get<bool>(EXPERIMENT_SYNCHRONOUS_FLAG);
    experiment_settings.synchronous_start_mode          = experiment_tree.get<bool>(EXPERIMENT_SYNCHRONOUS_START_FLAG);
    experiment_settings.mc_distribution_phase_duration  = std::chrono::milliseconds(experiment_tree.get<uint32_t>(MC_DISTRIBUTION_PHASE_DURATION));
    experiment_settings.mc_client_stop_offset           = std::chrono::milliseconds(experiment_tree.get<uint32_t>(MC_CLIENT_STOP_OFFSET));
    experiment_settings.mc_client_reconfig_offset       = std::chrono::milliseconds(experiment_tree.get<uint32_t>(MC_CLIENT_RECONFIG_OFFSET));
    experiment_settings.mc_client_start_offset          = std::chrono::milliseconds(experiment_tree.get<uint32_t>(MC_CLIENT_START_OFFSET));
    experiment_settings.inter_mc_gap_min                = std::chrono::milliseconds(experiment_tree.get<uint32_t>(INTER_MC_GAP_MIN));
    experiment_settings.inter_mc_gap_max                = std::chrono::milliseconds(experiment_tree.get<uint32_t>(INTER_MC_GAP_MAX));
    experiment_settings.hyperperiod_duration            = std::chrono::milliseconds(experiment_tree.get<uint32_t>(HYPERPERIOD_DURATION));
    //experiment_settings.hyperperiod_slots               = experiment_tree.get<uint32_t>(HYPERPERIOD_SLOTS);

    experiment_settings.startup_mode = experiment_tree.get<uint32_t>(EXPERIMENT_STARTUP_MODE); 
    for (const auto& outer : experiment_tree.get_child(EXPERIMENT_STARTUP_MODE_MAP)) 
    {
        uint32_t outer_key = std::stoi(outer.first);  // Convert string key to integer
        std::map<uint32_t, uint32_t> inner_map;
        for (const auto& inner : outer.second) 
        {
            uint32_t inner_key = std::stoi(inner.first);  // Convert string key to integer
            uint32_t value = inner.second.get_value<uint32_t>();
            inner_map[inner_key] = value;
        }
        experiment_settings.startup_mode_map[outer_key] = inner_map;
    }

    for (const auto &reconfiguration_order_child : experiment_tree.get_child(EXPERIMENT_RECONFIGURATION_ORDER)) 
    {
        experiment_settings.reconfiguration_order.push_back(reconfiguration_order_child.second.get_value<uint32_t>());
    }            
    for (const auto& outer : experiment_tree.get_child(EXPERIMENT_RECONFIGURATION_MAP)) 
    {
        uint32_t outer_key = std::stoi(outer.first);  // Convert string key to integer
        std::map<uint32_t, uint32_t> inner_map;
        for (const auto& inner : outer.second) 
        {
            uint32_t inner_key = std::stoi(inner.first);  // Convert string key to integer
            uint32_t value = inner.second.get_value<uint32_t>();
            inner_map[inner_key] = value;
        }
        experiment_settings.reconfiguration_map[outer_key] = inner_map;
    }
    
    return experiment_settings;
}


/*
*
*/
uint32_t ConfigReader::convert_hex_to_int(std::string hex_string)
{
    if(!hex_string.empty())
    {
        // Remove the "0x" prefix if present
        hex_string = remove_hex_prefix_string(hex_string);
        // convert hex to uint32_t
        return std::stoul(hex_string, nullptr, 16);
    }
    else
    {
        return 0;
    }
}


/*
*
*/
std::string ConfigReader::remove_hex_prefix_string(std::string hex_string)
{
    if(!hex_string.empty())
    {
        // Remove the "0x" prefix if present
        if (hex_string.find("0x") == 0 || hex_string.find("0X") == 0) 
        {
            hex_string = hex_string.substr(2);
        }
        return hex_string;
    }
    else
    {
        return 0;
    }
}