#include <assert.h>
#include <easy_iterator.h>
#include <glue/keys.h>
#include <glue/lua/state.h>
#include <observe/event.h>

#include <exception>
#include <memory>
#include <sstream>

#define SOL_PRINT_ERRORS 0
#define SOL_SAFE_NUMERICS 1
#define SOL_AUTOMAGICAL_TYPES_BY_DEFAULT 0
#include <sol/sol.hpp>

using namespace glue;

namespace glue {
  namespace lua {
    namespace detail {

      using MapCache = std::unordered_map<const glue::Map *, sol::table>;

      sol::object anyToSol(lua_State *state, const Any &value, MapCache *cache = nullptr);
      Any solToAny(sol::object value);

      struct LuaGlueData {
        LuaGlueData() = default;
        LuaGlueData(const LuaGlueData &) = delete;
        Context context;
        observe::Event<> onDestroy;
      };

      struct LuaGlueInstance : public Any {
        sol::main_table classTable;
      };

      LuaGlueData &getLuaGlueData(sol::state_view state) {
        sol::table registry = state.registry();
        constexpr auto key = "LuaGlueData";
        sol::object data = registry[key];
        if (!data) {
          registry[key] = std::make_unique<LuaGlueData>();
          data = registry[key];
        }
        return data.as<LuaGlueData &>();
      }

      struct LuaMap final : public revisited::DerivedVisitable<LuaMap, glue::Map> {
        sol::main_table data;
        observe::Event<>::Observer lifetimeObserver;

        LuaMap(sol::table t) : data(std::move(t)) {
          if (data.lua_state())
            lifetimeObserver = getLuaGlueData(data.lua_state()).onDestroy.createObserver([this]() {
              data = sol::lua_nil;
            });
        }

        LuaMap(const LuaMap &other) : LuaMap(other.data) {}

        Any get(const std::string &key) const {
          auto value = data[key];
          if (value.valid()) {
            return solToAny(data[key]);
          } else {
            return Any();
          }
        }

        void set(const std::string &key, const Any &value) {
          data[key] = anyToSol(data.lua_state(), value);
        }

        bool forEach(const std::function<bool(const std::string &)> &callback) const {
          for (auto &&[k, v] : data) {
            if (k.is<std::string>()) {
              callback(k.as<std::string>());
            }
          }
          return false;
        }
      };

      struct LuaFunction {
        sol::main_function data;
        observe::Event<>::Observer lifetimeObserver;

        LuaFunction(sol::function d) : data(std::move(d)) {
          if (data.lua_state())
            lifetimeObserver = getLuaGlueData(data.lua_state()).onDestroy.createObserver([this]() {
              data = sol::function();
            });
        }

        LuaFunction(const LuaFunction &other) : LuaFunction(other.data) {}

        Any operator()(const AnyArguments &args) const {
          data.push();
          auto state = data.lua_state();
          for (auto &arg : args) {
            anyToSol(state, arg).push(state);
          }
          lua_call(data.lua_state(), int(args.size()), 1);
          return solToAny(sol::stack::pop<sol::object>(state));
        }
      };

      Any solToAny(sol::object value) {
        if (!value.valid()) {
          return Any();
        }

        switch (value.get_type()) {
          case sol::type::none:
          case sol::type::lua_nil:
            return Any();
          case sol::type::boolean:
            return value.as<bool>();
          case sol::type::string:
            return value.as<std::string>();
          case sol::type::number:
            if (value.is<__int64_t>()) {
              return value.as<__int64_t>();
            } else {
              return value.as<double>();
            }
          function_case:
          case sol::type::function: {
            using namespace revisited;
            using FunctionVisitable = DataVisitableWithBasesAndConversions<LuaFunction, TypeList<>,
                                                                           TypeList<AnyFunction>>;
            Any v;
            v.set<FunctionVisitable>(LuaFunction{std::move(value)});
            return v;
          }
          case sol::type::table:
            return LuaMap(value);
          case sol::type::userdata:
          case sol::type::lightuserdata:
            if (value.is<sol::function>()) {
              goto function_case;
            } else if (value.is<Any>()) {
              return value.as<Any>();
            } else {
              return value;
            }
          default:
            return value;
        }
      }

      AnyArguments solArgsToAnyArgs(sol::variadic_args &vArgs) {
        AnyArguments args;
        for (auto &&arg : vArgs) {
          args.push_back(solToAny(sol::object(arg)));
        }
        return args;
      }

      struct AnyToSolVisitor
          : revisited::RecursiveVisitor<
                const __int16_t &, const __uint16_t &, const __int32_t &, const __uint32_t &,
                const __int64_t &, double, bool, const std::string &, std::string, AnyFunction,
                const glue::Map &, const LuaMap &, const LuaFunction &, sol::object> {
        lua_State *state;
        sol::object result;
        MapCache *cache;

        AnyToSolVisitor(lua_State *s, MapCache *c = nullptr) : state(s), cache(c) {}

        bool visit(sol::object v) override {
          result = std::move(v);
          return true;
        }

        bool visit(const __int16_t &v) override {
          result = sol::make_object(state, v);
          return true;
        }

        bool visit(const __uint16_t &v) override {
          result = sol::make_object(state, v);
          return true;
        }

        bool visit(const __int32_t &v) override {
          result = sol::make_object(state, v);
          return true;
        }

        bool visit(const __uint32_t &v) override {
          result = sol::make_object(state, v);
          return true;
        }

        bool visit(const __int64_t &v) override {
          result = sol::make_object(state, v);
          return true;
        }

        bool visit(double v) override {
          result = sol::make_object(state, v);
          return true;
        }

        bool visit(bool v) override {
          result = sol::make_object(state, v);
          return true;
        }

        bool visit(std::string v) override {
          result = sol::make_object(state, std::move(v));
          return true;
        }

        bool visit(const std::string &v) override {
          result = sol::make_object(state, v);
          return true;
        }

        bool visit(const LuaFunction &v) override {
          result = sol::function(state, v.data);
          return true;
        }

        bool visit(AnyFunction f) override {
          result = sol::make_object(state, [f = std::move(f)](sol::variadic_args vArgs) {
            return anyToSol(vArgs.lua_state(), f.call(solArgsToAnyArgs(vArgs)));
          });
          return true;
        }

        bool visit(const LuaMap &v) override {
          result = sol::table(state, v.data);
          return true;
        }

        bool visit(const glue::Map &v) override {
          if (auto it = cache ? easy_iterator::find(*cache, &v) : nullptr) {
            result = it->second;
          } else {
            sol::table table(state, sol::create);

            if (auto classInfo = v.get(keys::classKey)) {
              table[keys::classKey] = anyToSol(state, classInfo, cache);
              auto &data = detail::getLuaGlueData(state);
              data.context.addRootMap(glue::MapValue(std::make_shared<LuaMap>(table)));
            }

            v.forEach([&](auto &&k) {
              table[k] = anyToSol(state, v.get(k), cache);
              return false;
            });

            auto extends = table[keys::extendsKey];
            if (extends.valid()) {
              sol::table metatable(state, sol::new_table(1));
              metatable[sol::meta_function::index] = extends;
              table[sol::metatable_key] = std::move(metatable);
            }

            if (cache) {
              (*cache)[&v] = table;
            }

            result = std::move(table);
          }
          return true;
        }
      };

      sol::object anyToSol(lua_State *state, const Any &value, MapCache *cache) {
        if (!value) {
          return sol::lua_nil;
        }

        AnyToSolVisitor visitor(state, cache);
        if (value.accept(visitor)) {
          return visitor.result;
        } else {
          auto &data = getLuaGlueData(state);
          auto instance = data.context.createInstance(value);
          if (instance) {
            auto &luaTable = revisited::visitor_cast<LuaMap &>(**instance.classMap);
            return sol::make_object(state,
                                    LuaGlueInstance{std::move(instance.data), luaTable.data});
          } else {
            return sol::make_object(state, value);
          }
        }
      }

    }  // namespace detail
  }    // namespace lua
}  // namespace glue

struct lua::Data {
  std::unique_ptr<sol::state> owned;
  sol::state_view state;
  std::shared_ptr<detail::LuaMap> rootMap;

  void init() { rootMap = std::make_shared<detail::LuaMap>(state.globals()); }

  Data(lua_State *existing) : state(existing) { init(); }
  Data() : owned(std::make_unique<sol::state>()), state(*owned) { init(); }
  ~Data() {
    if (owned) detail::getLuaGlueData(*owned).onDestroy.emit();
  }
};

lua::State::State() : data(std::make_shared<Data>()) {
  using Instance = detail::LuaGlueInstance;

  // clang-format off
  data->state.new_usertype<Any>("Any", 
    sol::meta_function::to_string, +[](const Any &value) {
      std::stringstream stream;
      stream << value.type().name << '(' << &value << ')';
      return stream.str();
    }
  );

  auto forwardBinaryMetaMethodWithDefault = [](auto  glueName, auto defaultOp){
    return [glueName, defaultOp](sol::this_state state, const Instance &value, const Instance &other) -> sol::object {
      auto metamethod = value.classTable[glueName];
      if (metamethod.valid()) {
        return metamethod(detail::anyToSol(state, value), detail::anyToSol(state, other));
      } else {
        return sol::make_object(state, defaultOp(value, other));
      }
    };
  };

  auto forwardBinaryMetaMethod = [](auto  glueName){
    return [glueName](sol::object value, sol::object other) -> sol::object {
      auto &instance = value.as<Instance &>();
      auto metamethod = instance.classTable[glueName];
      if (metamethod.valid()) {
        return metamethod(value, other);
      } else {
        throw std::runtime_error("used unsupported binary operator");
      }
    };
  };

  data->state.new_usertype<Instance>("LuaGlueInstance",
    sol::base_classes, sol::bases<Any>(),
    sol::meta_function::index, +[](const Instance &value, sol::object key) -> sol::object { 
      return value.classTable[key];
    },
    sol::meta_function::equal_to, forwardBinaryMetaMethodWithDefault(keys::operators::eq, [](auto && a, auto && b){ return &a == &b; }),
    sol::meta_function::unary_minus, forwardBinaryMetaMethod(keys::operators::unm),
    sol::meta_function::addition, forwardBinaryMetaMethod(keys::operators::add),
    sol::meta_function::subtraction, forwardBinaryMetaMethod(keys::operators::sub),
    sol::meta_function::multiplication, forwardBinaryMetaMethod(keys::operators::mul),
    sol::meta_function::division, forwardBinaryMetaMethod(keys::operators::div),
    sol::meta_function::power_of, forwardBinaryMetaMethod(keys::operators::pow),
    sol::meta_function::less_than, forwardBinaryMetaMethod(keys::operators::lt),
    sol::meta_function::less_than_or_equal_to, forwardBinaryMetaMethod(keys::operators::le),
    sol::meta_function::floor_division, forwardBinaryMetaMethod(keys::operators::idiv),
    sol::meta_function::modulus, forwardBinaryMetaMethod(keys::operators::mod),
    sol::meta_function::to_string, +[](sol::this_state state,const Instance &value) -> sol::object {
      auto toString = value.classTable[keys::operators::tostring];
      if (toString.valid()) {
        return value.classTable[keys::operators::tostring](detail::anyToSol(state, value));
      } else {
        std::stringstream stream;
        stream << value.type().name << '(' << &value << ')';
        return sol::make_object(state, stream.str());
      }
    }
  );
  // clang-format on
}

lua::State::State(lua_State *existing) : data(std::make_shared<Data>(existing)) {}

glue::MapValue lua::State::root() const { return MapValue(data->rootMap); }

void lua::State::openStandardLibs() const { data->state.open_libraries(); }

void lua::State::collectGarbage() const { data->state.collect_garbage(); }

Value lua::State::run(const std::string_view &code, const std::string &name) const {
  return detail::solToAny(data->state.script(code, name));
}

Value lua::State::runFile(const std::string &path) const {
  return detail::solToAny(data->state.script_file(path));
}

lua_State *lua::State::getRawLuaState() const { return data->state.lua_state(); }

lua::State::~State() {}

void lua::State::addModule(const MapValue &map, const MapValue &r) {
  // use cache to not create copies of referenced tables
  detail::MapCache cache;

  // first pass: convert types only
  glue::Context context;
  context.addRootMap(map);
  for (auto &&id : context.uniqueTypes) {
    auto &&type = context.types[id.index];
    detail::anyToSol(data->state, type.data.data, &cache);
  }

  // second pass: convert all values
  auto convertedMap
      = Value(detail::solToAny(detail::anyToSol(data->state, map.data, &cache))).asMap();
  assert(convertedMap);

  convertedMap.forEach([&](auto &&key, auto &&value) {
    r[key] = value;
    return false;
  });
}

Value lua::State::getValueDeleter() const {
  return detail::solToAny(sol::make_object(data->state, [](Any &value) { value.reset(); }));
}
