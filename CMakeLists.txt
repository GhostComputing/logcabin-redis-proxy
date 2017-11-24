cmake_minimum_required(VERSION 3.5)
project(logcabin_redis_proxy)

set(CMAKE_CXX_STANDARD 11)
##find public deps first
if (APPLE)
    message(STATUS "Can not find library in non-Linux machine")
elseif (UNIX)

    find_library(LZ4_LIB_STATIC NAMES liblz4.a)
    find_library(PROTOBUF_LIB_STATIC NAMES libprotobuf.a)
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
            ${PROTOBUF_LIB_STATIC}
            ${BOOST_SYSTEM_LIB_STATIC}
            z pthread rt
    )
else ()
    message(STATUS "Can not find library in non-Linux machine")
endif ()

# build logcabin
if(NOT DEFINED LOGCABIN_DIR)
    set(LOGCABIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/logcabin/)
endif()

message("the logcabin dir is:" ${LOGCABIN_DIR})

if (NOT EXISTS ${LOGCABIN_DIR}/build)
    message(STATUS "Building logcabin =>")
    execute_process(
            COMMAND "cmake" .
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/logcabin/
    )
    execute_process(
            COMMAND "make"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/logcabin/
    )
endif()

if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/deps/glog/.libs)
    message(STATUS "Building glog =>")
    execute_process(
        COMMAND "./autogen.sh"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/glog/
        )
    execute_process(
        COMMAND "./configure"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/glog/
        )
    execute_process(
        COMMAND "make"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/glog/
        )
endif()

set(SOURCE_FILES
            src/ae/ae_epoll.c
            src/ae/ae.c
            src/ae/anet.c
            src/proxy/proxy.cc
            src/proxy/main.cc
            src/proxy/handler.cc
            src/proxy/SessionManager.cc
    )


set(THIRD_PARTY_LIBS
    ${LOGCABIN_DIR}/liblogcabin.a
    ${LOGCABIN_DIR}/build/third_party/gtest-c6d1811/libgtest.a
    ${LOGCABIN_DIR}/build/third_party/rocksdb-5.2.1/librocksdb.a
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/glog/.libs/libglog.a
    )

include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/deps/simple_resp/include
            ${CMAKE_CURRENT_SOURCE_DIR}/deps/ThreadPool
            ${CMAKE_CURRENT_SOURCE_DIR}/deps/glog/src
            ${LOGCABIN_DIR}/include
    )

add_subdirectory(deps/simple_resp)

add_compile_options(-g)


## turn this on for production release
add_compile_options(-DNDEBUG)

add_executable(logcabin_redis_proxy ${SOURCE_FILES})
target_link_libraries(logcabin_redis_proxy simpleresp ${THIRD_PARTY_LIBS} ${BASE_LIBS})
