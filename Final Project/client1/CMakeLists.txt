cmake_minimum_required(VERSION 3.12)
project(client1)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")
add_executable(client1 ${SOURCES})

find_package(SEAL)
target_link_libraries(client1 SEAL::seal)
