cmake_minimum_required(VERSION 3.0)
project(media-server-master
    VERSION 1.0.0
    LANGUAGES C CXX
     DESCRIPTION "media-server-master"
)

# 设置默认构建类型（如果未指定）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type: Debug or Release" FORCE)
endif()


# 输出目录设置（支持多平台）
set(LIBRARY_OUTPUT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_SYSTEM_PROCESSOR}" CACHE PATH "Library output directory")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT})

# 创建输出目录（如果不存在）
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_ROOT})
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output directory: ${LIBRARY_OUTPUT_ROOT}")



# 添加子项目（有选择性地添加）
option(BUILD_LIBFLV "Build FLV library" ON)
option(BUILD_LIBHLS "Build HLS library" ON)
option(BUILD_LIBMOV "Build MOV library" ON)
option(BUILD_LIBMPEG "Build MPEG library" ON)
option(BUILD_LIBRTMP "Build RTMP library" ON)
option(BUILD_LIBRTP "Build RTP library" ON)


if(BUILD_LIBFLV)
    add_subdirectory(libflv)
endif()

if(BUILD_LIBHLS)
    add_subdirectory(libhls)
endif()

if(BUILD_LIBMOV)
    add_subdirectory(libmov)
endif()

if(BUILD_LIBMPEG)
    add_subdirectory(libmpeg)
endif()

if(BUILD_LIBRTMP)
    add_subdirectory(librtmp)
endif()

if(BUILD_LIBRTP)
    add_subdirectory(librtp)
endif()

# 安装规则（可选）
install(DIRECTORY ${LIBRARY_OUTPUT_ROOT}/ 
    DESTINATION lib
    FILES_MATCHING PATTERN "*.a" PATTERN "*.so" PATTERN "*.dll"
)