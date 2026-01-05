cmake_minimum_required(VERSION 3.12)
project(packet
    VERSION 1.0.1031
    LANGUAGES C CXX
     DESCRIPTION "packet"
)


# 设置MT编译选项（静态链接运行时库）
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
endif()


# 设置默认构建类型（如果未指定）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type: Debug or Release" FORCE)
endif()

#-----------------------------------------------------------------------------
# 平台检测
#-----------------------------------------------------------------------------
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(LIB_PLATFORM "arm64")
    message(STATUS "目标平台: ARM64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(LIB_PLATFORM "linux_x86")
    message(STATUS "目标平台: Linux x86_64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    set(LIB_PLATFORM "x64")
    message(STATUS "目标平台: Windows x64")
else()
    message(FATAL_ERROR "不支持的平台: ${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")
endif()

# 输出目录设置（支持多平台）
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIB_PLATFORM})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${LIB_PLATFORM})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${LIB_PLATFORM})

# 添加子项目（有选择性地添加）
option(BUILD_PS_DEMUX "Build ps_demux library" ON)
option(BUILD_PS_MUX "Build ps_mux library" ON)
option(BUILD_RTP_DEPACKET "Build rtpdepacket library" ON)
option(BUILD_RTP_PACKET "Build rtppacket library" ON)



if(BUILD_PS_DEMUX)
    add_subdirectory(ps_demux)
    set_target_properties(ps_demux  PROPERTIES
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
endif()

if(BUILD_PS_MUX)
    add_subdirectory(ps_mux)
    set_target_properties(ps_mux  PROPERTIES
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
   LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
endif()

if(BUILD_RTP_DEPACKET)
    add_subdirectory(rtpdepacket)
   set_target_properties(rtpdepacket  PROPERTIES
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
endif()

if(BUILD_RTP_PACKET)
    add_subdirectory(rtppacket)
    set_target_properties(rtppacket  PROPERTIES
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
endif()

# 安装规则（可选）
install(DIRECTORY ${LIBRARY_OUTPUT_ROOT}/ 
    DESTINATION lib
    FILES_MATCHING PATTERN "*.a" PATTERN "*.so" PATTERN "*.dll"
)

# 编译后自动拷贝头文件到 Include 目录
add_custom_target(copy_headers ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/include
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/ps_demux/ps_demux.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ps_mux/ps_mux.h
        ${CMAKE_CURRENT_SOURCE_DIR}/rtpdepacket/rtp_depacket.h
        ${CMAKE_CURRENT_SOURCE_DIR}/rtppacket/rtp_packet.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    COMMENT "Copying public headers to Include directory"
)