cmake_minimum_required(VERSION 3.0)

project(XHNetSDK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加选项，用于选择生成动态库、静态库或可执行文件
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(BUILD_STATIC_LIBS "Build using static libraries" ON)
option(BUILD_EXECUTABLE "Build executable" OFF)

# 含 当前 文件夹下的头文件目录
include_directories(${CMAKE_SOURCE_DIR})
# 包含 asio 文件夹下的头文件目录
include_directories(${CMAKE_SOURCE_DIR}/asio/include)

# 包含 thread_pool 文件夹下的头文件目录
include_directories(${CMAKE_SOURCE_DIR}/thread_pool)

# 设置项目的目标
FILE(GLOB XHNetSDK_SRC_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/*.*
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool/*.* 
)


# 根据选项生成对应的目标
if (BUILD_SHARED_LIBS)
   add_library(${PROJECT_NAME} SHARED ${XHNetSDK_SRC_LIST})
     message("build BUILD_SHARED_LIBS")
  
elseif (BUILD_STATIC_LIBS)
    add_library(${PROJECT_NAME}  STATIC ${XHNetSDK_SRC_LIST} )
     message("build BUILD_STATIC_LIBS")
else()
    add_executable(${PROJECT_NAME}  ${XHNetSDK_SRC_LIST})
     message("build out file")
endif()




# 链接当前文件夹下的 asio 头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE asio/include)

target_link_libraries(${PROJECT_NAME} pthread)

# 设置生成的库文件路径为上一级目录下的 Bin 文件夹
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/../Bin)
