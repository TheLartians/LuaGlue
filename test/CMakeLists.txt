cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(LuaGlueTests LANGUAGES CXX C)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

cpmaddpackage("gh:doctest/doctest@2.4.9")
cpmaddpackage("gh:TheLartians/Format.cmake@1.7.3")

if(TEST_INSTALLED_VERSION)
  find_package(LuaGlue REQUIRED)
else()
  cpmaddpackage(NAME LuaGlue SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(LuaGlueTests ${sources})
target_link_libraries(LuaGlueTests doctest LuaGlue)

set_target_properties(LuaGlueTests PROPERTIES CXX_STANDARD 17)

if(NOT TEST_INSTALLED_VERSION)
  target_compile_definitions(LuaGlue PRIVATE -DSOL_ALL_SAFETIES_ON=1)
endif()

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES
                                              "GNU")
    target_compile_options(LuaGlue PUBLIC -Wall -pedantic -Wextra -Werror)
  elseif(MSVC)
    # sol related warnings are disabled as we have no control over the library
    target_compile_options(LuaGlue PUBLIC /W4 /WX /wd4996 /wd4100)
    target_compile_definitions(LuaGlueTests
                               PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endif()

# ---- Add LuaGlueTests ----

enable_testing()

# Note: doctest and similar testing frameworks can automatically configure CMake
# tests For other testing frameworks add the tests target instead:
# ADD_TEST(LuaGlueTests LuaGlueTests)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(LuaGlueTests)

# ---- code coverage ----

if(ENABLE_TEST_COVERAGE)
  target_compile_options(LuaGlue PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(LuaGlue PUBLIC -fprofile-arcs -ftest-coverage)
endif()
