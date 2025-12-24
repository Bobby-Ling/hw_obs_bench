# 预编译的
add_library(huawei_obs_sdk SHARED IMPORTED GLOBAL)

set_target_properties(huawei_obs_sdk PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/libeSDKOBS.so"
)

target_include_directories(huawei_obs_sdk INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_directories(huawei_obs_sdk INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# RUNPATH / RPATH
target_link_options(huawei_obs_sdk INTERFACE
    "LINKER:-rpath,${CMAKE_CURRENT_SOURCE_DIR}/lib"
    "LINKER:--disable-new-dtags"
)