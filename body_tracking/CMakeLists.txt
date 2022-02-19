cmake_minimum_required(VERSION 3.10.2)
project(K4aFrameExtractor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(k4a)
find_package(k4abt)
find_package(k4arecord)


add_executable(k4a_body_tracking
    k4a_body_tracking.cpp
    )

target_link_libraries(k4a_body_tracking PRIVATE
    k4a::k4a
    k4abt::k4abt
    k4a::k4arecord
    # stdc++fs
    )