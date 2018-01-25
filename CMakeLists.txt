cmake_minimum_required(VERSION 2.8)

project(libkcp)

set(CMAKE_CXX_STANDARD 17)

set(CONFIG_LIST 
    CMAKE_C_FLAGS 
    CMAKE_C_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_RELWITHDEBINFO
    CMAKE_CXX_FLAGS 
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_RELWITHDEBINFO)

foreach(CONFIG ${CONFIG_LIST})
    if (${CONFIG} MATCHES "/MD")
        string(REPLACE "/MD" "/MT" ${CONFIG} ${${CONFIG}})
    endif()
endforeach()

include_directories($ENV{BOOST_ROOT})
link_directories($ENV{BOOST_ROOT}/vc15/lib)

include_directories(src)
link_libraries(kcp)

add_subdirectory(src)
add_subdirectory(samples)