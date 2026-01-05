cmake_minimum_required(VERSION 3.12)
project(ABLMediaServer
    VERSION 1.26.0.0105
    DESCRIPTION "ABLMediaServer"
    LANGUAGES C CXX
)

# =============================
# 构建选项
# =============================
option(USE_BOOST "Use Boost library" OFF)
option(USE_WEBRTC "Use WEBRTC library" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# =============================
# 构建类型（默认 Release）
# =============================
set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type: Debug or Release" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O0 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

# =============================
# 架构判断
# =============================
if(WIN32)
    set(ARCH_DIR "${CMAKE_BUILD_TYPE}")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(ARCH_DIR "arm64")
elseif(UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i686|i386")
    set(ARCH_DIR "linux86")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()
message(STATUS "CMAKE_SYSTEM_PROCESSOR:${CMAKE_SYSTEM_PROCESSOR}  Target architecture: ${ARCH_DIR}")

# =============================
# 输出与依赖目录
# =============================
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/${ARCH_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/${ARCH_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/${ARCH_DIR}")

# =============================
# RPATH 设置
# =============================
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# =============================
# Boost
# =============================
if(USE_BOOST)
    set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rd/boost")
    set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
    set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")
    add_compile_definitions(USE_BOOST)
    include_directories(${BOOST_INCLUDEDIR})
    link_directories(${BOOST_LIBRARYDIR})
endif()

# =============================
# 子项目
# =============================


add_subdirectory(XHNetSDK)

add_subdirectory(3rd/spdlog)

add_subdirectory(packet)

add_subdirectory(media-server-master)

# =============================
# 目标名收集，便于维护
# =============================
set(MEDIA_SERVER_LIBS flv rtmp mpeg hls mov rtp)
set(PACKET_LIBS ps_mux ps_demux rtpdepacket rtppacket)

# =============================
# 设置子库输出属性
# =============================
foreach(lib XHNetSDK spdloghead ${PACKET_LIBS} ${MEDIA_SERVER_LIBS})
    set_target_properties(${lib} PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH TRUE
        SKIP_BUILD_RPATH FALSE
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endforeach()

# =============================
# 源文件
# =============================
file(GLOB ABL_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ABLMediaServer/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ABLMediaServer/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ABLMediaServer/ffmpeg/*.cpp
)
list(FILTER ABL_SRCS EXCLUDE REGEX ".*/test\\.cpp$")

# =============================
# 添加可执行文件
# =============================
add_executable(${PROJECT_NAME} ${ABL_SRCS})

# =============================
# 包含目录
# =============================
target_include_directories(${PROJECT_NAME} PRIVATE
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
    ${CMAKE_CURRENT_SOURCE_DIR}/packet/include
)

target_link_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/ffmpeg6/lib/${ARCH_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/faac-1.30/lib/${ARCH_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/XHNetSDK/openssl/lib/${ARCH_DIR}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE FFMPEG6)

set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
    SKIP_BUILD_RPATH FALSE
    LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# =============================
# 链接需要的库文件
# =============================
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${MEDIA_SERVER_LIBS}
    faac
    avcodec
    avutil
    avformat
    swscale
    avfilter
    swresample
    XHNetSDK
    ${PACKET_LIBS}
    spdloghead
    pthread
    dl
)
if(USE_WEBRTC)
    target_link_libraries(${PROJECT_NAME} PRIVATE webrtc-streamer)
endif()
if(USE_BOOST)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        boost_system
        boost_thread
        boost_log
        boost_filesystem
        boost_date_time
        boost_chrono
        boost_atomic
    )
endif()

# =============================
# 安装
# =============================
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

