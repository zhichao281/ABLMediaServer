cmake_minimum_required(VERSION 3.0)

project(XHNetSDK
    VERSION 0.2.5.4
    DESCRIPTION "XHNetSDK"
    LANGUAGES C CXX)

#=============================
# 构建类型（默认 Release）
#============================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 允许相对路径
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type: Debug or Release" FORCE)
#set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O0 -Wall")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")



# 添加选项，用于选择生成动态库、静态库或可执行文件
option(BUILD_XHNetSDK_SHARED "Build using shared libraries" ON)
option(BUILD_XHNetSDK_STATIC "Build using static libraries" OFF)

option(BUILD_OPENSSL "Build SSL" ON)
option(USE_OPENSSL "use SSL" ON)
option(USE_XHNetSDK_BOOST "Use  XHNetSDK Boost " OFF)

add_definitions(-DASIO_STANDALONE)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}           # 含 当前 文件夹下的头文件目录
        ${CMAKE_CURRENT_SOURCE_DIR}/asio/include   # 包含 asio 文件夹下的头文件目录
        ${CMAKE_CURRENT_SOURCE_DIR}/asio/include/asio
        ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool
  )
  
# 设置项目的目标
FILE(GLOB XHNetSDK_SRC_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/*.*
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool/*.* 
)
# 源文件列表
#set(XHNetSDK_SRC_LIST
 #   client.cpp
 #   client_manager.cpp
 #   identifier_generator.cpp
 #   io_context_pool.cpp
 #   libnet.cpp
 #   server.cpp
 #   server_manager.cpp
 #   udp_session.cpp
 #   udp_session_manager.cpp
 #   circular_buffer.cpp
 #   MediaFifo.cpp
 #   ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool/thread_pool.cpp
#)


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


# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${INSTALL_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${INSTALL_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${INSTALL_LIB_DIR})


#=============================
# Boost
#=============================
if(USE_XHNetSDK_BOOST)
    set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../3rd/boost")
    set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
    set(BOOST_LIBRARYDIR "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${SYSTEM_NAME_LOWER}")
    add_compile_definitions(USE_BOOST)
    include_directories(${BOOST_INCLUDEDIR})
    link_directories(${BOOST_LIBRARYDIR})
    message(STATUS "BOOST_ROOT    :${BOOST_ROOT}    BOOST_LIBRARYDIR    :${BOOST_LIBRARYDIR}    ")


endif()

if(USE_OPENSSL)
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/openssl/include/openssl
        ${CMAKE_CURRENT_SOURCE_DIR}/openssl/include/
            )
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/openssl/lib/${SYSTEM_NAME_LOWER})
endif()



# 根据选项生成对应的目标
if (BUILD_XHNetSDK_SHARED)
   add_library(${PROJECT_NAME} SHARED ${XHNetSDK_SRC_LIST})
     message("build BUILD_XHNetSDK_SHARED")
  
elseif (BUILD_XHNetSDK_STATIC)
    add_library(${PROJECT_NAME}  STATIC ${XHNetSDK_SRC_LIST} )
     message("build BUILD_XHNetSDK_STATIC")
else()
    add_executable(${PROJECT_NAME}  ${XHNetSDK_SRC_LIST})
     message("build out file")
endif()


# 链接当前文件夹下的 asio 头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE asio/include)

target_link_libraries(${PROJECT_NAME} PRIVATE  pthread)

if(USE_OPENSSL)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        libssl.a 
        libcrypto.a)
endif()

if(USE_XHNetSDK_BOOST)
  link_directories("/home/server/ABLMediaServer/src/bin/linux86")
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
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

# 设置INSTALL_RPATH，用于安装时
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")

# 安装目标文件到 Bin 文件夹中
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${INSTALL_BIN_DIR}
    LIBRARY DESTINATION ${INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
)
install(DIRECTORY html DESTINATION ${INSTALL_BIN_DIR})
install(FILES config.json DESTINATION ${INSTALL_BIN_DIR})
install(FILES Procfile DESTINATION ${INSTALL_BIN_DIR})
