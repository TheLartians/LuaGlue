[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/MacOS/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Windows/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Ubuntu/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Style/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Install/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![codecov](https://codecov.io/gh/TheLartians/LuaGlue/branch/master/graph/badge.svg)](https://codecov.io/gh/TheLartians/LuaGlue)

# LuaGlue

Lua bindings for [glue](https://github.com/TheLartians/Glue).

## Documentation

Yet to be written. Check the [tests](test/source/state.cpp) for functionality and examples.

## Usage

LuaGlue can be easily integrated through CPM.
If not added before, this will add the Glue package as well.

```cmake
CPMAddPackage(
  NAME LuaGlue
  # Until release use a current git tag
  GIT_TAG 98732f7231790b17d5ff7af03371d4086a743b05
  GITHUB_REPOSITORY TheLartians/LuaGlue
)

target_link_libraries(myLibrary LuaGlue)
```
