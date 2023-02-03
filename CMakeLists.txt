cmake_minimum_required(VERSION 3.16)
project(kolokwium C)

set(CMAKE_C_STANDARD 11)
add_link_options(-pthread)

add_executable(server server.c)
add_executable(client client.c)
target_link_libraries(server rt)
target_link_libraries(client rt)


