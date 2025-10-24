cmake_minimum_required(VERSION 3.12)

project(XHNetSDK
    VERSION 1.0.0
     DESCRIPTION "XHNetSDK"
    LANGUAGES C CXX
)


# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(USE_BOOST "Use Boost library" ON)


# 检测操作系统
if(WIN32)
    message(STATUS "Building for Windows platform")
    # Windows特定设置
    add_definitions(-DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0601)
    
    # 添加wepoll源文件
    set(WEPOLL_SRC wepoll.c)
    
    # Windows编译选项
    add_compile_options(/fp:precise /W3 /GL /MP)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    
else()
    message(STATUS "Building for Unix-like platform")
    # Linux/Unix编译选项
    add_compile_options(-fPIC -fvisibility=hidden)
endif()

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

#=============================
# Boost
#=============================
if(USE_BOOST)
    set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rd/boost")
    set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
    set(BOOST_LIBRARYDIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/Linux")
    set(Boost_LIBRARIES       
        boost_system
        boost_thread
        boost_log
        boost_filesystem
        boost_date_time
        boost_chrono
        boost_atomic)

    add_compile_definitions(USE_BOOST)
    include_directories(${BOOST_INCLUDEDIR})
    link_directories(${BOOST_LIBRARYDIR})
endif()


# 源文件列表

# 源文件列表（加上src/前缀）
set(SOURCES
    src/client.cpp
    src/client_manager.cpp
    src/identifier_generator.cpp
    src/XHNetSDK.cpp
    src/ClientReadPool.cpp
    src/ClientSendPool.cpp
    src/ConnectCheckPool.cpp
    src/ListenPool.cpp
    src/MediaFifo.cpp
    src/UdpPool.cpp
)

# 如果是Windows，添加wepoll.c
if(WIN32)
    list(APPEND SOURCES src/${WEPOLL_SRC})
endif()

# 添加共享库
add_library(XHNetSDK SHARED ${SOURCES})

# 设置库属性
set_target_properties(XHNetSDK  PROPERTIES
    OUTPUT_NAME "XHNetSDK"
    VERSION 1.0.0
    SOVERSION 1
    # 让运行时查找当前目录
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE  # 让 INSTALL_RPATH 在构建时也生效
    SKIP_BUILD_RPATH FALSE         # 不跳过构建时的 RPATH
)
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")

# 平台特定的链接库
if(WIN32)
    target_link_libraries(XHNetSDK
        ${Boost_LIBRARIES}
        ws2_32
        advapi32
    )
    # Windows下的库命名约定
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".dll")
else()
    target_link_libraries(${PROJECT_NAME}
        ${Boost_LIBRARIES}
        pthread
    )
    # Linux下的库命名约定
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".so")
endif()



# 安装目标
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# 可选：复制依赖的DLL（Windows）
if(WIN32)
    # 如果需要在安装时复制Boost DLL
    install(FILES ${Boost_LIBRARIES} DESTINATION bin)
endif()