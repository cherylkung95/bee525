cmake_minimum_required(VERSION 3.12)
project(draft)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")
add_executable(draft ${SOURCES})

find_package(SEAL)
target_link_libraries(draft SEAL::seal)
