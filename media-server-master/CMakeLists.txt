cmake_minimum_required(VERSION 3.0)
project(media-server-master)


# 设置所有子项目的静态库输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_SYSTEM_PROCESSOR})
# 如果是动态库，可以额外设置 LIBRARY_OUTPUT_DIRECTORY
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_SYSTEM_PROCESSOR})



message("CMAKE_ARCHIVE_OUTPUT_DIRECTORY: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

# 添加所有子目录
#add_subdirectory(libdash)
add_subdirectory(libflv)
add_subdirectory(libhls)
#add_subdirectory(libmkv)
add_subdirectory(libmov)
add_subdirectory(libmpeg)
add_subdirectory(librtmp)
add_subdirectory(librtp)
#add_subdirectory(librtsp)
#add_subdirectory(libsip)

