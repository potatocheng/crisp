cmake_minimum_required(VERSION 3.16)

project(uring_echo_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBURING REQUIRED liburing)

add_executable(EchoServer
    src/EchoServer.cpp
    main.cpp
)

target_include_directories(EchoServer PRIVATE
    include
    ${LIBURING_INCLUDE_DIRS}
)

target_link_libraries(EchoServer
    ${LIBURING_LIBRARIES}
    pthread
)
