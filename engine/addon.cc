#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "simulationcraft.hpp"

using namespace v8;
using namespace std;


class SimcraftResponseWrapper
{
  public:
    SimcraftResponseWrapper (sim_t & l, std::vector<std::string> array) : sim(l) {
      std::streambuf* cout_sbuf = std::cout.rdbuf(); // save original sbuf
      std::streambuf* cerr_sbuf = std::cerr.rdbuf(); // save original sbuf
      std::ofstream   fout("/dev/null");
      std::cout.rdbuf(fout.rdbuf()); // redirect 'cout' to a 'fout'
      std::cerr.rdbuf(fout.rdbuf());

      response = sim.returns( array );

      // End muted code

      std::cout.rdbuf(cout_sbuf); // restore the original stream buffer
      std::cerr.rdbuf(cerr_sbuf); // Restore original CERR

    };
    ~SimcraftResponseWrapper () throw () {
      delete response;
    }
    sim_t_response* Response() {
      return response;
    }
  private:
    sim_t& sim;
    sim_t_response* response;
};

Local<Object> CreateReturnObject(sim_t* simulator) {
  Local<Object> returnObj = Object::New();

  double player_dps = simulator->active_player->collected_data.dps.mean();
  Local<Number> dps = Number::New(player_dps);
  returnObj->Set(String::New("dps"), dps);

  return returnObj;

}

Handle<Value> Run(const Arguments& args) {

  HandleScope scope;

  if (args.Length() < 1) {
      return scope.Close(Undefined());
  }

  if (args[0]->IsObject()) {
    Handle<Object> object = Handle<Object>::Cast(args[0]);

    const Local<Array> props = object->GetPropertyNames();
    const uint32_t length = props->Length();

    if (length < 1) {
      // Kill it with an error
    }

    std::vector<std::string> array(length);

    std::string param_key_string;

    for (uint32_t i=0 ; i<length ; ++i)
    {
        const Local<Value> key = props->Get(i);
        const Local<Value> value = object->Get(key);

        const v8::String::Utf8Value param_key(key->ToString());
        const v8::String::Utf8Value param_value(value->ToString());

        param_key_string = std::string(*param_key) + "=" + std::string(*param_value);

        array[i] = param_key_string;

        // Needs to be added to a string vector to form the argument list

    }

    sim_t sim;
    SimcraftResponseWrapper wrapper(sim,array);
    sim_t_response* response(wrapper.Response());

    if (args.Length() > 1) {

      if (args[1]->IsFunction()) {
        Handle<Function> fn = Handle<Function>::Cast(args[1]);
        const unsigned argc = 2;
        // Check if the simulation went through
        std::vector<std::string> errors = response->simulator->error_list;
        if (errors.size() > 0 || response->error != "") {
          // There is an error object in the response too so we can check that
          //throw the error
          const char* error_c_str;

          if (errors.size() > 0) {
            error_c_str = errors[0].c_str();
          } else {
            error_c_str = response->error.c_str();
          }

          Local<Value> argv[argc] = { String::New(error_c_str), Local<Value>::New(Undefined()) };
          fn->Call(Context::GetCurrent()->Global(), argc, argv);
        } else {

          Local<Object> returnObj = CreateReturnObject(response->simulator);

          Local<Value> argv[argc] = { Local<Value>::New(Undefined()), Local<Value>::New(returnObj) };
          fn->Call(Context::GetCurrent()->Global(), argc, argv);
        }

      }

    }

  }

  return scope.Close(Undefined());
}

void Init(Handle<Object> exports) {
  exports->Set(String::NewSymbol("run"),
      FunctionTemplate::New(Run)->GetFunction());
}

NODE_MODULE(simc, Init)
