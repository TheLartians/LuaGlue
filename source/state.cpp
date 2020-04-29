#include <glue/lua/state.h>

#include <exception>
#define SOL_PRINT_ERRORS 0
#include <sol/sol.hpp>

using namespace glue;

namespace glue {
  namespace lua {
    namespace detail {

      sol::object anyToSol(lua_State *state, const Any &value);
      Any solToAny(sol::object value);

      struct LuaMap final : public revisited::DerivedVisitable<LuaMap, glue::Map> {
        lua_State *state;
        sol::table table;

        LuaMap(lua_State *s, sol::table t) : state(s), table(std::move(t)) {}

        Any get(const std::string &key) const { return solToAny(table[key]); }

        void set(const std::string &key, const Any &value) { table[key] = anyToSol(state, value); }

        bool forEach(const std::function<bool(const std::string &, const Any &)> &callback) const {
          for (auto &&[k, v] : table) {
            if (k.is<std::string>()) {
              callback(k.as<std::string>(), solToAny(v));
            }
          }
          return false;
        }
      };

      struct LuaFunction {
        sol::function data;

        Any operator()(const revisited::AnyArguments &args) const {
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

      sol::object anyToSol(lua_State *state, const Any &value) {
        if (!value) {
          return sol::lua_nil;
        }

        struct Visitor
            : revisited::RecursiveVisitor<const int &, const size_t &, double, bool,
                                          const std::string &, std::string, AnyFunction,
                                          const glue::Map &, const LuaMap &, const LuaFunction &> {
          lua_State *state;
          sol::object result;

          Visitor(lua_State *s) : state(s) {}

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
              revisited::AnyArguments args;
              for (auto &&arg : vArgs) {
                args.push_back(solToAny(sol::object(arg)));
              }
              return anyToSol(vArgs.lua_state(), f.call(args));
            });
            return true;
          }

          bool visit(const LuaMap &v) override {
            result = v.table;
            return true;
          }

          bool visit(const glue::Map &v) override {
            sol::table table(state, sol::create);
            v.forEach([&](auto &&k, auto &&v) {
              table[k] = anyToSol(state, v);
              return false;
            });
            result = table;
            return true;
          }
        } visitor(state);

        if (value.accept(visitor)) {
          return visitor.result;
        } else {
          return sol::make_object(state, value);
        }
      }

    }  // namespace detail

  }  // namespace lua
}  // namespace glue

struct lua::Data {
  sol::state state;
  std::shared_ptr<detail::LuaMap> rootMap;
};

lua::State::State() : data(std::make_shared<Data>()) {
  data->rootMap = std::make_shared<detail::LuaMap>(data->state, data->state.globals());
}

glue::MapValue lua::State::root() const { return MapValue(data->rootMap); }

void lua::State::openStandardLibs() const { data->state.open_libraries(); }

void lua::State::collectGarbage() const { data->state.collect_garbage(); }

Any lua::State::run(const std::string_view &code, const std::string &name) const {
  return detail::solToAny(data->state.script(code, name));
}

lua::State::~State() {}
