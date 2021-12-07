cmake_minimum_required(VERSION 3.10.2)
project(K4aFrameExtractor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(libjpeg-turbo_DIR /opt/libjpeg-turbo/lib64/cmake/libjpeg-turbo)
include_directories(/opt/libjpeg-turbo/include/)

find_package(k4a)
find_package(k4arecord)
find_package(libjpeg-turbo)
find_package(OpenCV REQUIRED)

if (OpenCV_REQUIRED)
    find_package(OpenCV REQUIRED)
else()
    find_package(OpenCV)
endif ()

add_executable(transformation_example 
    main.cpp
    transformation_helpers.cpp
    transformation_helpers.h)

target_link_libraries(transformation_example PRIVATE
    k4a::k4a
    k4a::k4arecord
    /opt/libjpeg-turbo/lib64/libturbojpeg.so
    ${OpenCV_LIBS}
    stdc++fs
    )
