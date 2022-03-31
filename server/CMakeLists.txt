cmake_minimum_required(VERSION 3.10)

set(SERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

find_package(Threads REQUIRED)
find_package(Boost)

set(CMAKE_CXX_STANDARD 20)

include_directories(
        .
        ${EASYLOGGING_DIR}
)

ADD_LIBRARY(server STATIC server.cpp client.cpp task.cpp)

target_link_libraries(server Threads::Threads easyloggingpp ${Boost_LIBRARIES})

