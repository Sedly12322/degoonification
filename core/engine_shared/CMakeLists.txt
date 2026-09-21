cmake_minimum_required(VERSION 3.20)
project(engine_shared VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(include)

add_library(engine_shared SHARED
    src/temporal_smoothing.cpp
    src/bloom_filter.cpp
    src/radix_tree.cpp
    src/c_api.cpp
)

add_library(engine_shared_static STATIC
    src/temporal_smoothing.cpp
    src/bloom_filter.cpp
    src/radix_tree.cpp
    src/c_api.cpp
)

target_include_directories(engine_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(engine_shared_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(BUILD_TESTING)
    enable_testing()
    foreach(test_name test_tracker test_dns test_c_api)
        add_executable(${test_name} tests/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE engine_shared_static)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
