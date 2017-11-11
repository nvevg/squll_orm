cmake_minimum_required(VERSION 3.7)

project(squll_orm_tests)

file(GLOB_RECURSE SOURCES *.cc)

add_executable("${PROJECT_NAME}" "${SOURCES}")

include_directories(../include/)

find_path(SQLITE3_INCLUDE sqlite3.h)
include_directories(BEFORE "${SQLITE3_INCLUDE}")

find_library(SQLITE3_LIB sqlite3)
target_link_libraries("${PROJECT_NAME}" "${SQLITE3_LIB}")

