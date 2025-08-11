cmake_minimum_required(VERSION 3.0)
project(rtmp)

# 设置库类型 (对应 OUTTYPE = 2)
set(LIB_TYPE STATIC)  # STATIC 对应静态库(.a)


# 包含目录 (对应 INCLUDES = . ./include)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 收集源文件 (对应 SOURCE_PATHS 和 SOURCE_FILES)
file(GLOB_RECURSE SOURCES 
    "source/*.cpp"
    "source/*.c"
)

# 排除不需要的源文件 (对应 SOURCE_FILES := $(filter-out...))
# 假设要排除的文件列表在 _SOURCE_FILES 变量中
if(DEFINED _SOURCE_FILES)
    list(REMOVE_ITEM SOURCES ${_SOURCE_FILES})
endif()

# 创建库
add_library(${PROJECT_NAME} ${LIB_TYPE} ${SOURCES})

