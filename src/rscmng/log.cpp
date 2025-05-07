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


#include <rscmng/utils/log.hpp>


/*
*
*/
void rscmng::init_rm_file_log(std::string log_prefix, std::string log_suffix)
{
    char* home_dir = getenv("HOME");

    std::string log_file = std::string(home_dir) + "/" + log_directory + log_prefix + log_suffix + ".log";

    boost::log::add_common_attributes();
    
    #ifdef FILE_ON
    boost::log::add_file_log
    (
        boost::log::keywords::file_name = log_file,
        boost::log::keywords::target_file_name = log_file,
        boost::log::keywords::auto_flush = true,
        boost::log::keywords::format = "%TimeStamp%, %Message%",
        boost::log::keywords::filter = boost::log::trivial::severity == boost::log::trivial::trace
    );
    #endif

    #ifdef CONSOLE_ON
    boost::log::add_console_log
    (
        std::cout,
        boost::log::keywords::format = "[%TimeStamp%][%Severity%]: %Message%",
        boost::log::keywords::auto_flush = true,
        boost::log::keywords::filter = boost::log::trivial::severity >= boost::log::trivial::info
    );
    #endif
}


/*
*
*/
void rscmng::init_app_log(std::string log_prefix, std::string log_suffix)
{
    char* home_dir = getenv("HOME");

    std::string log_file = std::string(home_dir) + "/" + log_directory + log_prefix + log_suffix + ".log";
    
    boost::log::add_common_attributes();    

    boost::log::add_file_log
    (
        boost::log::keywords::file_name = log_file,
        boost::log::keywords::target_file_name = log_file,
        boost::log::keywords::auto_flush = true,
        boost::log::keywords::format = "%TimeStamp%, %Message%",
        boost::log::keywords::filter = boost::log::trivial::severity == boost::log::trivial::debug
    );
}