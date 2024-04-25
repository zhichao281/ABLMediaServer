cmake_minimum_required(VERSION 3.12)

project(ABLMediaServer)

# 设置编译选项
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O0 -Wall")

# 添加宏定义
add_definitions(-DFFMPEG6)

# 判断是否启用了 USE_BOOST 选项
option(USE_BOOST "Use Boost library" OFF)

if(USE_BOOST)
    add_definitions(-DUSE_BOOST)
    # 添加 Boost 头文件目录
    include_directories(
        ./3rd/boost/include/
    )
endif()

# 设置输出目录
message("当前系统架构: ${CMAKE_SYSTEM_PROCESSOR}")
set(SYSTEM_NAME_LOWER "amd64")   # 自定义系统名称
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")   # 自定义构建类型
endif()

# 根据不同架构设置输出目录和链接目录
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux)    
    # 设置链接目录
set(LIBRARY_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux
    ${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/${SYSTEM_NAME_LOWER}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/ffmpeg6/lib/linux86
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/boost/linux86
)


elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/arm64)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/arm64)    
    # 设置链接目录
set(LIBRARY_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux
    ${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/${SYSTEM_NAME_LOWER}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/ffmpeg6/lib/arm64
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/boost/arm64
	)
endif()


# 设置库文件链接路径
link_directories(${LIBRARY_DIR})

# 设置头文件包含路径
include_directories(
    ./3rd/ffmpeg6/include
    ./rapidjson-master/include
    ./media-server-master/libflv/include
    ./media-server-master/librtmp/include
    ./media-server-master/libmpeg/include
    ./media-server-master/libhls/include
    ./media-server-master/librtp/include
    ./media-server-master/libmov/include
    ./ABLMediaServer/
    ./3rd/spdlog/include
)




# 添加子目录，这样子目录中的CMakeLists.txt才会被调用
#add_subdirectory(webrtc-streamer)

# 添加子目录，
#add_subdirectory(XHNetSDK)
# 链接静态库
link_libraries(
    rtmp
    flv
    mpeg
    hls
    mov
    rtp
)

# 添加源文件
file(GLOB ABL_SRCS
    ./ABLMediaServer/*.cpp
    ./ABLMediaServer/*.cc
    ./ABLMediaServer/ffmpeg/*.cpp
)

# 添加可执行文件
add_executable(${PROJECT_NAME} ${ABL_SRCS})

# 链接需要的库文件
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    faac
    avcodec
    avutil
    avformat
    swscale
    avfilter
    postproc
    swresample
    pthread
    XHNetSDK
    PsDemux
    PsMux
    RtpDepacket
    RtpPacket
    webrtc-streamer
    dl
)

# 如果启用了 USE_BOOST 选项，则链接 Boost 库文件
if(USE_BOOST)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
    boost_system
    boost_thread
    boost_log
    boost_regex
    boost_filesystem
    boost_date_time
    boost_chrono
    boost_atomic
    )
endif()


