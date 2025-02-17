cmake_minimum_required(VERSION 3.0)

project(XHNetSDK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加选项，用于选择生成动态库、静态库或可执行文件
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_STATIC_LIBS "Build using static libraries" OFF)
option(BUILD_EXECUTABLE "Build executable" OFF)
option(BUILD_OPENSSL "Build SSL" ON)

add_definitions(-DASIO_STANDALONE)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}           # 含 当前 文件夹下的头文件目录
        ${CMAKE_CURRENT_SOURCE_DIR}/asio/include   # 包含 asio 文件夹下的头文件目录
        ${CMAKE_CURRENT_SOURCE_DIR}/asio/include/asio
        ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool
        ${CMAKE_CURRENT_SOURCE_DIR}/openssl/include/openssl
        ${CMAKE_CURRENT_SOURCE_DIR}/openssl/include/
  )



# 设置项目的目标
FILE(GLOB XHNetSDK_SRC_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/*.*
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool/*.* 
)


# 设置安装路径
if (UNIX AND NOT APPLE)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(INSTALL_BIN_DIR "${CMAKE_CURRENT_LIST_DIR}/bin/arm64" CACHE PATH "exe和dll的安装路径")
        set(INSTALL_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/../bin/arm64" CACHE PATH "lib的安装路径")
        set(SYSTEM_NAME_LOWER "arm64")   # 自定义系统名称
    else()
        set(INSTALL_BIN_DIR "${CMAKE_CURRENT_LIST_DIR}/bin/linux86" CACHE PATH "exe和dll的安装路径")
        set(INSTALL_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/bin/linux86" CACHE PATH "lib的安装路径")
        set(SYSTEM_NAME_LOWER "linux86")   # 自定义系统名称
    endif()
endif()

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/openssl/lib/${SYSTEM_NAME_LOWER})

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${INSTALL_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${INSTALL_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${INSTALL_LIB_DIR})

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

target_link_libraries(${PROJECT_NAME} libssl.a libcrypto.a pthread)

# 设置INSTALL_RPATH，用于安装时
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")

# 设置RPATH，用于构建时（可选）
set_target_properties(${PROJECT_NAME}  PROPERTIES   RPATH "$ORIGIN")

# 安装目标文件到 Bin 文件夹中
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${INSTALL_BIN_DIR}
    LIBRARY DESTINATION ${INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
)
install(DIRECTORY html DESTINATION ${INSTALL_BIN_DIR})
install(FILES config.json DESTINATION ${INSTALL_BIN_DIR})
install(FILES Procfile DESTINATION ${INSTALL_BIN_DIR})
