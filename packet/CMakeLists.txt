cmake_minimum_required(VERSION 3.12)

project(packet
    VERSION 1.0.0
     DESCRIPTION "packet"
    LANGUAGES C CXX
)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项，包括 -fPIC
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# 添加选项，用于选择生成动态库还是静态库
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# 设置输出目录
set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Bin)
set(LIBRARY_OUTPUT_PATH ${OUTPUT_DIR})



# 根据选项设置生成的库类型
if (BUILD_SHARED_LIBS)
# 构建四个库
file(GLOB PS_DEMUX_SOURCES ps_demux/*.cpp)
# 添加动态库
add_library(ps_demux STATIC  ${PS_DEMUX_SOURCES})



file(GLOB PS_MUX_SOURCES ps_mux/*.cpp)
add_library(ps_mux STATIC ${PS_MUX_SOURCES})


file(GLOB RTPDEPACKET_SOURCES rtpdepacket/*.cpp)
add_library(rtpdepacket STATIC ${RTPDEPACKET_SOURCES})

file(GLOB RTPPACKET_SOURCES rtppacket/*.cpp)
add_library(rtppacket STATIC ${RTPPACKET_SOURCES})

else()

# 构建四个库
file(GLOB PS_DEMUX_SOURCES ps_demux/*.cpp)
# 添加动态库
add_library(ps_demux SHARED ${PS_DEMUX_SOURCES})



file(GLOB PS_MUX_SOURCES ps_mux/*.cpp)
add_library(ps_mux SHARED ${PS_MUX_SOURCES})


file(GLOB RTPDEPACKET_SOURCES rtpdepacket/*.cpp)
add_library(rtpdepacket SHARED ${RTPDEPACKET_SOURCES})

file(GLOB RTPPACKET_SOURCES rtppacket/*.cpp)
add_library(rtppacket SHARED ${RTPPACKET_SOURCES})

endif()


