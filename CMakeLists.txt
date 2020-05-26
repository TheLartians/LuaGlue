cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# ---- Project ----

project(LuaGlue
  VERSION 1.2.1
  LANGUAGES CXX C
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

# ---- Add dependencies via CPM ----

include(cmake/CPM.cmake)

CPMAddPackage(
  NAME Glue
  VERSION 1.5
  GITHUB_REPOSITORY TheLartians/Glue
)

if (NOT TARGET Lua)
  CPMAddPackage(
    NAME Lua
    GITHUB_REPOSITORY lua/lua
    VERSION 5.3.5
    # lua has no CMakeLists.txt
    DOWNLOAD_ONLY YES
  )

  if (Lua_ADDED)
    # we have just downloaded the lua source as lua isn't installed locally
    # create a new independent library LuaForGlue that is aliased to lua 
    # this allows installing and using LuaGlue without interfering with other installations of lua  
    FILE(GLOB lua_sources ${Lua_SOURCE_DIR}/*.c)
    FILE(GLOB lua_headers ${Lua_SOURCE_DIR}/*.h)
    add_library(LuaForGlue ${lua_sources} ${lua_headers})
    set_target_properties(LuaForGlue PROPERTIES LINKER_LANGUAGE C)

    target_include_directories(LuaForGlue
      PUBLIC
        $<BUILD_INTERFACE:${Lua_SOURCE_DIR}>
    )

    if(ANDROID)
    target_compile_definitions(LuaForGlue PRIVATE LUA_USE_POSIX LUA_USE_DLOPEN)
    elseif(EMSCRIPTEN OR WIN32)
    elseif(UNIX OR APPLE)
      target_compile_definitions(LuaForGlue PRIVATE LUA_USE_POSIX_SPAWN LUA_USE_POSIX)
    endif()

    packageProject(
      NAME LuaForGlue
      VERSION 5.3.5
      BINARY_DIR ${Lua_BINARY_DIR}
      INCLUDE_DIR ${Lua_SOURCE_DIR}
      INCLUDE_DESTINATION include/LuaForGlue-5.3.5
    )

    add_library(Lua ALIAS LuaForGlue)
    set(ADDITIONAL_GLUE_DEPENDENCIES "LuaForGlue")
  endif()
else()
  set(ADDITIONAL_GLUE_DEPENDENCIES "Lua")
endif()

CPMAddPackage(
  NAME sol2
  URL https://github.com/ThePhD/sol2/archive/v3.2.0.zip
  VERSION 3.2.0
  DOWNLOAD_ONLY YES
)

CPMAddPackage(
  NAME EasyIterator
  VERSION 1.4
  GIT_REPOSITORY https://github.com/TheLartians/EasyIterator.git
)

CPMAddPackage(
  NAME Observe
  VERSION 3.2
  GITHUB_REPOSITORY TheLartians/Observe
)

# ---- Add source files ----

FILE(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
FILE(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

add_library(LuaGlue ${headers} ${sources})
set_target_properties(LuaGlue PROPERTIES CXX_STANDARD 17)

# beeing a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(LuaGlue PUBLIC "$<$<BOOL:${MSVC}>:/permissive->")

target_link_libraries(LuaGlue PRIVATE Lua EasyIterator Observe)
target_include_directories(LuaGlue SYSTEM PRIVATE ${sol2_SOURCE_DIR}/include)
target_link_libraries(LuaGlue PUBLIC Glue)

target_include_directories(LuaGlue
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Create an installable target ----

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  DEPENDENCIES Glue Observe ${ADDITIONAL_GLUE_DEPENDENCIES}
)
