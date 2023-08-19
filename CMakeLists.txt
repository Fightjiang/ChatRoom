# 设置 cmake 的最低版本和项目名称
cmake_minimum_required(VERSION 3.0)

project(Tiny_ChatRoom)

# 生成 debug 版本，可以进行 gdb 调试
set(CMAKE_BUILD_TYPE "Debug")

# 设置项目可行性文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 设置项目库文件输出的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib) 

# 设置项目编译头文件搜索路径 -I
include_directories(${PROJECT_SOURCE_DIR}/include) 

# # # 设置项目库文件搜索路径 -L
# link_directories(${PROJECT_SOURCE_DIR}/lib)

aux_source_directory(${PROJECT_SOURCE_DIR}/src/base SRC_BASE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/rpc SRC_RPC) 
aux_source_directory(${PROJECT_SOURCE_DIR}/src/proxy SRC_PROXY) 
aux_source_directory(${PROJECT_SOURCE_DIR}/src/user SRC_USER) 

add_library(Tiny_ChatRoom  
            ${SRC_BASE}   
            ${SRC_RPC}
            ${SRC_PROXY}
            ${SRC_USER}
           )

# 目标动态库所需连接的库（这里需要连接libpthread.so）
target_link_libraries(Tiny_ChatRoom protobuf muduo_net muduo_base pthread mysqlclient zookeeper_mt hiredis)

# src 包含了 Tiny_ChatRoom 所有的相关代码
add_subdirectory(src)

# example 包含了 Tiny_ChatRoom 框架是由的示例代码
add_subdirectory(example)