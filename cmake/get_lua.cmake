set(LUA_VERSION 5.4.2)

CPMAddPackage(
  NAME Lua
  VERSION ${LUA_VERSION}
  # lua has no CMakeLists.txt
  DOWNLOAD_ONLY YES
  GITHUB_REPOSITORY lua/lua
)

if (Lua_ADDED)

  set(LUA_LIB_SRCS 
      "${Lua_SOURCE_DIR}/lapi.c"
      "${Lua_SOURCE_DIR}/lcode.c"
      "${Lua_SOURCE_DIR}/lctype.c"
      "${Lua_SOURCE_DIR}/ldebug.c"
      "${Lua_SOURCE_DIR}/ldo.c"
      "${Lua_SOURCE_DIR}/ldump.c"
      "${Lua_SOURCE_DIR}/lfunc.c"
      "${Lua_SOURCE_DIR}/lgc.c"
      "${Lua_SOURCE_DIR}/llex.c"
      "${Lua_SOURCE_DIR}/lmem.c"
      "${Lua_SOURCE_DIR}/lobject.c"
      "${Lua_SOURCE_DIR}/lopcodes.c"
      "${Lua_SOURCE_DIR}/lparser.c"
      "${Lua_SOURCE_DIR}/lstate.c"
      "${Lua_SOURCE_DIR}/lstring.c"
      "${Lua_SOURCE_DIR}/ltable.c"
      "${Lua_SOURCE_DIR}/ltm.c"
      "${Lua_SOURCE_DIR}/lundump.c"
      "${Lua_SOURCE_DIR}/lvm.c"
      "${Lua_SOURCE_DIR}/lzio.c"
      "${Lua_SOURCE_DIR}/lauxlib.c"
      "${Lua_SOURCE_DIR}/lbaselib.c"
      "${Lua_SOURCE_DIR}/lcorolib.c"
      "${Lua_SOURCE_DIR}/ldblib.c"
      "${Lua_SOURCE_DIR}/liolib.c"
      "${Lua_SOURCE_DIR}/lmathlib.c"
      "${Lua_SOURCE_DIR}/loadlib.c"
      "${Lua_SOURCE_DIR}/loslib.c"
      "${Lua_SOURCE_DIR}/lstrlib.c"
      "${Lua_SOURCE_DIR}/ltablib.c"
      "${Lua_SOURCE_DIR}/lutf8lib.c"
      "${Lua_SOURCE_DIR}/linit.c"
  )

  find_library(LIBM m REQUIRED)
  add_library(LuaForGlue ${LUA_LIB_SRCS})

  # create a new independent library LuaForGlue that is aliased to lua 
  # this allows installing and using LuaGlue without interfering with other installations of lua  
  target_link_libraries(LuaForGlue INTERFACE ${LIBM})
  target_include_directories(LuaForGlue
    PUBLIC
      $<BUILD_INTERFACE:${Lua_SOURCE_DIR}>
      $<INSTALL_INTERFACE:include/LuaForGlue-${LUA_VERSION}
  )

  if(UNIX)
    target_compile_definitions(LuaForGlue PRIVATE LUA_USE_POSIX LUA_USE_POSIX_SPAWN)
  elseif(ANDROID)
    target_compile_definitions(LuaForGlue PRIVATE LUA_USE_POSIX LUA_USE_DLOPEN)
  elseif(IOS)
    target_compile_definitions(LuaForGlue PRIVATE LUA_USE_POSIX_SPAWN LUA_USE_POSIX)
  elseif(EMSCRIPTEN OR WIN32)
  endif()

  packageProject(
    NAME LuaForGlue
    VERSION ${LUA_VERSION}
    BINARY_DIR ${Lua_BINARY_DIR}
    INCLUDE_DIR ${Lua_SOURCE_DIR}
    INCLUDE_DESTINATION include/LuaForGlue-${LUA_VERSION}
  )

  add_library(Lua ALIAS LuaForGlue)
  set(ADDITIONAL_GLUE_DEPENDENCIES "LuaForGlue")
endif()