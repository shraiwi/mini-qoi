cmake_minimum_required(VERSION 3.5)

# set the project name
project(mqoi_decoder)

include_directories(${PROJECT_SOURCE_DIR}/../../src)

# add the executable
add_executable(mqoi_decoder mqoi_decoder.c ${PROJECT_SOURCE_DIR}/../../src/mini_qoi.c)
