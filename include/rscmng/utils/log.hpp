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


#ifndef RM_LOG_h
#define RM_LOG_h


#include <cstdlib>
#include <iostream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>


/**
 * @brief Preprocessor wrapper for boost logging library
 * 
 */
#ifdef LOG_ON

#define RM_logTrace(msg) BOOST_LOG_TRIVIAL(trace) << msg; 
#define RM_logDebug(msg) BOOST_LOG_TRIVIAL(debug) << msg;
#define RM_logInfo(msg) BOOST_LOG_TRIVIAL(info) << msg;
#define RM_logWarning(msg) BOOST_LOG_TRIVIAL(warning) << msg;
#define RM_logError(msg) BOOST_LOG_TRIVIAL(error) << msg;
#define RM_logFatal(msg) BOOST_LOG_TRIVIAL(fatal) << msg;
#define AppLog(msg) BOOST_LOG_TRIVIAL(debug) << msg;

#else
#define RM_logTrace(msg)
#define RM_logDebug(msg)
#define RM_logInfo(msg)
#define RM_logWarning(msg)
#define RM_logError(msg)
#define RM_logFatal(msg)
#define AppLog(msg)
#endif

const std::string log_directory = "rscmng_logs/";

namespace rscmng {

void init_rm_file_log(std::string log_prefix, std::string log_suffix);

void init_app_log(std::string log_prefix, std::string log_suffix);

};

#endif