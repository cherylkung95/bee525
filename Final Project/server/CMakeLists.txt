cmake_minimum_required(VERSION 3.12)
project(server)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")
add_executable(server ${SOURCES})

find_package(SEAL)
target_link_libraries(server SEAL::seal)
