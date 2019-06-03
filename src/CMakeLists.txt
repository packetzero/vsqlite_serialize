CMAKE_MINIMUM_REQUIRED(VERSION 2.7)

set (PROJECT_NAME vsqlite-serialize)
PROJECT(${PROJECT_NAME})

file(GLOB HDRS "../include/*.h*" "*.h*")
file(GLOB SRCS "*.cpp" )

add_library (${PROJECT_NAME} ${SRCS} ${HDRS})

install(TARGETS vsqlite-serialize ARCHIVE DESTINATION lib)
