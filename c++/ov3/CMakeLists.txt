cmake_minimum_required(VERSION 3.26)
project(ov3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra -Wno-unused-parameter")

if(APPLE)
    include_directories(/usr/local/include /opt/homebrew/include)
endif()


add_executable(ov3 main.cpp)
