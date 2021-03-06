#pragma once

#include <glue/context.h>

struct lua_State;

namespace glue {
  namespace lua {

    struct Data;

    class State {
    private:
      std::shared_ptr<Data> data;
      std::shared_ptr<Map> globalTable;

    public:
      /**
       * Create a new lua state and destroys the state after use.
       */
      State();
      State(lua_State *existing);
      State(const State &other) = delete;

      /**
       * Runs the code from path and returns the returned result as a `Any`.
       */
      Value runFile(const std::string &path) const;

      /**
       * Runs the code and returns the returned result as a `Any`.
       */
      Value run(const std::string_view &code, const std::string &name = "anonymous lua code") const;

      /**
       * Runs the expression and returns the result as a `Any`
       */
      Value get(const std::string &value) const { return run("return " + value); }

      /**
       * Runs the code and returns the result as `T`
       */
      template <class T> T get(const std::string &code) const { return get(code)->get<T>(); }

      /**
       * Runs lua garbage collector
       */
      void collectGarbage() const;

      /**
       * Opens the lua standard libraries
       */
      void openStandardLibs() const;

      /**
       * returns the value map of the global table.
       */
      MapValue root() const;

      /**
       * adds a module to the target map and registers all contained classes
       */
      void addModule(const MapValue &map, const MapValue &target);
      void addModule(const MapValue &map) { return addModule(map, root()); }

      /**
       * returns a pointer to the internal lua state
       */
      lua_State *getRawLuaState() const;

      /**
       * returns a function to delete lua values
       */
      Value getValueDeleter() const;

      ~State();
    };

  }  // namespace lua
}  // namespace glue
