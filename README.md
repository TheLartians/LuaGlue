[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/MacOS/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Windows/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Ubuntu/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Style/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![Actions Status](https://github.com/TheLartians/LuaGlue/workflows/Install/badge.svg)](https://github.com/TheLartians/LuaGlue/actions)
[![codecov](https://codecov.io/gh/TheLartians/LuaGlue/branch/master/graph/badge.svg)](https://codecov.io/gh/TheLartians/LuaGlue)

# LuaGlue

Lua bindings for [Glue](https://github.com/TheLartians/Glue).

## Usage

### Example

Using LuaGlue you can interact with Lua using a simple binding interface.
The following example illustrates the basic usage.

```cpp
#include <glue/lua/state.h>
#include <iostream>

void exampleBasics() {
  glue::lua::State state;
  state.openStandardLibs();

  // run JS code
  state.run("print('Hello Lua!')");

  // extract values
  std::cout << state.get("'Hello' .. ' ' .. 'C++!'")->get<std::string>() << std::endl;

  // extract maps
  auto map = state.get("({a=1, b='2'})").asMap();
  map["a"]->get<int>(); // -> 1

  // extract functions
  auto f = state.get("function(a,b) return a+b end").asFunction();
  f(3, 4).get<int>(); // -> 7

  // inject values
  auto global = state.root();
  global["x"] = 42;
  global["square"] = [](double x){ return x*x; };
  
  // interact with JS directly
  state.run("print(square(x))");
  // or using Glue
  global["print"](global["square"](global["x"]));
}
```

Classes and inheritance are also supported.

```cpp
#include <glue/class.h>

struct A {
  std::string member;
  A(std::string m): member(m) {}
  auto method() const { return "member: " + member; }
};

void exampleModules() {
  glue::lua::State state;
  state.openStandardLibs();

  // inject C++ classes and APIs into JavaScript
  auto module = glue::createAnyMap();
  
  module["A"] = glue::createClass<A>()
    .addConstructor<std::string>()
    .addMember("member", &A::member)
    .addMethod("method", &A::method)
  ;

  state.addModule(module, state.root());

  state.run("a = A.new('test');");
  state.run("print(a:member());");
  state.run("print(a:method());");
}
```

Check the [API](include/glue/lua/state.h) and [tests](test/source/state.cpp) for functionality and examples.
See [here](https://github.com/TheLartians/TypeScriptXX) for a full example project using automatic TypeScript declarations.

### Adding to your project

LuaGlue can be easily integrated through CPM.
If not available before, this will automatically add a Lua and Glue target as well.

```cmake
CPMAddPackage(
  NAME LuaGlue
  VERSION 1.1.2
  GITHUB_REPOSITORY TheLartians/LuaGlue
)

target_link_libraries(myLibrary LuaGlue)
```

### Build and run tests

To build and run the tests, run the following commands from the project's root.

```bash
cmake -Htest -Bbuild
cmake --build build -j8
node build/LuaGlueTests.js
```
