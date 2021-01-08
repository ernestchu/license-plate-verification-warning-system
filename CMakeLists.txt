cmake_minimum_required(VERSION 3.0)

project(scooter_lpr_system)

add_compile_options(-std=c++11 -O3)

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

add_executable(main main.cpp)

target_link_libraries(main ${OpenCV_LIBS} ${CMAKE_SOURCE_DIR}/lib/libultimate_alpr-sdk.so)
