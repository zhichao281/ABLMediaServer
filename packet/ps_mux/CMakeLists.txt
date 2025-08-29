cmake_minimum_required(VERSION 3.12)


project(PsMux
    VERSION 0.6.0.4
    DESCRIPTION "PsMux"
    LANGUAGES C CXX)

# 使用现代目标式 CMake 替代传统变量
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认隐藏符号（对应 -fvisibility=hidden）
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# 设置默认构建类型（与 Makefile 中的 RELEASE 变量对应）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# ========================
# 系统宏定义
# ========================
if(UNIX AND NOT APPLE)
    add_definitions(-DOS_LINUX)
elseif(APPLE)
    add_definitions(-DOS_MAC)
endif()


# 根据构建类型设置选项
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DNDEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
else()
    add_definitions(-DDEBUG -D_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

# ========================
# 配置选项
# ========================
option(BUILD_PACKET_SHARED "Build as shared library" ON)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 源文件收集（对应 SOURCE_FILES）
file(GLOB_RECURSE SOURCE_FILES "*.cpp")

# 生成库
if(BUILD_PACKET_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
else()
    add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
endif()



# 现代头文件包含方式（PUBLIC 表示会传递给依赖此库的目标）
target_include_directories(${PROJECT_NAME}
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        # 仅内部使用的头文件路径可以放这里
)

