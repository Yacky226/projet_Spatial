cmake_minimum_required(VERSION 3.14)
project(antennes_5g)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)
find_package(PostgreSQL REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
   
)

# Trouve récursivement tous les fichiers .cc et .cpp dans src/
file(GLOB_RECURSE SRC_FILES "src/*.cc" "src/*.cpp")

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Drogon::Drogon
    PostgreSQL::PostgreSQL
)