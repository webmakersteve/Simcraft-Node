/*
 * The GPL License
 */

#include "simulationcraft.h"

namespace simnode {

Persistent<Object> module;

static Handle<Object> CreateTypeObject() {
	
}

extern "C" void
init(Handle<Object> target) {

}

Handle<Object> GetModule() {
	return module;
}

} // namespace simnode

NODE_MODULE(simnode, simnode::init)
