cmake_minimum_required(VERSION 2.5)
project(swiftNetCore)

# mymuduo 最终编译为so动态库，设置动态库的路径。
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

# 定义参与编译的源文件
aux_source_directory(./src SRC_LIST)
#编译动态库
add_library(swiftNetCore SHARED ${SRC_LIST})