cmake_minimum_required(VERSION 3.0)
project(packet
    VERSION 1.0.0
    LANGUAGES C CXX
     DESCRIPTION "packet"
)

# 设置默认构建类型（如果未指定）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type: Debug or Release" FORCE)
endif()


# 输出目录设置（支持多平台）
set(LIBRARY_OUTPUT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/bin/${ARCH_DIR}" CACHE PATH "Library output directory")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT})

# 创建输出目录（如果不存在）
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_ROOT})
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output directory: ${LIBRARY_OUTPUT_ROOT}")



# 添加子项目（有选择性地添加）
option(BUILD_PS_DEMUX "Build ps_demux library" ON)
option(BUILD_PS_MUX "Build ps_mux library" ON)
option(BUILD_RTP_DEPACKET "Build rtpdepacket library" ON)
option(BUILD_RTP_PACKET "Build rtppacket library" ON)



if(BUILD_PS_DEMUX)
    add_subdirectory(ps_demux)
    set_target_properties(PsDemux  PROPERTIES
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
    RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
)
endif()

if(BUILD_PS_MUX)
    add_subdirectory(ps_mux)
    set_target_properties(PsMux  PROPERTIES
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
    RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
)
endif()

if(BUILD_RTP_DEPACKET)
    add_subdirectory(rtpdepacket)
   set_target_properties(RtpDepacket  PROPERTIES
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
    RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
)
endif()

if(BUILD_RTP_PACKET)
    add_subdirectory(rtppacket)
    set_target_properties(RtpPacket  PROPERTIES
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
    RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_ROOT}
)
endif()



# 安装规则（可选）
install(DIRECTORY ${LIBRARY_OUTPUT_ROOT}/ 
    DESTINATION lib
    FILES_MATCHING PATTERN "*.a" PATTERN "*.so" PATTERN "*.dll"
)