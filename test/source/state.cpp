#include <doctest/doctest.h>
#include <glue/class.h>
#include <glue/enum.h>
#include <glue/lua/state.h>

#include <exception>
#include <string>

TEST_CASE("Run script and get result") {
  glue::lua::State state;
  state.openStandardLibs();
  CHECK_NOTHROW(state.run("assert(1==1)"));
  CHECK_THROWS_AS(state.run("assert(1==0)"), std::runtime_error);
  CHECK_THROWS_AS(state.run("f(syntax error]"), std::runtime_error);
  CHECK(state.get<bool>("true") == true);
  CHECK(state.get<bool>("false") == false);
  CHECK(state.get<int>("2+3") == 5);
  CHECK(state.get<std::string>("'a' .. 'b'") == "ab");
  CHECK(state.get<glue::AnyFunction>("function(a,b) return a+b end")(2, 3).get<int>() == 5);

  SUBCASE("regular map") {
    auto map = glue::MapValue(state.get<std::shared_ptr<glue::Map>>("{a=1,b=2}"));
    REQUIRE(map);
    CHECK(map.keys().size() == 2);
    CHECK(map["a"]->get<int>() == 1);
    CHECK(map["b"]->get<int>() == 2);
  }

  SUBCASE("map with non-string keys") {
    auto map = glue::MapValue(state.get<std::shared_ptr<glue::Map>>("{a=1,[1]=2}"));
    REQUIRE(map);
    CHECK(map.keys().size() == 1);
    CHECK(map["a"]->get<int>() == 1);
  }
}

TEST_CASE("Mapped Values") {
  glue::lua::State state;
  glue::MapValue root = state.root();

  SUBCASE("primitives") {
    CHECK_NOTHROW(root["x"] = 42);
    CHECK(root["x"]->get<int>() == 42);
    CHECK(state.get<int>("x") == 42);
    CHECK_NOTHROW(root["y"] = "x");
    CHECK(root["y"]->get<std::string>() == "x");
    CHECK(state.get<std::string>("y") == "x");
    CHECK_NOTHROW(root["z"] = true);
    CHECK(root["z"]->get<bool>() == true);
    CHECK_NOTHROW(root["x"] = size_t(128));
    CHECK(root["x"]->get<size_t>() == 128);
  }

  SUBCASE("set callbacks") {
    SUBCASE("defined arguments") {
      CHECK_NOTHROW(root["f"] = [](int x) { return 42 + x; });
    }
    SUBCASE("any arguments") {
      CHECK_NOTHROW(root["f"] = [](const glue::AnyArguments &args) -> glue::Any {
        REQUIRE(args.size() == 1);
        return 42 + args[0].get<int>();
      });
    }
    REQUIRE(root["f"].asFunction());
    CHECK(root["f"].asFunction()(3).get<int>() == 45);
    CHECK(state.get<int>("f(10)") == 52);
  }

  SUBCASE("extract callbacks") {
    CHECK_NOTHROW(state.run("function g(a,b) return a+b end"));
    CHECK(root["g"].asFunction()(2, 3).get<int>() == 5);
  }

  SUBCASE("any") {
    static thread_local int counter = 0;
    struct A {
      int value;
      A(int v) : value(v) { counter++; }
      A(A &&o) : value(o.value) { counter++; }
      A(const A &) = delete;
      ~A() { counter--; }
    };

    SUBCASE("create and get values") {
      CHECK_NOTHROW(root["a"] = A{43});
      CHECK(root["a"]->get<const A &>().value == 43);
      CHECK_NOTHROW(root["a"]->get<A &>().value = 3);
      CHECK(root["a"]->get<const A &>().value == 3);
      CHECK(counter == 1);
      root["a"] = glue::Any();
      state.collectGarbage();
      CHECK(counter == 0);
    }

    SUBCASE("to string") {
      state.openStandardLibs();
      CHECK_NOTHROW(root["a"] = A{0});
      CHECK_NOTHROW(state.run("assert(string.find(tostring(a), 'A') ~= nil)"));
    }

    SUBCASE("delete values") {
      state.root()["delete"] = state.getValueDeleter();
      CHECK(counter == 0);
      CHECK_NOTHROW(root["a"] = A{43});
      CHECK(counter == 1);
      CHECK_NOTHROW(state.run("delete(a);"));
      CHECK(counter == 0);
    }
  }

  SUBCASE("standard library") {
    state.openStandardLibs();
    CHECK(root["tostring"].asFunction()(42).get<std::string>() == "42");
  }

  SUBCASE("maps") {
    auto map = glue::createAnyMap();
    map["a"] = 44;
    map["b"] = [](int a, int b) { return a + b; };
    CHECK_NOTHROW(root["map"] = map);
    REQUIRE(root["map"].asMap());
    CHECK(root["map"].asMap().keys().size() == 2);
    CHECK(root["map"].asMap()["a"]->get<int>() == 44);
    CHECK(root["map"].asMap()["b"].asFunction()(26, 19).get<int>() == 45);
  }
}

TEST_CASE("Passthrough arguments") {
  glue::lua::State state;
  state.openStandardLibs();
  glue::MapValue root = state.root();

  root["f"] = [](glue::Any v) { return v; };
  CHECK(root["f"].asFunction()(46).get<int>() == 46);
  CHECK(root["f"].asFunction()("hello").get<std::string>() == "hello");
  CHECK_NOTHROW(state.run("assert(f(47) == 47)"));
  CHECK_NOTHROW(state.run("assert(f(1.141) == 1.141)"));
  CHECK_NOTHROW(state.run("assert(f('test') == 'test')"));
  CHECK_NOTHROW(state.run("x = {a=1,b=2}; assert(f(x) == x)"));
  CHECK_NOTHROW(state.run("x = {1,2}; assert(f(x) == x)"));
  CHECK_NOTHROW(state.run("x = function() end; assert(f(x) == x)"));
  CHECK_NOTHROW(state.run("x = nil; assert(f(x) == x)"));
}

TEST_CASE("Run file") {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
  static const std::string slash = "\\";
#else
  static const std::string slash = "/";
#endif
  std::string sourcePath = __FILE__;
  std::string dirPath = sourcePath.substr(0, sourcePath.rfind(slash));
  glue::lua::State state;
  CHECK(state.runFile(dirPath + slash + "test.lua")->get<std::string>() == "Hello Lua!");
  CHECK_THROWS_AS(state.runFile("this file does not exist"), std::runtime_error);
}

TEST_CASE("Share state") {
  glue::lua::State state;
  state.root()["x"] = 50;
  glue::lua::State state2(state.getRawLuaState());
  CHECK(state2.root()["x"]->get<int>() == 50);
  state2.root()["x"] = 42;
  CHECK(state.root()["x"]->get<int>() == 42);
}

TEST_CASE("Modules") {
  struct A {
    std::string member;
  };

  struct B : public A {
    B(std::string value) : A{value} {}
    int method(int v) { return int(member.size()) + v; }
  };

  enum class E : int { V1, V2, V3 };

  auto module = glue::createAnyMap();
  auto inner = glue::createAnyMap();

  inner["A"] = glue::createClass<A>().addConstructor<>().addMember("member", &A::member);

  module["inner"] = inner;

  module["B"] = glue::createClass<B>(glue::WithBases<A>())
                    .addConstructor<std::string>()
                    .setExtends(inner["A"])
                    .addMethod("method", &B::method)
                    .addMethod(glue::keys::operators::eq,
                               [](const B &a, const B &b) { return a.member == b.member; })
                    .addMethod(glue::keys::operators::add,
                               [](const B &a, const B &b) { return B(a.member + b.member); })
                    .addMethod(glue::keys::operators::mul,
                               [](const B &a, int o) { return a.member.size() * o; })
                    .addMethod(glue::keys::operators::tostring,
                               [](const B &b) { return "B(" + b.member + ")"; });

  module["createB"] = []() { return B("unnamed"); };

  module["E"] = glue::createEnum<E>()
                    .addValue("V1", E::V1)
                    .addValue("V2", E::V2)
                    .addValue("V3", E::V3)
                    .addValue("V4", E::V1);

  glue::lua::State state;
  state.openStandardLibs();
  state.addModule(module);

  SUBCASE("methods") {
    CHECK(state.run("local a = inner.A.__new(); a:setMember('testA'); return a:member()")
              ->as<std::string>()
          == "testA");
    CHECK(state.run("local b = B.__new('testB'); return b:member()")->as<std::string>() == "testB");
    CHECK(state.run("local b = createB(); return b:member()")->as<std::string>() == "unnamed");
  }

  SUBCASE("operators") {
    CHECK_NOTHROW(state.run("local a = inner.A.__new(); return tostring(a)")->get<std::string>());
    CHECK(state.run("local b = createB(); return tostring(b)")->as<std::string>() == "B(unnamed)");

    CHECK_NOTHROW(state.run("local a = inner.A.__new(); assert(a == a);"));
    CHECK_NOTHROW(
        state.run("local a = inner.A.__new(); local b = inner.A.__new(); assert(not (a == b));"));
    CHECK_NOTHROW(state.run("local a = B.__new('A'); local b = B.__new('A'); assert(a == b);"));
    CHECK_NOTHROW(
        state.run("local a = B.__new('A'); local b = B.__new('B'); assert(not (a == b));"));
    CHECK_THROWS(
        state.run("local a = inner.A__new('A'); local b = inner.A__new('B'); return a + b"));
    CHECK_NOTHROW(state.run(
        "local a = B.__new('A'); local b = B.__new('B'); assert(a + b == B.__new('AB'));"));
    CHECK_NOTHROW(state.run("local a = B.__new('four'); assert(a*2 == 8);"));
    CHECK_THROWS(state.run("local a = B.__new('four'); assert(a/a);"));
  }

  SUBCASE("enums") {
    CHECK(state.run("return E.V1")->as<E>() == E::V1);
    CHECK_NOTHROW(state.run("assert(E.V1 == E.V1)"));
    CHECK_NOTHROW(state.run("assert(E.V1 == E.V4)"));
    CHECK_NOTHROW(state.run("assert(E.V1 ~= E.V2)"));
    CHECK(state.run("return E.V1:value()")->as<int>() == int(E::V1));
  }
}

TEST_CASE("Lua lifetime") {
  glue::AnyFunction f;
  glue::MapValue m;
  {
    glue::lua::State state;
    f = state.get("function() end").asFunction();
    m = state.get("{a=1, b=2}").asMap();
  }
  // sanitizer will complain unless objects are safely destroyed
}