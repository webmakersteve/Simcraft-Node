#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "simulationcraft.hpp"

using namespace v8;
using namespace std;

Handle<Value> Run(const Arguments& args) {
  HandleScope scope;

  if (args.Length() < 1) {
      return scope.Close(Undefined());
  }

  String::AsciiValue ascii();
  std::vector<std::string> array(1);

  int argCurrent = 0;

  if (args[0]->IsObject()) {
    Handle<Object> object = Handle<Object>::Cast(args[0]);
    Handle<Value> armory_value = object->Get(String::New("armory"));

    // Value like us,illidan,john
    String::AsciiValue ascii(armory_value);
    std::string arg_armory = std::string("armory=") + *ascii;
    array[argCurrent] = arg_armory;

  } else {
    return scope.Close(Undefined());
  }

  sim_t sim;

  sim_t_response* response (sim.returns( array ));

  double player_dps = response->simulator->active_player->collected_data.dps.mean();
  Local<Number> dps = Number::New(player_dps);

  Local<Object> returnObj = Object::New();
  returnObj->Set(String::New("DPS"), dps);

  if (args.Length() > 1) {

    if (args[1]->IsFunction()) {
      Handle<Function> fn = Handle<Function>::Cast(args[1]);
      const unsigned argc = 1;
      Local<Value> argv[argc] = { Local<Value>::New(returnObj) };
      fn->Call(Context::GetCurrent()->Global(), argc, argv);

    }

  }

  delete response; // I guess should use RAII? Otherwise if we get an exception
  // We get a GIANT problem (huge amounts of leaked memory)

  return scope.Close(Undefined());
}

void Init(Handle<Object> exports) {
  exports->Set(String::NewSymbol("run"),
      FunctionTemplate::New(Run)->GetFunction());
}

NODE_MODULE(simc, Init)
