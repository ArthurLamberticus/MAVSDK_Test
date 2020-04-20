cmake_minimum_required(VERSION 2.8.12)

project(recup_rotor)

if(NOT MSVC)
    add_definitions("-std=c++11 -Wall -Wextra")
else()
    add_definitions("-std=c++11 -WX -W2")
endif()

find_package(MAVSDK REQUIRED)

add_executable(recup_rotor
    recup_rotor.cpp
)

target_link_libraries(recup_rotor
    MAVSDK::mavsdk_telemetry
    MAVSDK::mavsdk_action
    MAVSDK::mavsdk
)
