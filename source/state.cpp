#include <assert.h>
#include <easy_iterator.h>
#include <glue/keys.h>
#include <glue/lua/state.h>

#include <exception>
#include <sstream>
#define SOL_PRINT_ERRORS 0
#include <sol/sol.hpp>

using namespace glue;

namespace glue {
  namespace lua {
    namespace detail {

      using MapCache = std::unordered_map<const glue::Map *, sol::table>;

      sol::object anyToSol(lua_State *state, const Any &value, MapCache *cache = nullptr);
      Any solToAny(sol::object value);

      struct LuaGlueData {
        Context context;
      };

      struct LuaGlueInstance : public Any {
        sol::table classTable;
      };

      LuaGlueData &getLuaGlueData(sol::state_view state) {
        sol::table registry = state.registry();
        constexpr auto key = "LuaGlueData";
        sol::object data = registry[key];
        if (!data) {
          registry[key] = LuaGlueData();
          data = registry[key];
        }
        return data.as<LuaGlueData &>();
      }

      struct LuaMap final : public revisited::DerivedVisitable<LuaMap, glue::Map> {
        lua_State *state;
        sol::table table;

        LuaMap(lua_State *s, sol::table t) : state(s), table(std::move(t)) {}

        Any get(const std::string &key) const { return solToAny(table[key]); }

        void set(const std::string &key, const Any &value) { table[key] = anyToSol(state, value); }

        bool forEach(const std::function<bool(const std::string &)> &callback) const {
          for (auto &&[k, v] : table) {
            if (k.is<std::string>()) {
              callback(k.as<std::string>());
            }
          }
          return false;
        }
      };

      struct LuaFunction {
        sol::function data;

        Any operator()(const AnyArguments &args) const {
          data.push();
          for (auto &arg : args) {
            anyToSol(data.lua_state(), arg).push();
          }
          lua_call(data.lua_state(), int(args.size()), 1);
          return solToAny(sol::stack::pop<sol::object>(data.lua_state()));
        }
      };

      Any solToAny(sol::object value) {
        switch (value.get_type()) {
          case sol::type::none:
          case sol::type::lua_nil:
            return Any();
          case sol::type::boolean:
            return value.as<bool>();
          case sol::type::string:
            return value.as<std::string>();
          case sol::type::number:
            return value.as<double>();
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
            return LuaMap(value.lua_state(), value);
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
                const int &, const size_t &, double, bool, const std::string &, std::string,
                AnyFunction, const glue::Map &, const LuaMap &, const LuaFunction &, sol::object> {
        lua_State *state;
        sol::object result;
        MapCache *cache;

        AnyToSolVisitor(lua_State *s, MapCache *c = nullptr) : state(s), cache(c) {}

        bool visit(sol::object v) override {
          result = std::move(v);
          return true;
        }

        bool visit(const int &v) override {
          result = sol::make_object(state, v);
          return true;
        }

        bool visit(const size_t &v) override {
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
          result = v.data;
          return true;
        }

        bool visit(AnyFunction f) override {
          result = sol::make_object(state, [f = std::move(f)](sol::variadic_args vArgs) {
            return anyToSol(vArgs.lua_state(), f.call(solArgsToAnyArgs(vArgs)));
          });
          return true;
        }

        bool visit(const LuaMap &v) override {
          result = v.table;
          return true;
        }

        bool visit(const glue::Map &v) override {
          if (auto it = cache ? easy_iterator::find(*cache, &v) : nullptr) {
            result = it->second;
          } else {
            sol::table table(state, sol::create);
            v.forEach([&](auto &&k) {
              table[k] = anyToSol(state, v.get(k), cache);
              return false;
            });
            if (cache) {
              (*cache)[&v] = table;
            }
            result = table;
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
                                    LuaGlueInstance{std::move(instance.data), luaTable.table});
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

  void init() { rootMap = std::make_shared<detail::LuaMap>(state, state.globals()); }

  Data(lua_State *existing) : state(existing) { init(); }
  Data() : owned(std::make_unique<sol::state>()), state(*owned) { init(); }
};

lua::State::State() : data(std::make_shared<Data>()) {
  using Instance = detail::LuaGlueInstance;

  // clang-format off
  data->state.new_usertype<Any>("Any", sol::meta_function::to_string, [](const Any &value) {
    std::stringstream stream;
    stream << value.type().name << '(' << &value << ')';
    return stream.str();
  });

  data->state.new_usertype<Instance>(
      "LuaGlueInstance", sol::meta_function::index,
      [](const Instance &value, sol::object key) -> sol::object { return value.classTable[key]; },
      sol::base_classes, sol::bases<Any>());
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
  auto convertedMap
      = Value(detail::solToAny(detail::anyToSol(data->state, map.data, &cache))).asMap();
  assert(convertedMap);

  auto &luaGlueData = detail::getLuaGlueData(data->state);
  luaGlueData.context.addRootMap(convertedMap);

  for (auto &&id : luaGlueData.context.uniqueTypes) {
    auto &&type = luaGlueData.context.types[id.index];
    auto &&typeMap = type.data;
    auto table = revisited::visitor_cast<detail::LuaMap &>(*typeMap.data).table;
    if (auto extends = typeMap[keys::extendsKey]) {
      sol::table metatable(data->state, sol::new_table(1));
      metatable[sol::meta_function::index] = detail::anyToSol(data->state, *extends);
      table[sol::metatable_key] = metatable;
    }
  }

  convertedMap.forEach([&](auto &&key, auto &&value) {
    r[key] = value;
    return false;
  });
}

Value lua::State::getValueDeleter() const {
  return sol::make_object(data->state, [](Any &value) { value.reset(); });
  // return detail::solToAny(sol::make_object(data->state,[](sol::object value){
  //   sol::table t = value;
  //   t["__glue_delete"](value);
  // })).get<AnyFunction>();
}
