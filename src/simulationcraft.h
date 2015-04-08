#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#ifndef SIMNODE_H
#define SIMNODE_H

#include <config.hpp>
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
using std::vector;

struct sim_t_response;
struct sim_signal_handler_t;

struct simnode_t : sim_t {

  sim_t_response* returns( std::vector<std::string>& args );

};

struct sim_t_response {
  int iterations;
  float max_time;
  float combat_length;
  int optimal_raid;
  std::string fight_style;
  std::string error;
  std::string sc_version;
  std::string wow_version;
  std::string wow_ptr_version;
  std::string build_level;
  int canceled;

	simnode_t *simulator;

};

namespace simnode {

	Handle<Object> GetModule();

}

#endif // SIMNODE_H
