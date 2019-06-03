CMAKE_MINIMUM_REQUIRED(VERSION 2.7)

set (PROJECT_NAME vsqlite-tests)
PROJECT(${PROJECT_NAME})

file(GLOB HDRS "../include/vsqlite/*.h*" "*.h*")
file(GLOB SRCS "*.cpp")

include_directories(../include )
include_directories(.. ${GTEST_DIR}/include )


add_executable (${PROJECT_NAME} ${SRCS} ${HDRS})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} vsqlite-serialize ${VSQLITE_LIB} ${GTEST_DIR}/lib/libgtest.a)
