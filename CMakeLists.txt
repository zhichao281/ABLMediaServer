cmake_minimum_required(VERSION 3.12)

project(ABLMediaServer)
# 判断是否启用了 USE_BOOST 选项
option(USE_BOOST "Use Boost library" OFF)
option(USE_WEBRTC "Use WEBRTC library" OFF)
# 设置编译选项
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 检测是否满足标准，否则报错

#set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O0 -Wall")

# 允许相对路径
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

set(CMAKE_BUILD_TYPE "Release")   # 自定义构建类型

# 添加宏定义
add_definitions(-DFFMPEG6)

# 设置输出目录
message("当前系统架构: ${CMAKE_SYSTEM_PROCESSOR}")

# 根据不同架构设置输出目录和链接目录
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    set(SYSTEM_NAME_LOWER "linux86")   # 自定义系统名称
elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    set(SYSTEM_NAME_LOWER "arm64")   # 自定义系统名称
endif()

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${SYSTEM_NAME_LOWER})
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${SYSTEM_NAME_LOWER})  

# 设置链接目录
set(LIBRARY_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/ffmpeg6/lib/${SYSTEM_NAME_LOWER}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/boost/${SYSTEM_NAME_LOWER}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/faac-1.30/lib/${SYSTEM_NAME_LOWER}
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/${SYSTEM_NAME_LOWER}
    ${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/${CMAKE_SYSTEM_PROCESSOR}
    ${CMAKE_CURRENT_SOURCE_DIR}/XHNetSDK/openssl/lib/${SYSTEM_NAME_LOWER}
)

if(USE_BOOST)
    add_definitions(-DUSE_BOOST)
    # 添加 Boost 头文件目录
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd/boost/include/)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd/boost/${SYSTEM_NAME_LOWER})
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
    ./3rd/spdlog/
)

# 添加子目录，这样子目录中的CMakeLists.txt才会被调用
add_subdirectory(XHNetSDK)
if(USE_WEBRTC)
    add_subdirectory(webrtc-streamer)
endif()

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
      ${CMAKE_CURRENT_SOURCE_DIR}/ABLMediaServer/*.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/ABLMediaServer/*.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/ABLMediaServer/ffmpeg/*.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/3rd/spdlog/*.cpp
)

# 添加可执行文件
add_executable(${PROJECT_NAME} ${ABL_SRCS})

set_target_properties(${CMAKE_PROJECT_NAME}  PROPERTIES
				ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/${SYSTEM_NAME_LOWER}"       # 静态库输出路径
				LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/${SYSTEM_NAME_LOWER}"       # 动态库输出路径
				RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/${SYSTEM_NAME_LOWER}"       # 可执行文件输出路径
                )
# 设置INSTALL_RPATH，用于安装时
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "$ORIGIN:/home/server/ABLMediaServer/src/bin/linux86"
    BUILD_WITH_INSTALL_RPATH TRUE  # 确保构建时 RPATH 生效
)

# 设置RPATH，用于构建时
set_target_properties(${PROJECT_NAME} PROPERTIES
    RPATH "$ORIGIN"
    LINK_FLAGS "-Wl,-rpath,./"

)

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
    XHNetSDK  # 链接 XHNetSDK 库
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
