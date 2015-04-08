#include "simulationcraft.h"

sim_t_response * simnode_t::returns( std::vector<std::string>& args) {

  // sim_signal_handler_t handler( this );

  //cache_initializer_t cache_init( ( get_cache_directory() + "/simc_cache.dat" ).c_str() );
  //dbc_initializer_t dbc_init;
  //module_t::init();

  sim_control_t control;

  sim_t_response *response = new sim_t_response;
  int canceled = 0;

  try
  {
    control.options.parse_args(args);
  }
  catch (const std::exception& e) {
    response->error = "ERROR! Incorrect option format";
    canceled = 1;

    return response;
  }

  bool setup_success = true;
  std::string errmsg;

  try
  {
    setup( &control );
  }
  catch( const std::exception& e ){
    errmsg = e.what();
    setup_success = false;
  }

  response->sc_version = SC_VERSION;
  response->wow_version = dbc.wow_version();
  response->wow_ptr_version = dbc.wow_ptr_status();
  response->build_level = util::to_string( dbc.build_level() ).c_str();

  if ( ! setup_success )
  {
    response->error =  "ERROR! Setup failure: " + errmsg;
    return response;
  }

  if ( canceled ) {
    response->error = "Canceled";
    return response;
  }

  if ( spell_query )
  {
    try
    {
      spell_query -> evaluate();
      //print_spell_query();
    }
    catch( const std::exception& e ){
      response->error = "ERROR! Spell Query failure";
      return response;
    }
  }
  //else if ( need_to_save_profiles( this ) )
  //{
  //  init();
  //}
  else
  {
    response->iterations = iterations;
    response->max_time = max_time.total_seconds();
    response->combat_length = vary_combat_length;
    response->fight_style = fight_style.c_str();

    if ( execute() )
    {
      scaling      -> analyze();
      plot         -> analyze();
      reforge_plot -> analyze();
    }
    else {
      response->canceled = 1;
      return response;
    }
  }

  response->simulator = this; //Just pass this to node. It has what we need
  // Since this is node, of course, we want callbacks for every time it incremements
  response->error = "";

  return response;



}
