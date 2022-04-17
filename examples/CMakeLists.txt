cmake_minimum_required(VERSION 3.5)

# set the project name
project(mqoi_decoder)

include_directories(${PROJECT_SOURCE_DIR}/../)

# add the executable
add_executable(mqoi_decoder mqoi_decoder.c ${PROJECT_SOURCE_DIR}/../mini_qoi.c)
