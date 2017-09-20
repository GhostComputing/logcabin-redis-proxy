cmake_minimum_required(VERSION 3.7)
project(logcabin_redis_proxy)

set(CMAKE_CXX_STANDARD 11)

if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/deps/pre_built)
    message(STATUS "Uncompress pre_built package =>")
    execute_process(
            COMMAND "tar" axvf ${CMAKE_CURRENT_SOURCE_DIR}/deps/pre_built.tar.gz
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps
    )
endif()

set(SOURCE_FILES
            src/ae/ae_epoll.c
            src/ae/ae.c
            src/ae/anet.c
            src/proxy/proxy.cc
            src/proxy/main.cc
    )

set(THIRD_PARTY_LIBS
            ${CMAKE_CURRENT_SOURCE_DIR}/deps/pre_built/logcabin/liblogcabin.a
            ${CMAKE_CURRENT_SOURCE_DIR}/deps/pre_built/gtest-c6d1811/libgtest.a
            ${CMAKE_CURRENT_SOURCE_DIR}/deps/pre_built/rocksdb-5.2.1/librocksdb.a
            ${CMAKE_CURRENT_SOURCE_DIR}/deps/pre_built/protobuf-2.6.1/libprotobuf.a
    )

include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/deps/simple_resp/include
            ${CMAKE_CURRENT_SOURCE_DIR}/deps/pre_built/logcabin/include
    )

add_subdirectory(deps/simple_resp)

if (APPLE)
    message(STATUS "Can not find library in non-Linux machine")
elseif (UNIX)
    find_library(LZ4_LIB_STATIC NAMES liblz4.a)
    find_library(CRYPTOPP_LIB_STATIC NAMES libcrypto++.a)
    find_library(SNAPPY_LIB_STATIC NAMES libsnappy.a)
    find_library(BZ2_LIB_STATIC NAMES libbz2.a)
    find_library(JEMALLOC_LIB_STATIC NAMES libjemalloc.a)
    find_library(BOOST_SYSTEM_LIB_STATIC NAMES libboost_system.a)

    set(BASE_LIBS
            ${LZ4_LIB_STATIC}
            ${CRYPTOPP_LIB_STATIC}
            ${SNAPPY_LIB_STATIC}
            ${BZ2_LIB_STATIC}
            ${JEMALLOC_LIB_STATIC}
            ${BOOST_SYSTEM_LIB_STATIC}
            z pthread rt
    )
else ()
    message(STATUS "Can not find library in non-Linux machine")
endif ()

add_compile_options(-ggdb3)

add_executable(logcabin_redis_proxy ${SOURCE_FILES})
target_link_libraries(logcabin_redis_proxy simpleresp ${THIRD_PARTY_LIBS} ${BASE_LIBS})
