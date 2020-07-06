# cmake needs this line
cmake_minimum_required(VERSION 2.8)
# Define project name
project(video_capture)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
set(CMAKE_C_FLAGS_Release "${CMAKE_C_FLAGS_Release} -O3")
#SET(CMAKE_SYSTEM_NAME Linux)
#SET(CMAKE_C_COMPILER /home/jinhong/imx6_gcc_g++/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)
#SET(CMAKE_CXX_COMPILER /home/jinhong/imx6_gcc_g++/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-c++)

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI
#set(OpenCV_DIR/usr/local/opencv/release)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
#find_package(OpenCV REQUIRED)

# If the package has been found, several variables will
# be set, you can find the full list with descriptions
# in the OpenCVConfig.cmake file.
# Print some message showing some of them
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

if(CMAKE_VERSION VERSION_LESS "2.8.11")
  # Add OpenCV headers location to your include paths
  include_directories(${OpenCV_INCLUDE_DIRS})
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(protocol_lib STATIC ./include/frame_protocol.c) 
add_library(shm_lib STATIC ./include/shm.c ./include/sem.c) 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Declare the executable target built from your sources
add_executable(video_capture video_capture.cpp)
# Link your application with OpenCV libraries 被依赖的库的库放在依赖它的库的后面
target_link_libraries(video_capture shm_lib protocol_lib  ${OpenCV_LIBS} ) 

add_executable(video_server video_server.cpp)
target_link_libraries(video_server shm_lib protocol_lib pthread ${OpenCV_LIBS} ) 

add_library(jpeg_lib STATIC ./include/yuv_to_jpeg.cpp) 
add_executable(video_client video_client.cpp)
target_link_libraries(video_client shm_lib protocol_lib jpeg_lib jpeg pthread ${OpenCV_LIBS} ) 

add_executable(video_capture_sim video_capture_sim.cpp)
target_link_libraries(video_capture_sim shm_lib common_lib  ${OpenCV_LIBS} ) 

