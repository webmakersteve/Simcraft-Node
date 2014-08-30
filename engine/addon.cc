#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "simulationcraft.hpp"

using namespace v8;

Handle<Value> Add(const Arguments& args) {
  HandleScope scope;

  sim_t sim;

  std::vector<std::string> array(1);
  array[0] = "armory=us,illidan,john";
  sim_t_response* response = sim.returns( array );
  delete response; // I guess should use RAII?

  return scope.Close(Undefined());
}

void Init(Handle<Object> exports) {
  exports->Set(String::NewSymbol("add"),
      FunctionTemplate::New(Add)->GetFunction());
}

NODE_MODULE(simc, Init)
