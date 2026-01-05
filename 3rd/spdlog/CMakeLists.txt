cmake_minimum_required(VERSION 3.13)

project(spdloghead
    VERSION 1.26.0.0105
    DESCRIPTION "spdloghead"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SPDLOGHEAD_BUILD_SHARED "Build spdloghead shared library" ON)
option(SPDLOGHEAD_BUILD_STATIC "Build spdloghead static library" OFF)

file(GLOB SPDLOGHEAD_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/spdloghead.cpp
)

if(SPDLOGHEAD_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SPDLOGHEAD_SRC})
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE SPDLOGHEAD_EXPORTS _WIN32)    
    endif() 

    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "spdloghead"
        POSITION_INDEPENDENT_CODE ON
    )
endif()

if(SPDLOGHEAD_BUILD_STATIC)
    add_library(${PROJECT_NAME}_static STATIC ${SPDLOGHEAD_SRC})
    target_include_directories(${PROJECT_NAME}_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
     if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32)    
    endif() 

    set_target_properties(spdloghead_static PROPERTIES
        OUTPUT_NAME "spdloghead_static"
        POSITION_INDEPENDENT_CODE ON
    )
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)

if(SPDLOGHEAD_BUILD_SHARED)
    install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    )
endif()

if(SPDLOGHEAD_BUILD_STATIC)
    install(TARGETS ${PROJECT_NAME}_static
        ARCHIVE DESTINATION lib
        INCLUDES DESTINATION include
    )
endif()