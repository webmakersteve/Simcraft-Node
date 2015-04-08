#include "simulationcraft.h"

sim_t_response * simnode_t::returns( std::vector<std::string>& args) {

  sim_signal_handler_t handler( this );

  cache_initializer_t cache_init( ( get_cache_directory() + "/simc_cache.dat" ).c_str() );
  dbc_initializer_t dbc_init;
  module_t::init();

  sim_control_t control;

  sim_t_response *response = new sim_t_response;
  int cancelled = 0;

  if ( ! control.options.parse_args( args ) )
  {
      response->error = "ERROR! Incorrect option format..\n";
      cancelled = 1;
  }
  else if ( ! setup( &control ) )
  {
      response->error =  "ERROR! Setup failure...\n";
      cancelled = 1;
  }

  if (cancelled) {
      response->simulator = this;
      return response;
  }

  response->sc_version = SC_VERSION;
  response->wow_version = dbc.wow_version();
  response->wow_ptr_version = dbc.wow_ptr_status();
  response->build_level = util::to_string( dbc.build_level() ).c_str();

  if ( spell_query )
  {
      spell_query -> evaluate();
      report::print_spell_query( this, spell_query_level );
  } else if ( need_to_save_profiles( this ) )
  {
      init();
  }
  else
  {
      if ( max_time <= timespan_t::zero() )
      {
          response->error = "simulationcraft: One of -max_time or -target_health must be specified.\n";
          return response;
      }
      if ( fabs( vary_combat_length ) >= 1.0 )
      {
          vary_combat_length = 0.0;
      }
      if ( confidence <= 0.0 || confidence >= 1.0 )
      {
          confidence = 0.95;
      }

      response->iterations = iterations;
      response->max_time = max_time.total_seconds();
      response->optimal_raid = optimal_raid;
      response->fight_style = fight_style.c_str();

      if ( execute() )
      {

          scaling      -> analyze();
          plot         -> analyze();
          reforge_plot -> analyze();

          // We would need to do the magic here. At this point all of the data we need
          response->simulator = this; //Just pass this to node. It has what we need
          // Since this is node, of course, we want callbacks for every time it incremements
          response->error = "";
      }
      else
          response->error = "Cancelled";
  }

  return response;

}
