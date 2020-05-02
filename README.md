[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/MacOS/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Windows/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Ubuntu/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Style/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Install/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![codecov](https://codecov.io/gh/TheLartians/LuaGlue/branch/master/graph/badge.svg)](https://codecov.io/gh/TheLartians/LuaGlue)

# LuaGlue

Lua bindings for [Glue](https://github.com/TheLartians/Glue).

## Documentation

Yet to be written. 
Check the [API](include/glue/lua/state.h) and [tests](test/source/state.cpp) for functionality and examples.
For a full example project using TypeScript transpiling to Lua with automatic declarations, see [here](https://github.com/TheLartians/TypeScriptXX).

## Usage

LuaGlue can be easily integrated through CPM.
If not available before, this will automatically add a Lua and Glue target as well.

```cmake
CPMAddPackage(
  NAME LuaGlue
  VERSION 1.0
  GITHUB_REPOSITORY TheLartians/LuaGlue
)

target_link_libraries(myLibrary LuaGlue)
```
