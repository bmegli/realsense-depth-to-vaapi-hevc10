cmake_minimum_required(VERSION 3.0)

project(
    realsense-depth-to-vaapi-hevc10
)

add_library(hve SHARED hardware-video-encoder/hve.c)

add_executable(realsense-depth-to-vaapi-hevc10 main.cpp)
target_include_directories(realsense-depth-to-vaapi-hevc10 PRIVATE hardware-video-encoder)
target_link_libraries(realsense-depth-to-vaapi-hevc10 hve avcodec avutil realsense2)
