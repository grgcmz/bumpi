cmake_minimum_required(VERSION 3.0)
project(sbg_client C)

add_subdirectory(includes/sbgECom)

add_executable(sbg_client main.c)

# Link against the libsbgECom library
target_link_libraries(sbg_client PRIVATE sbgECom)

