cmake_minimum_required(VERSION 3.0.0)
project(rscmng VERSION 0.1.0)
set (CMAKE_CXX_STANDARD 
11)

include(GNUInstallDirs)

### cmake macros
include(${PROJECT_SOURCE_DIR}/cmake/dev/set_sources.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/dev/generate_msvc_libraries.cmake)

### Build directory structure
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

### search for local boost
find_package(Boost 1.73.0 EXACT COMPONENTS thread log log_setup REQUIRED)

### Define library content
set(
    LIB_SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/rscmng/log.cpp
    ${PROJECT_SOURCE_DIR}/src/rscmng/config_reader.cpp
    ${PROJECT_SOURCE_DIR}/src/rscmng/messages.cpp
    ${PROJECT_SOURCE_DIR}/src/rscmng/rm_communication.cpp
    ${PROJECT_SOURCE_DIR}/src/rscmng/rm_abstraction.cpp
)

## Define library content
add_library(rscmng SHARED ${LIB_SRC_FILES})

target_include_directories(rscmng
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(rscmng PUBLIC boost_system boost_log boost_log_setup boost_thread pthread)
target_compile_definitions(rscmng PUBLIC BOOST_LOG_DYN_LINK)

# Enable logging via -DLOG during build process
if(DEFINED LOG)
    message("Logging activated")
    target_compile_definitions(rscmng PUBLIC LOG_ON)
endif()

# Enable file log via -DFILE during build process
if(DEFINED FILE)
    message("File logging activated")
    target_compile_definitions(rscmng PUBLIC FILE_ON)
endif()

# Enable console log via -DCONSOLE during build process
if(DEFINED CONSOLE)
    message("Console logging activated")
    target_compile_definitions(rscmng PUBLIC CONSOLE_ON)
endif()

target_compile_options(rscmng PUBLIC -g)

include(CMakePackageConfigHelpers)

### Add headers to system include dir 
set_public_headers_directory(${PROJECT_SOURCE_DIR}/include rscmng
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT headers
    INSTALL
)

### Install lib
install(
    TARGETS rscmng
    EXPORT rscmngTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    EXPORT rscmngTargets
    NAMESPACE rscmng::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rscmng
    FILE rscmngTargets.cmake
)

### Install template implementation files
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} 
    FILES_MATCHING PATTERN "*.tpp"
)

### Add package info for cmake
configure_package_config_file( 
    "rscmngConfig.cmake.in" 
    "rscmngConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rscmng
    PATH_VARS
        CMAKE_INSTALL_LIBDIR
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/rscmngConfigVersion.cmake
    VERSION 0.1.0
    COMPATIBILITY SameMajorVersion
)

### Install Config and ConfigVersion files
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/rscmngConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/rscmngConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rscmng"
)

### Build Targets

file(GLOB_RECURSE ALL_HEADERS
    ${PROJECT_SOURCE_DIR}/include/rscmng/**/*.hpp
    ${PROJECT_SOURCE_DIR}/include/rscmng/*.hpp
)

set(SRC_FILES
    ${ALL_HEADERS}
    src/rscmng/log.cpp
    src/rscmng/config_reader.cpp
    src/rscmng/messages.cpp
    src/rscmng/rm_communication.cpp
    src/rscmng/rm_abstraction.cpp
    src/rscmng/protocol_basic_payload.cpp
    src/rscmng/protocol_rm_wired_payload.cpp
    src/rscmng/traffic_generator.cpp
    src/rscmng/traffic_sink_statistic.cpp

)

# Define executables
add_executable(wired_rm_client_endnode examples/wired_basic/wired_rm_client_endnode.cpp ${SRC_FILES})
target_link_libraries(wired_rm_client_endnode rscmng boost_system boost_log boost_log_setup boost_thread pthread)

add_executable(wired_rm_client_switch examples/wired_basic/wired_rm_client_switch.cpp ${SRC_FILES})
target_link_libraries(wired_rm_client_switch rscmng boost_system boost_log boost_log_setup boost_thread pthread)

add_executable(wired_rm examples/wired_basic/wired_rm.cpp ${SRC_FILES})
target_link_libraries(wired_rm rscmng boost_system boost_log boost_log_setup boost_thread pthread)
