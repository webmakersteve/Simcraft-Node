/*
 * The GPL License
 */

#include "simulationcraft.h"

class SimcraftResponseWrapper
{
  public:
    SimcraftResponseWrapper() {}
    SimcraftResponseWrapper ( vector<string> array) {};
    ~SimcraftResponseWrapper () throw () {
      //delete response;
    }
    void SetResponse() {

      simnode_t sim;
      response = sim.returns( array );

    };
    void SetResponse(vector<string> a) {
      array = a;
      simnode_t sim;
      response = sim.returns( a );

    };
    sim_t_response* Response() {
      if (!response)
        SetResponse();
      return response;
    };

  private:
    std::vector<std::string> array;
    sim_t_response* response;
};

struct Baton {
    SimcraftResponseWrapper sim;
    uv_work_t request;
    v8::Persistent<v8::Function> callback;
    int error_code;
    std::string error_message;

    // Custom data
    int32_t result;

};


Local<Object> CreateReturnObject(simnode_t* simulator) {
  Local<Object> returnObj = Object::New();

  double player_dps = simulator->active_player->collected_data.dps.mean();
  Local<Number> dps = Number::New(player_dps);
  returnObj->Set(String::New("dps"), dps);

  return returnObj;

}

void AsyncWork(uv_work_t* req) {

    Baton* baton = static_cast<Baton*>(req->data);

    SimcraftResponseWrapper wrapper = baton->sim;

    wrapper.SetResponse(); // Does the work here

}

void AsyncAfter(uv_work_t *req, int status) {
    HandleScope scope;

    Baton* baton = static_cast<Baton*>(req->data);

    SimcraftResponseWrapper wrapper = baton->sim;
    sim_t_response* response = wrapper.Response();

    /*Exception::TypeError(
        String::NewFromUtf8(isolate, "Wrong number of arguments")) */

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

      Local<Value> argv[argc] = { Exception::Error(String::New(error_c_str)), Local<Value>::New(Undefined()) };

      TryCatch try_catch;
      baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
      if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
      }

    } else {

      Local<Object> returnObj = CreateReturnObject(response->simulator);

      Local<Value> argv[argc] = { Local<Value>::New(Undefined()), Local<Value>::New(returnObj) };

      TryCatch try_catch;
      baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
      if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
      }

    }

    // baton->callback->Dispose(); I dont understand it seems the spec says there is
    delete baton;

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
      return scope.Close(Undefined());
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

    if (args.Length() > 1 && args[1]->IsFunction()) {

      Local<Function> callback = Local<Function>::Cast(args[1]);
      Persistent<Function> fn = Persistent<Function>::New(callback);

      SimcraftResponseWrapper wrapper(array);
      wrapper.SetResponse(array);

      Baton* baton = new Baton();
      baton->request.data = baton;
      baton->callback = fn;

      uv_queue_work(uv_default_loop(), &baton->request,
        AsyncWork, AsyncAfter);

    }

  }

  return scope.Close(Undefined());
}

void InitNodeAddon(Handle<Object> exports) {
  exports->Set(String::NewSymbol("run"),
      FunctionTemplate::New(Run)->GetFunction());
  exports->Set(String::NewSymbol("version"),
      String::New(SC_VERSION));
}

NODE_MODULE(simcraft, InitNodeAddon)
