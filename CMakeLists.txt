cmake_minimum_required(VERSION 3.10)

project("Server")

# find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(easylogging)
add_subdirectory(server)

include_directories(
  "src"
  ${EASYLOGGING_DIR}
  ${SERVER_DIR}
  ${GTEST_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIR}
  )

set(SOURCE_FILES
  # src/server.cpp
  )

add_executable(Server src/server_main.cpp ${SOURCE_FILES})
add_executable(Client src/client_main.cpp ${SOURCE_FILES})
add_executable(Plays src/main.cpp ${SOURCE_FILES})

target_link_libraries(Server server easyloggingpp boost_iostreams)
target_link_libraries(Client server easyloggingpp boost_iostreams boost_filesystem)
target_link_libraries(Plays easyloggingpp boost_iostreams)

