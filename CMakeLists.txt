cmake_minimum_required(VERSION 3.10)
project(mosalloc)

set(CMAKE_CXX_STANDARD 11)

# enable testing
enable_testing()
# Add custom target to run tests verbosely
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

# Define macros required by the code to enable thread-safety code
add_definitions(-DTHREAD_SAFETY)
# Include CompilerFlags cmake file to add extra warning/error compiler flags
include(CompilerFlags.cmake)

set(API_LIBRARY "${PROJECT_NAME}-api")
add_subdirectory(src)
add_subdirectory(test)

# ============== gtest dependency - start ==============
# integrated gtest according to the instructions in:
# https://github.com/google/googletest/tree/master/googletest#incorporating-into-an-existing-cmake-project

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# ============== gtest dependency - end ==============
