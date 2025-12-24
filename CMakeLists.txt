cmake_minimum_required(VERSION 3.10)

project(lsm_tree_demo)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_COMMON_FLAGS "-Wall")
set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -ftemplate-backtrace-limit=0")
set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -fmacro-backtrace-limit=0")
set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -ferror-limit=0")

# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

message(STATUS "Using ${CMAKE_C_COMPILER_ID}")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -ggdb3")
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # https://clang.llvm.org/docs/UsersManual.html#controlling-debugger-tuning
    set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -glldb")
    set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -fstandalone-debug")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enable Debug")
    set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -O0 ")
    # 修复使用clang编译时lldb打印std::string提示 C++: error: summary string parsing error
    # https://clang.llvm.org/docs/UsersManual.html#cmdoption-fstandalone-debug
    add_definitions(-DDEBUG)
else()
    message(STATUS "Disable Debug")
    set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -O2")
endif()

# TSAN和ASAN不能同时使用
# TODO: gcc (Ubuntu 13.1.0-8ubuntu1~22.04) 13.1.0下可能产生误报, clang 17.0.6正常
# cmake -DCMAKE_BUILD_TYPE=Debug -DUSE_ASAN=ON ..
option(USE_ASAN "Enable Address Sanitizer" OFF)
if(USE_ASAN)
    message(STATUS "Enable ASan")
    set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -fno-stack-protector -fno-omit-frame-pointer")
    # https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html
    set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -fsanitize=address")
    # ASAN_OPTIONS=detect_leaks=0
    # set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -fsanitize=leak")
    set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -fsanitize=undefined")
    # set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -fsanitize=thread") # The option cannot be combined with -fsanitize=address
endif()

# cmake -DCMAKE_BUILD_TYPE=Debug -DUSE_TSAN=ON ..
option(USE_TSAN "Enable Thread Sanitizer" OFF)
if(USE_TSAN)
    message(STATUS "Enable TSan")
    # https://stackoverflow.com/questions/77850769/fatal-threadsanitizer-unexpected-memory-mapping-when-running-on-linux-kernels
    # 6.12.26-WSL2-LTS+; clang 17.0.6 需要sudo sysctl vm.mmap_rnd_bits=28才能工作, 否则一直"ThreadSanitizer: unexpected memory mapping"
    set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -fno-stack-protector -fno-omit-frame-pointer")
    # https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html
    set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -fsanitize=thread")
endif()

add_subdirectory(deps)
include_directories(include)
add_subdirectory(example)
add_subdirectory(src)
