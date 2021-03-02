cmake_minimum_required(VERSION 3.12)
project(lab3_1)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")
add_executable(lab3_1 ${SOURCES})

find_package(SEAL)
target_link_libraries(lab3_1 SEAL::seal)
