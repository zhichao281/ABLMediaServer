cmake_minimum_required(VERSION 3.12)
project(ABLMediaServer
    VERSION 1.0.0
     DESCRIPTION "ABLMediaServer"
    LANGUAGES C CXX
)


#=============================
# 构建选项
#=============================
option(USE_BOOST "Use Boost library" OFF)
option(USE_WEBRTC "Use WEBRTC library" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

#=============================
# 构建类型（默认 Release）
#============================
set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type: Debug or Release" FORCE)
#set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O0 -Wall")

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fPIC")

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-fPIC")



#=============================
# 架构判断  # 根据不同架构设置输出目录和链接目录
#=============================
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(ARCH_DIR "linux86")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(ARCH_DIR "arm64")
else()
    message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()
message(STATUS "CMAKE_SYSTEM_PROCESSOR:${CMAKE_SYSTEM_PROCESSOR}  Target architecture: ${ARCH_DIR}")


#=============================
# 输出与依赖目录
#=============================
set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin/${ARCH_DIR}")
set(MEDIA_SERVER_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/${CMAKE_SYSTEM_PROCESSOR}")

#=============================
# RPATH 设置
#=============================
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../lib")
elseif(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path;@loader_path/../lib")
endif()

#=============================
# 宏定义
#=============================
add_compile_definitions(FFMPEG6)


#=============================
# Boost
#=============================
if(USE_BOOST)
    set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rd/boost")
    set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
    set(BOOST_LIBRARYDIR "${BOOST_ROOT}/${ARCH_DIR}")
    add_compile_definitions(USE_BOOST)
    include_directories(${BOOST_INCLUDEDIR})
    link_directories(${BOOST_LIBRARYDIR})
endif()



#=============================
# 包含目录
#=============================
set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/ffmpeg6/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson-master/include
    ${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/libflv/include
    ${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/librtmp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/libmpeg/include
    ${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/libhls/include
    ${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/librtp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/libmov/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ABLMediaServer
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/spdlog/include
)

include_directories(${PROJECT_INCLUDE_DIRS})

#=============================
# 链接目录
#=============================
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/ffmpeg6/lib/${ARCH_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/faac-1.30/lib/${ARCH_DIR}
    ${OUTPUT_DIR}
    ${MEDIA_SERVER_LIB_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/XHNetSDK/openssl/lib/${ARCH_DIR}
)

#=============================
# 子项目
#=============================
if(USE_WEBRTC)
    add_subdirectory(simple-media-server)
endif()

add_subdirectory(XHNetSDK)
set_target_properties(XHNetSDK  PROPERTIES
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

add_subdirectory(packet)
set_target_properties(packet  PROPERTIES
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)




add_subdirectory(media-server-master)

#=============================
# 源文件
#=============================
file(GLOB ABL_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ABLMediaServer/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ABLMediaServer/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ABLMediaServer/ffmpeg/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/spdlog/*.cpp
)

# 这里可选排除文件
list(FILTER ABL_SRCS EXCLUDE REGEX ".*/test\\.cpp$")

#=============================
# 添加可执行文件
#=============================
add_executable(${PROJECT_NAME} ${ABL_SRCS})

set_target_properties(${PROJECT_NAME}  PROPERTIES
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

#设置RPATH路径
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")


#=============================
# 链接需要的库文件
#=============================
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    rtmp
    flv
    mpeg
    hls
    mov
    rtp)
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
    dl
)

if(USE_WEBRTC)
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    webrtc-streamer   
)
endif()

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

#=============================
# 安装
#=============================
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${OUTPUT_ROOT}/
    DESTINATION lib
    FILES_MATCHING PATTERN "*.so*" PATTERN "*.dll" PATTERN "*.a"
)