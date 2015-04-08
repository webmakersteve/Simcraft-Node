#ifndef SIMNODE_H
#define SIMNODE_H

#include <v8.h>
#include "cvv8/convert.hpp"
#include <node.h>
#include <simulationcraft.hpp>
#include <node_object_wrap.h>
#include <node_buffer.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

using namespace v8;
using namespace node;
using namespace cvv8;
using std::string;

namespace simnode {
	Handle<Object> GetModule();

}

#endif // SIMNODE_H
