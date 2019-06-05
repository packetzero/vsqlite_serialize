CMAKE_MINIMUM_REQUIRED(VERSION 2.7)

set (PROJECT_NAME vsqlite-serialize-project)
PROJECT(${PROJECT_NAME})

set(CMAKE_SUPPRESS_REGENERATION true)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF) # want c++11 not gnu++11

set(CMAKE_CXX_RELEASE_FLAGS "-DNDEBUG=1")

# on MacOS with brew:
include_directories(/usr/local/include)
include_directories(${CMAKE_SOURCE_DIR}/deps/dyno/include)

# env

if (DEFINED ENV{VSQLITE_DIR})
  set(VSQLITE_DIR $ENV{VSQLITE_DIR})
  include_directories("${VSQLITE_DIR}/include")
else()
  message("VSQLITE_DIR not set")
endif()

if (DEFINED ENV{RAPIDJSON_DIR})
  set(RAPIDJSON_DIR $ENV{RAPIDJSON_DIR})
  include_directories("${RAPIDJSON_DIR}/include")

else()
  message("RAPIDJSON_DIR not set")
endif()

if (DEFINED ENV{CROW_DIR})
  set(CROW_DIR $ENV{CROW_DIR})
  include_directories("${CROW_DIR}/include")
else()
  message("CROW_DIR not set")
endif()

# find dependencies

FIND_LIBRARY(VSQLITE_LIB vsqlite HINT ${VSQLITE_DIR}/lib )

add_subdirectory(src)
add_subdirectory(benchmarks)

if(DEFINED ENV{GTEST_DIR})
  set(GTEST_DIR $ENV{GTEST_DIR})
  add_subdirectory(tests)
endif()

install(DIRECTORY include DESTINATION . FILES_MATCHING PATTERN "*.h" )
