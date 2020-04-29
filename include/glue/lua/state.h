#pragma once

#include <glue/context.h>

namespace glue {
  namespace lua {

    class State {
      private:

      struct Data;
      std::shared_ptr<Data> data;
      
      public:

      State();
      ~State();
    };

  }
}
