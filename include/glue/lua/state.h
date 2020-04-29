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
      //    struct Error: public std::exception {
      //      lua_State * L;
      //      unsigned stackSize;
      //      std::shared_ptr<bool> valid;
      //
      //      Error(lua_State * l, unsigned stackSize);
      //      Error();
      //
      //      const char * what() const noexcept override;
      //      ~Error();
      //    };

      /**
       * Create a new lua state and destroys the state after use.
       */
      State();
      State(const State &other) = delete;

      void openStandardLibs() const;

      //    /**
      //     * Runs the code from path and returns the returned result as a `Any`.
      //     */
      //    Any runFile(const std::string &path) const;
      //
      /**
       * Runs the code and returns the returned result as a `Any`.
       */
      Any run(const std::string_view &code, const std::string &name = "anonymous lua code") const;

      /**
       * Runs the expression and returns the result as a `Any`
       */
      Any get(const std::string &value) const { return run("return " + value); }

      /**
       * Runs the code and returns the result as `T`
       */
      template <class T> T get(const std::string &code) const { return get(code).get<T>(); }

      /**
       * Runs lua garbage collector
       */
      void collectGarbage() const;

      /**
       * returns the value map of the global table.
       */
      MapValue root() const;

      ~State();
    };

  }  // namespace lua
}  // namespace glue
