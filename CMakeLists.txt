cmake_minimum_required(VERSION 3.5)
project(RAI_D)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    client/client.c
    client/client.h
    controller/controller.c
    controller/controller.h
    drive/drive.c
    drive/drive.h
    network/network.c
    network/network.h)
set(CLIENT_FILES
        client/client.c
        client/client.h)
set(DRIVE_FILES
        drive/drive.c
        drive/drive.h)
set(CONTROLLER_FILES
        controller/controller.c
        controller/controller.h)
set(NETWORK_FILES
        network/network.c
        network/network.h)

add_executable(client ${CLIENT_FILES})
add_executable(drive ${DRIVE_FILES})
add_executable(controller ${CONTROLLER_FILES})