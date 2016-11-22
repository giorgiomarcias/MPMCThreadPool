cmake_minimum_required(VERSION 3.3)

project(MPMCThreadPool_example)

option(ATTACH_SOURCES "When generating an IDE project, add MPMCThreadPool header files to project sources." ON)


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/lib_MPMCThreadPool)
include(${CMAKE_CURRENT_SOURCE_DIR}/../sources_properties.cmake)

set(src
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
add_executable(${PROJECT_NAME} ${src})

target_link_libraries(${PROJECT_NAME} MPMCThreadPool)
set_MPMCThreadPool_source_files_properties()
