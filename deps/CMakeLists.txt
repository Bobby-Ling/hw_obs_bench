if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif ()
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
add_subdirectory(fmt)
set(SPDLOG_FMT_EXTERNAL ON)
add_subdirectory(spdlog)
set(BUILD_GMOCK OFF)
add_subdirectory(googletest)

add_subdirectory(huawei_obs_c_sdk)

find_package(benchmark REQUIRED)

# 打包成一个external库
add_library(external INTERFACE)
target_link_libraries(external INTERFACE fmt::fmt spdlog::spdlog)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(cpptrace.cmake)
    message(STATUS "Enable Cpptrace")
    target_link_libraries(external INTERFACE cpptrace::cpptrace)
else()
    message(STATUS "Disable Cpptrace")
endif()
