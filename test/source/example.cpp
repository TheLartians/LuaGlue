#include <doctest/doctest.h>

// clang-format off
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

// clang-format on

TEST_CASE("Example") {
  CHECK_NOTHROW(exampleBasics());
  CHECK_NOTHROW(exampleModules());
}
