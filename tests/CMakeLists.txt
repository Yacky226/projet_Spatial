cmake_minimum_required(VERSION 3.10)
project(antennes_5g)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_executable(antennes_5g src/main.cpp src/controllers/ObstacleController.cc src/services/ObstacleService.cc src/utils/Validator.cc src/utils/ErrorHandler.cc)

enable_testing()
add_subdirectory(tests)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

# Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Link Google Test to the test executable
add_executable(ObstacleControllerTest tests/ObstacleControllerTest.cc)
target_link_libraries(ObstacleControllerTest ${GTEST_LIBRARIES} pthread)