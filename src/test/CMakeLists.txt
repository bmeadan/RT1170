cmake_minimum_required(VERSION 3.16)
project(flip_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(flip_sim
    test/flip_task_test.cpp
)

target_link_libraries(flip_sim PRIVATE rotem_core)

target_include_directories(flip_sim PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/mocks
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/../controllers
)

if(NOT WIN32)
  target_link_libraries(flip_sim PRIVATE pthread)
endif()
