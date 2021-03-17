cmake_minimum_required(VERSION 3.12)
project(client2)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")
add_executable(client2 ${SOURCES})

find_package(SEAL)
target_link_libraries(client2 SEAL::seal)