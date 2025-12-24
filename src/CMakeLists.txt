add_executable(hw_obs_bench
    hw_obs_bench.cpp
)

target_link_libraries(hw_obs_bench external gtest_main benchmark huawei_obs_sdk)

add_executable(hw_obs_test
    hw_obs_test.cpp
)

target_link_libraries(hw_obs_test external gtest_main benchmark huawei_obs_sdk)
