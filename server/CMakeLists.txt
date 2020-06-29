cmake_minimum_required(VERSION 3.9)

project(cmd_visualizer_server)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig)
pkg_check_modules(PC_ZeroMQ QUIET zmq)
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS serialization program_options)

include_directories(
    ../include
    ${ZeroMQ_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIR} 
    ${Boost_INCLUDE_DIR}
)

add_executable(cvs server.cpp)

target_link_libraries(cvs PUBLIC zmq ${OpenCV_LIBS} ${Boost_LIBRARIES})
install(TARGETS cvs DESTINATION /usr/local/bin)
install(FILES ../include/frame.hpp DESTINATION /usr/local/include)
install(FILES ./.cvsinfo DESTINATION $ENV{HOME})