cmake_minimum_required(VERSION 3.12)
project(lab3_2)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")
add_executable(lab3_2 ${SOURCES})

find_package(SEAL)
target_link_libraries(lab3_2 SEAL::seal)
