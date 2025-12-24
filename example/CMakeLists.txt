file(GLOB SRC_FILES "*.cpp")
# set(SRC_FILES
# )

foreach(SRC_FILE ${SRC_FILES})
    message(STATUS "using ${SRC_FILE}")
    get_filename_component(TARGET_NAME ${SRC_FILE} NAME_WE)
    add_executable(${TARGET_NAME} ${SRC_FILE})
    target_link_libraries(${TARGET_NAME} external gtest_main benchmark huawei_obs_sdk)
endforeach()
