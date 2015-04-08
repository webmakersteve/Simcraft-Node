// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

enum stats_mask_e
{
  MASK_DMG     = 1 << STATS_DMG,
  MASK_HEAL    = 1 << STATS_HEAL,
  MASK_ABSORB  = 1 << STATS_ABSORB,
  MASK_NEUTRAL = 1 << STATS_NEUTRAL
};

bool has_avoidance( const std::vector<stats_t::stats_results_t>& s )
{
  return ( s[ RESULT_MISS   ].count.mean() +
           s[ RESULT_DODGE  ].count.mean() +
           s[ RESULT_PARRY  ].count.mean() ) > 0;
}

bool has_multistrike( const std::vector<stats_t::stats_results_t>& s)
{
  return ( s[ RESULT_MULTISTRIKE ].count.mean() + s[ RESULT_MULTISTRIKE_CRIT ].count.mean() ) > 0;
}

bool has_amount_results( const std::vector<stats_t::stats_results_t>& res )
{
  return (
      res[ RESULT_HIT ].actual_amount.mean() > 0 ||
      res[ RESULT_CRIT ].actual_amount.mean() > 0 ||
      res[ RESULT_MULTISTRIKE ].actual_amount.mean() > 0 ||
      res[ RESULT_MULTISTRIKE_CRIT ].actual_amount.mean() > 0
  );
}

bool has_block( const std::vector<stats_t::stats_results_t>& s )
{
  return ( s[ FULLTYPE_HIT_BLOCK ].count.mean() +
           s[ FULLTYPE_HIT_CRITBLOCK ].count.mean() +
           s[ FULLTYPE_GLANCE_BLOCK ].count.mean() +
           s[ FULLTYPE_GLANCE_CRITBLOCK ].count.mean() +
           s[ FULLTYPE_CRIT_BLOCK ].count.mean() +
           s[ FULLTYPE_CRIT_CRITBLOCK ].count.mean() ) > 0;
}

bool player_has_tick_results( player_t* p, unsigned stats_mask )
{
  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    if ( ! ( stats_mask & ( 1 << p -> stats_list[ i ] -> type ) ) )
      continue;

    if ( p -> stats_list[ i ] -> num_tick_results.count() > 0 )
      return true;
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    if ( player_has_tick_results( p -> pet_list[ i ], stats_mask ) )
      return true;
  }

  return false;
}

bool player_has_avoidance( player_t* p, unsigned stats_mask )
{
  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    if ( ! ( stats_mask & ( 1 << p -> stats_list[ i ] -> type ) ) )
      continue;

    if ( has_avoidance( p -> stats_list[ i ] -> direct_results ) )
      return true;

    if ( has_avoidance( p -> stats_list[ i ] -> tick_results ) )
      return true;
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    if ( player_has_avoidance( p -> pet_list[ i ], stats_mask ) )
      return true;
  }

  return false;
}

bool player_has_multistrike( player_t* p, unsigned stats_mask )
{
  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    if ( ! ( stats_mask & ( 1 << p -> stats_list[ i ] -> type ) ) )
      continue;

    if ( has_multistrike( p -> stats_list[ i ] -> direct_results ) )
      return true;

    if ( has_multistrike( p -> stats_list[ i ] -> tick_results ) )
      return true;
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    if ( player_has_multistrike( p -> pet_list[ i ], stats_mask ) )
      return true;
  }

  return false;
}

bool player_has_block( player_t* p, unsigned stats_mask )
{
  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    if ( ! ( stats_mask & ( 1 << p -> stats_list[ i ] -> type ) ) )
      continue;

    if ( has_block( p -> stats_list[ i ] -> direct_results_detail ) ||
         has_block( p -> stats_list[ i ] -> tick_results_detail ) )
      return true;
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    if ( player_has_block( p -> pet_list[ i ], stats_mask ) )
      return true;
  }

  return false;
}

bool player_has_glance( player_t* p, unsigned stats_mask )
{
  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    if ( ! ( stats_mask & ( 1 << p -> stats_list[ i ] -> type ) ) )
      continue;

    if ( p -> stats_list[ i ] -> direct_results[ RESULT_GLANCE ].count.mean() > 0 )
      return true;
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    if ( player_has_glance( p -> pet_list[ i ], stats_mask ) )
      return true;
  }

  return false;
}

std::string output_action_name( stats_t* s, player_t* actor )
{
  std::string href = "#";
  std::string rel = " rel=\"lvl=" + util::to_string( s -> player -> level ) + "\"";
  std::string prefix, suffix, class_attr;
  action_t* a = 0;

  if ( s -> player -> sim -> report_details )
    class_attr = " class=\"toggle-details\"";

  for ( size_t i = 0; i < s -> action_list.size(); i++ )
  {
    if ( ( a = s -> action_list[ i ] ) -> id > 1 )
      break;
  }

  wowhead::wowhead_e domain = SC_BETA ? wowhead::BETA : wowhead::LIVE;
  if ( ! SC_BETA )
    domain = s -> player -> dbc.ptr ? wowhead::PTR : wowhead::LIVE;

  std::string name;
  if ( a && a -> sim -> wowhead_tooltips )
    name = wowhead::decorated_action_name( s -> name_str, a, domain );
  else
    name = "<a href=\"#\">" + s -> name_str + "</a>";

  // If we are printing a stats object that belongs to a pet, for an actual
  // actor, print out the pet name too
  if ( actor && ! actor -> is_pet() && s -> player -> is_pet() )
    name += " (" + s -> player -> name_str + ")";

  return "<span " + class_attr + ">" + name + "</span>";
}

// print_html_action_info =================================================

double mean_damage( const std::vector<stats_t::stats_results_t>& result )
{
  double mean = 0;
  size_t count = 0;

  for ( size_t i = 0; i < result.size(); i++ )
  {
    // Skip multistrike for mean damage
    if ( i == RESULT_MULTISTRIKE || i == RESULT_MULTISTRIKE_CRIT )
      continue;

    mean  += result[ i ].actual_amount.sum();
    count += result[ i ].actual_amount.count();
  }

  if ( count > 0 )
    mean /= count;

  return mean;
}

void print_html_action_summary( report::sc_html_stream& os, unsigned stats_mask, int result_type, const stats_t* s, player_t* p )
{
  std::string type_str;
  if ( result_type == 1 )
    type_str = "Periodic";
  else
    type_str = "Direct";

  const std::vector<stats_t::stats_results_t>& results = result_type == 1 ? s -> tick_results : s -> direct_results;
  const std::vector<stats_t::stats_results_t>& block_results = result_type == 1 ? s -> tick_results_detail : s -> direct_results_detail;

  // Result type
  os.format( "<td class=\"right small\">%s</td>\n", type_str.c_str() );

  // Count
  double count = ( result_type == 1 ) ? s -> num_tick_results.mean() : s -> num_direct_results.mean();

  os.format( "<td class=\"right small\">%.1f</td>\n", count );

  // Hit results
  os.format( "<td class=\"right small\">%.0f</td>\n",
      results[ RESULT_HIT ].actual_amount.pretty_mean() );

  // Crit results
  os.format( "<td class=\"right small\">%.0f</td>\n",
      results[ RESULT_CRIT ].actual_amount.pretty_mean() );

  // Mean amount
  os.format( "<td class=\"right small\">%.0f</td>\n",
      mean_damage( results ) );

  // Crit%
  os.format( "<td class=\"right small\">%.1f%%</td>\n",
    results[ RESULT_CRIT ].pct );

  if ( player_has_avoidance( p, stats_mask ) )
    os.format(
      "<td class=\"right small\">%.1f%%</td>\n",   // direct_results Avoid%
      results[ RESULT_MISS ].pct +
      results[ RESULT_DODGE  ].pct +
      results[ RESULT_PARRY  ].pct );

  if ( player_has_glance( p, stats_mask ) )
    os.format(
      "<td class=\"right small\">%.1f%%</td>\n",  // direct_results Glance%
      results[ RESULT_GLANCE ].pct );

  if ( player_has_block( p, stats_mask ) )
    os.format(
      "<td class=\"right small\">%.1f%%</td>\n", // direct_results Block%
      block_results[ FULLTYPE_HIT_BLOCK ].pct +
      block_results[ FULLTYPE_HIT_CRITBLOCK ].pct +
      block_results[ FULLTYPE_GLANCE_BLOCK ].pct +
      block_results[ FULLTYPE_GLANCE_CRITBLOCK ].pct +
      block_results[ FULLTYPE_CRIT_BLOCK ].pct +
      block_results[ FULLTYPE_CRIT_CRITBLOCK ].pct );

  if ( player_has_multistrike( p, stats_mask ) )
  {
    os.format(
      "<td class=\"right small\">%.1f</td>\n"     // count
      "<td class=\"right small\">%.0f</td>\n"     // direct_results MULTISTRIKE_HIT
      "<td class=\"right small\">%.0f</td>\n"     // direct_results MULTISTRIKE_CRIT
      "<td class=\"right small\">%.1f%%</td>\n",  // direct_results CRIT%
      results[ RESULT_MULTISTRIKE ].count.pretty_mean() + 
      results[ RESULT_MULTISTRIKE_CRIT ].count.pretty_mean(),
      results[ RESULT_MULTISTRIKE ].actual_amount.pretty_mean(),
      results[ RESULT_MULTISTRIKE_CRIT ].actual_amount.pretty_mean(),
      results[ RESULT_MULTISTRIKE_CRIT ].pct );
  }

  if ( player_has_tick_results( p, stats_mask ) )
  {
    if ( util::str_in_str_ci( type_str, "Periodic" ) )
      os.format(
        "<td class=\"right small\">%.1f%%</td>\n",   // Uptime%
        100 * s -> total_tick_time.mean() / p -> collected_data.fight_length.mean() );
    else
      os.format("<td class=\"right small\">&#160;</td>\n");
  }

}

void print_html_action_info( report::sc_html_stream& os, unsigned stats_mask, stats_t* s, int j, int n_columns, player_t* actor = 0 )
{
  player_t* p;
  if ( s -> player -> is_pet() )
    p = s -> player -> cast_pet() -> owner;
  else
    p = s -> player;

  std::string row_class = ( j & 1 ) ? " class=\"odd\"" : "";

  os << "<tr" << row_class << ">\n";
  int result_rows = has_amount_results( s -> direct_results ) + has_amount_results( s -> tick_results );
  if ( result_rows == 0 )
    result_rows = 1;

  // Ability name
  os << "<td class=\"left small\" rowspan=\"" << result_rows << "\">";
  if ( s -> parent && s -> parent -> player == actor )
    os << "&#160;&#160;&#160;\n";

  os << output_action_name( s, actor );
  os << "</td>\n";

  // DPS and DPS %
  // Skip for abilities that do no damage
  if ( s -> compound_amount > 0 || ( s -> parent && s -> parent -> compound_amount > 0 ) )
  {
    std::string compound_aps     = "";
    std::string compound_aps_pct = "";
    double cAPS    = s -> portion_aps.mean();
    double cAPSpct = s -> portion_amount;

    for ( size_t i = 0, num_children = s -> children.size(); i < num_children; i++ )
    {
      cAPS    += s -> children[ i ] -> portion_apse.mean();
      cAPSpct += s -> children[ i ] -> portion_amount;
    }

    if ( cAPS > s -> portion_aps.mean()  ) compound_aps     = "&#160;(" + util::to_string( cAPS, 0 ) + ")";
    if ( cAPSpct > s -> portion_amount ) compound_aps_pct = "&#160;(" + util::to_string( cAPSpct * 100, 1 ) + "%)";

    os.format( "<td class=\"right small\" rowspan=\"%d\">%.0f%s</td>\n",
      result_rows,
      s -> portion_aps.pretty_mean(), compound_aps.c_str() );
    os.format( "<td class=\"right small\" rowspan=\"%d\">%.1f%%%s</td>\n",
      result_rows,
      s -> portion_amount * 100, compound_aps_pct.c_str() );
  }

  // Number of executes
  os.format( "<td class=\"right small\" rowspan=\"%d\">%.1f</td>\n",
    result_rows,
    s -> num_executes.pretty_mean() );

  // Execute interval
  os.format( "<td class=\"right small\" rowspan=\"%d\">%.2fsec</td>\n",
    result_rows,
    s -> total_intervals.pretty_mean() );
  
  // Skip the rest of this for abilities that do no damage
  if ( s -> compound_amount > 0 )
  {
    // Amount per execute
    os.format( "<td class=\"right small\" rowspan=\"%d\">%.0f</td>\n",
      result_rows,
      s -> ape );

    // Amount per execute time
    os.format( "<td class=\"right small\" rowspan=\"%d\">%.0f</td>\n",
      result_rows,
      s -> apet );

    bool periodic_only = false;
    if ( has_amount_results( s -> direct_results ) )
      print_html_action_summary( os, stats_mask, 0, s, p );
    else if ( has_amount_results( s -> tick_results ) )
    {
      periodic_only = true;
      print_html_action_summary( os, stats_mask, 1, s, p );
    }
    else
      os.format("<td class=\"right small\" colspan=\"%d\"></td>\n", n_columns );

    os.format( "</tr>\n" );

    if ( ! periodic_only && has_amount_results( s -> tick_results ) )
    {
      os << "<tr" << row_class << ">\n";
      print_html_action_summary( os, stats_mask, 1, s, p );
      os << "</tr>\n";
    }

  }

  if ( p -> sim -> report_details )
  {
    std::string timeline_stat_aps_str                    = "";
    if ( ! s -> timeline_aps_chart.empty() )
    {
      timeline_stat_aps_str = "<img src=\"" + s -> timeline_aps_chart + "\" alt=\"" + ( s -> type == STATS_DMG ? "DPS" : "HPS" ) + " Timeline Chart\" />\n";
    }
    std::string aps_distribution_str                    = "";
    if ( ! s -> aps_distribution_chart.empty() )
    {
      aps_distribution_str = "<img src=\"" + s -> aps_distribution_chart + "\" alt=\"" + ( s -> type == STATS_DMG ? "DPS" : "HPS" ) + " Distribution Chart\" />\n";
    }
    os << "<tr class=\"details hide\">\n"
       << "<td colspan=\"" << ( 7 + n_columns ) << "\" class=\"filler\">\n";

    // Stat Details
    os.format(
      "<h4>Stats details: %s </h4>\n", s -> name_str.c_str() );

    os << "<table class=\"details\">\n"
       << "<tr>\n";

    os << "<th class=\"small\">Type</th>\n"
       << "<th class=\"small\">Executes</th>\n"
       << "<th class=\"small\">Direct Results</th>\n"
       << "<th class=\"small\">Ticks</th>\n"
       << "<th class=\"small\">Tick Results</th>\n"
       << "<th class=\"small\">Execute Time per Execution</th>\n"
       << "<th class=\"small\">Tick Time per  Tick</th>\n"
       << "<th class=\"small\">Actual Amount</th>\n"
       << "<th class=\"small\">Total Amount</th>\n"
       << "<th class=\"small\">Overkill %</th>\n"
       << "<th class=\"small\">Amount per Total Time</th>\n"
       << "<th class=\"small\">Amount per Total Execute Time</th>\n";

    os << "</tr>\n"
       << "<tr>\n";

    os.format(
      "<td class=\"right small\">%s</td>\n"
      "<td class=\"right small\">%.2f</td>\n"
      "<td class=\"right small\">%.2f</td>\n"
      "<td class=\"right small\">%.2f</td>\n"
      "<td class=\"right small\">%.2f</td>\n"
      "<td class=\"right small\">%.4f</td>\n"
      "<td class=\"right small\">%.4f</td>\n"
      "<td class=\"right small\">%.2f</td>\n"
      "<td class=\"right small\">%.2f</td>\n"
      "<td class=\"right small\">%.2f</td>\n"
      "<td class=\"right small\">%.2f</td>\n"
      "<td class=\"right small\">%.2f</td>\n",
      util::stats_type_string( s -> type ),
      s -> num_executes.mean(),
      s -> num_direct_results.mean(),
      s -> num_ticks.mean(),
      s -> num_tick_results.mean(),
      s -> etpe,
      s -> ttpt,
      s -> actual_amount.mean(),
      s -> total_amount.mean(),
      s -> overkill_pct,
      s -> aps,
      s -> apet );
    os << "</tr>\n";
    os << "</table>\n";
    if ( ! s -> portion_aps.simple || ! s -> portion_apse.simple || ! s -> actual_amount.simple )
    {
      os << "<table class=\"details\">\n";
      int sd_counter = 0;
      report::print_html_sample_data( os, p -> sim, s -> actual_amount, "Actual Amount", sd_counter );

      report::print_html_sample_data( os, p -> sim, s -> portion_aps, "portion Amount per Second ( pAPS )", sd_counter );

      report::print_html_sample_data( os, p -> sim, s -> portion_apse, "portion Effective Amount per Second ( pAPSe )", sd_counter );

      os << "</table>\n";

      if ( ! s -> portion_aps.simple && p -> sim -> scaling -> has_scale_factors() )
      {
        int colspan = 0;
        os << "<table class=\"details\">\n";
        os << "<tr>\n"
           << "<th><a href=\"#help-scale-factors\" class=\"help\">?</a></th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p -> scales_with[ i ] )
          {
            os.format(
              "<th>%s</th>\n",
              util::stat_type_abbrev( i ) );
            colspan++;
          }
        if ( p -> sim -> scaling -> scale_lag )
        {
          os << "<th>ms Lag</th>\n";
          colspan++;
        }
        os << "</tr>\n";
        os << "<tr>\n"
           << "<th class=\"left\">Scale Factors</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p -> scales_with[ i ] )
          {
            if ( s -> scaling.get_stat( i ) > 1.0e5 )
              os.format(
                "<td>%.*e</td>\n",
                p -> sim -> report_precision,
                s -> scaling.get_stat( i ) );
            else
              os.format(
                "<td>%.*f</td>\n",
                p -> sim -> report_precision,
                s -> scaling.get_stat( i ) );
          }
        os << "</tr>\n";
        os << "<tr>\n"
           << "<th class=\"left\">Scale Deltas</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
        {
          if ( ! p -> scales_with[ i ] )
            continue;

          double value = p -> sim -> scaling -> stats.get_stat( i );
          std::string prefix;
          if ( p -> sim -> scaling -> center_scale_delta == 1 && 
               i != STAT_SPIRIT && i != STAT_HIT_RATING && i != STAT_EXPERTISE_RATING )
          {
            prefix = "+/- ";
            value /= 2;
          }

          os.format( "<td>%s%.0f</td>\n", prefix.c_str(), value );
        }
        os << "</tr>\n";
        os << "<tr>\n"
           << "<th class=\"left\">Error</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p -> scales_with[ i ] )
          {
            scale_metric_e sm = p -> sim -> scaling -> scaling_metric;
            if ( p -> scaling_error[ sm ].get_stat( i ) > 1.0e5 )
              os.format(
                "<td>%.*e</td>\n",
                p -> sim -> report_precision,
                s -> scaling_error.get_stat( i ) );
            else
              os.format(
                "<td>%.*f</td>\n",
                p -> sim -> report_precision,
                s -> scaling_error.get_stat( i ) );
          }
        os << "</tr>\n";
        os << "</table>\n";

      }
    }

    // Detailed breakdown of damage or healing ability
    os << "<table  class=\"details\">\n";
    if ( s -> num_direct_results.mean() > 0 )
    {
      os << "<tr>\n"
         << "<th class=\"small\" colspan=\"3\">&#160;</th>\n"
         << "<th class=\"small\" colspan=\"3\">Simulation</th>\n"
         << "<th class=\"small\" colspan=\"3\">Iteration Average</th>\n"
         << "<th class=\"small\" colspan=\"3\">&#160;</th>\n"
         << "<tr>\n";

      // Direct Damage
      os << "<tr>\n"
         << "<th class=\"small\">Direct Results</th>\n"
         << "<th class=\"small\">Count</th>\n"
         << "<th class=\"small\">Pct</th>\n"
         << "<th class=\"small\">Mean</th>\n"
         << "<th class=\"small\">Min</th>\n"
         << "<th class=\"small\">Max</th>\n"
         << "<th class=\"small\">Mean</th>\n"
         << "<th class=\"small\">Min</th>\n"
         << "<th class=\"small\">Max</th>\n"
         << "<th class=\"small\">Actual Amount</th>\n"
         << "<th class=\"small\">Total Amount</th>\n"
         << "<th class=\"small\">Overkill %</th>\n"
         << "</tr>\n";
      int k = 0;
      if ( has_block( s -> direct_results_detail ) )
      {
        for ( full_result_e i = FULLTYPE_MAX; --i >= FULLTYPE_NONE; )
        {
          if ( ! s -> direct_results_detail[ i ].count.mean() )
            continue;

          os << "<tr";

          if ( k & 1 )
          {
            os << " class=\"odd\"";
          }
          k++;
          os << ">\n";

          os.format(
            "<td class=\"left small\">%s</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.2f%%</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "</tr>\n",
            util::full_result_type_string( i ),
            s -> direct_results_detail[ i ].count.mean(),
            s -> direct_results_detail[ i ].pct,
            s -> direct_results_detail[ i ].actual_amount.mean(),
            s -> direct_results_detail[ i ].actual_amount.min(),
            s -> direct_results_detail[ i ].actual_amount.max(),
            s -> direct_results_detail[ i ].avg_actual_amount.mean(),
            s -> direct_results_detail[ i ].avg_actual_amount.min(),
            s -> direct_results_detail[ i ].avg_actual_amount.max(),
            s -> direct_results_detail[ i ].fight_actual_amount.mean(),
            s -> direct_results_detail[ i ].fight_total_amount.mean(),
            s -> direct_results_detail[ i ].overkill_pct.mean() );
        }
      }
      else
      {
        for ( result_e i = RESULT_MAX; --i >= RESULT_NONE; )
        {
          if ( ! s -> direct_results[ i ].count.mean() )
            continue;

          os << "<tr";

          if ( k & 1 )
          {
            os << " class=\"odd\"";
          }
          k++;
          os << ">\n";

          os.format(
            "<td class=\"left small\">%s</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.2f%%</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "</tr>\n",
            util::result_type_string( i ),
            s -> direct_results[ i ].count.mean(),
            s -> direct_results[ i ].pct,
            s -> direct_results[ i ].actual_amount.mean(),
            s -> direct_results[ i ].actual_amount.min(),
            s -> direct_results[ i ].actual_amount.max(),
            s -> direct_results[ i ].avg_actual_amount.mean(),
            s -> direct_results[ i ].avg_actual_amount.min(),
            s -> direct_results[ i ].avg_actual_amount.max(),
            s -> direct_results[ i ].fight_actual_amount.mean(),
            s -> direct_results[ i ].fight_total_amount.mean(),
            s -> direct_results[ i ].overkill_pct.mean() );
        }
      }

    }

    if ( s -> num_tick_results.mean() > 0 )
    {
      // Tick Damage
      os << "<tr>\n"
         << "<th class=\"small\" colspan=\"3\">&#160;</th>\n"
         << "<th class=\"small\" colspan=\"3\">Simulation</th>\n"
         << "<th class=\"small\" colspan=\"3\">Iteration Average</th>\n"
         << "<th class=\"small\" colspan=\"3\">&#160;</th>\n"
         << "<tr>\n";

      os << "<tr>\n"
         << "<th class=\"small\">Tick Results</th>\n"
         << "<th class=\"small\">Count</th>\n"
         << "<th class=\"small\">Pct</th>\n"
         << "<th class=\"small\">Mean</th>\n"
         << "<th class=\"small\">Min</th>\n"
         << "<th class=\"small\">Max</th>\n"
         << "<th class=\"small\">Mean</th>\n"
         << "<th class=\"small\">Min</th>\n"
         << "<th class=\"small\">Max</th>\n"
         << "<th class=\"small\">Actual Amount</th>\n"
         << "<th class=\"small\">Total Amount</th>\n"
         << "<th class=\"small\">Overkill %</th>\n"
         << "</tr>\n";
      int k = 0;
      if ( has_block( s -> tick_results_detail ) )
      {
        for ( full_result_e i = FULLTYPE_MAX; --i >= FULLTYPE_NONE; )
        {
          if ( ! s -> tick_results_detail[ i ].count.mean() )
            continue;

          os << "<tr";
          if ( k & 1 )
          {
            os << " class=\"odd\"";
          }
          k++;
          os << ">\n";
          os.format(
            "<td class=\"left small\">%s</td>\n"
            "<td class=\"right small\">%.1f</td>\n"
            "<td class=\"right small\">%.2f%%</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "</tr>\n",
            util::full_result_type_string( i ),
            s -> tick_results_detail[ i ].count.mean(),
            s -> tick_results_detail[ i ].pct,
            s -> tick_results_detail[ i ].actual_amount.mean(),
            s -> tick_results_detail[ i ].actual_amount.min(),
            s -> tick_results_detail[ i ].actual_amount.max(),
            s -> tick_results_detail[ i ].avg_actual_amount.mean(),
            s -> tick_results_detail[ i ].avg_actual_amount.min(),
            s -> tick_results_detail[ i ].avg_actual_amount.max(),
            s -> tick_results_detail[ i ].fight_actual_amount.mean(),
            s -> tick_results_detail[ i ].fight_total_amount.mean(),
            s -> tick_results_detail[ i ].overkill_pct.mean() );
        }
      }
      else
      {
        for ( result_e i = RESULT_MAX; --i >= RESULT_NONE; )
        {
          if ( ! s -> tick_results[ i ].count.mean() )
            continue;

          os << "<tr";
          if ( k & 1 )
          {
            os << " class=\"odd\"";
          }
          k++;
          os << ">\n";
          os.format(
            "<td class=\"left small\">%s</td>\n"
            "<td class=\"right small\">%.1f</td>\n"
            "<td class=\"right small\">%.2f%%</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "</tr>\n",
            util::result_type_string( i ),
            s -> tick_results[ i ].count.mean(),
            s -> tick_results[ i ].pct,
            s -> tick_results[ i ].actual_amount.mean(),
            s -> tick_results[ i ].actual_amount.min(),
            s -> tick_results[ i ].actual_amount.max(),
            s -> tick_results[ i ].avg_actual_amount.mean(),
            s -> tick_results[ i ].avg_actual_amount.min(),
            s -> tick_results[ i ].avg_actual_amount.max(),
            s -> tick_results[ i ].fight_actual_amount.mean(),
            s -> tick_results[ i ].fight_total_amount.mean(),
            s -> tick_results[ i ].overkill_pct.mean() );
        }
      }
    }

    os << "</table>\n";

    os << "<div class=\"clear\">&#160;</div>\n";

    os.format(
      "%s\n",
      timeline_stat_aps_str.c_str() );
    os.format(
      "%s\n",
      aps_distribution_str.c_str() );

    os << "<div class=\"clear\">&#160;</div>\n";
    // Action Details
    std::vector<std::string> processed_actions;

    for ( size_t i = 0; i < s -> action_list.size(); i++ )
    {
      action_t* a = s -> action_list[ i ];

      bool found = false;
      size_t size_processed = processed_actions.size();
      for ( size_t k = 0; k < size_processed && !found; k++ )
        if ( processed_actions[ k ] == a -> name() )
          found = true;
      if ( found ) continue;
      processed_actions.push_back( a -> name() );

      os.format(
        "<h4>Action details: %s </h4>\n", a -> name() );

      os.format(
        "<div class=\"float\">\n"
        "<h5>Static Values</h5>\n"
        "<ul>\n"
        "<li><span class=\"label\">id:</span>%i</li>\n"
        "<li><span class=\"label\">school:</span>%s</li>\n"
        "<li><span class=\"label\">resource:</span>%s</li>\n"
        "<li><span class=\"label\">range:</span>%.1f</li>\n"
        "<li><span class=\"label\">travel_speed:</span>%.4f</li>\n"
        "<li><span class=\"label\">trigger_gcd:</span>%.4f</li>\n"
        "<li><span class=\"label\">min_gcd:</span>%.4f</li>\n"
        "<li><span class=\"label\">base_cost:</span>%.1f</li>\n"
        "<li><span class=\"label\">cooldown:</span>%.3f</li>\n"
        "<li><span class=\"label\">base_execute_time:</span>%.2f</li>\n"
        "<li><span class=\"label\">base_crit:</span>%.2f</li>\n"
        "<li><span class=\"label\">target:</span>%s</li>\n"
        "<li><span class=\"label\">harmful:</span>%s</li>\n"
        "<li><span class=\"label\">if_expr:</span>%s</li>\n"
        "</ul>\n"
        "</div>\n",
        a -> id,
        util::school_type_string( a -> get_school() ),
        util::resource_type_string( a -> current_resource() ),
        a -> range,
        a -> travel_speed,
        a -> trigger_gcd.total_seconds(),
        a -> min_gcd.total_seconds(),
        a -> base_costs[ a -> current_resource() ],
        a -> cooldown -> duration.total_seconds() * a -> cooldown -> get_recharge_multiplier(),
        a -> base_execute_time.total_seconds(),
        a -> base_crit,
        a -> target ? a -> target -> name() : "",
        a -> harmful ? "true" : "false",
        util::encode_html( a -> if_expr_str ).c_str() );

      // Spelldata
      if ( a -> data().ok() )
      {
        os.format(
          "<div class=\"float\">\n"
          "<h5>Spelldata</h5>\n"
          "<ul>\n"
          "<li><span class=\"label\">id:</span>%i</li>\n"
          "<li><span class=\"label\">name:</span>%s</li>\n"
          "<li><span class=\"label\">school:</span>%s</li>\n"
          "<li><span class=\"label\">tooltip:</span><span class=\"tooltip\">%s</span></li>\n"
          "<li><span class=\"label\">description:</span><span class=\"tooltip\">%s</span></li>\n"
          "</ul>\n"
          "</div>\n",
          a -> data().id(),
          a -> data().name_cstr(),
          util::school_type_string( a -> data().get_school_type() ),
          pretty_spell_text( a -> data(), a -> data().tooltip(), *p ).c_str(),
          util::encode_html( pretty_spell_text( a -> data(), a -> data().desc(), *p ) ).c_str() );
      }

      if ( a -> spell_power_mod.direct || a -> base_dd_min || a -> base_dd_max )
      {
        os.format(
          "<div class=\"float\">\n"
          "<h5>Direct Damage</h5>\n"
          "<ul>\n"
          "<li><span class=\"label\">may_crit:</span>%s</li>\n"
          "<li><span class=\"label\">attack_power_mod.direct:</span>%.6f</li>\n"
          "<li><span class=\"label\">spell_power_mod.direct:</span>%.6f</li>\n"
          "<li><span class=\"label\">base_dd_min:</span>%.2f</li>\n"
          "<li><span class=\"label\">base_dd_max:</span>%.2f</li>\n"
          "</ul>\n"
          "</div>\n",
          a -> may_crit ? "true" : "false",
          a -> attack_power_mod.direct,
          a -> spell_power_mod.direct,
          a -> base_dd_min,
          a -> base_dd_max );
      }
      if ( a -> dot_duration > timespan_t::zero() )
      {
        os.format(
          "<div class=\"float\">\n"
          "<h5>Damage Over Time</h5>\n"
          "<ul>\n"
          "<li><span class=\"label\">tick_may_crit:</span>%s</li>\n"
          "<li><span class=\"label\">tick_zero:</span>%s</li>\n"
          "<li><span class=\"label\">attack_power_mod.tick:</span>%.6f</li>\n"
          "<li><span class=\"label\">spell_power_mod.tick:</span>%.6f</li>\n"
          "<li><span class=\"label\">base_td:</span>%.2f</li>\n"
          "<li><span class=\"label\">dot_duration:</span>%.2f</li>\n"
          "<li><span class=\"label\">base_tick_time:</span>%.2f</li>\n"
          "<li><span class=\"label\">hasted_ticks:</span>%s</li>\n"
          "<li><span class=\"label\">dot_behavior:</span>%s</li>\n"
          "</ul>\n"
          "</div>\n",
          a -> tick_may_crit ? "true" : "false",
          a -> tick_zero ? "true" : "false",
          a -> attack_power_mod.tick,
          a -> spell_power_mod.tick,
          a -> base_td,
          a -> dot_duration.total_seconds(),
          a -> base_tick_time.total_seconds(),
          a -> hasted_ticks ? "true" : "false",
          util::dot_behavior_type_string( a -> dot_behavior ) );
      }
      // Extra Reporting for DKs
      if ( a -> player -> type == DEATH_KNIGHT )
      {
        os.format(
          "<div class=\"float\">\n"
          "<h5>Rune Information</h5>\n"
          "<ul>\n"
          "<li><span class=\"label\">Blood Cost:</span>%d</li>\n"
          "<li><span class=\"label\">Frost Cost:</span>%d</li>\n"
          "<li><span class=\"label\">Unholy Cost:</span>%d</li>\n"
          "<li><span class=\"label\">Runic Power Gain:</span>%.2f</li>\n"
          "</ul>\n"
          "</div>\n",
          a -> data().rune_cost() & 0x1,
          ( a -> data().rune_cost() >> 4 ) & 0x1,
          ( a -> data().rune_cost() >> 2 ) & 0x1,
          a -> rp_gain );
      }
      if ( a -> weapon )
      {
        os.format(
          "<div class=\"float\">\n"
          "<h5>Weapon</h5>\n"
          "<ul>\n"
          "<li><span class=\"label\">normalized:</span>%s</li>\n"
          "<li><span class=\"label\">weapon_power_mod:</span>%.6f</li>\n"
          "<li><span class=\"label\">weapon_multiplier:</span>%.2f</li>\n"
          "</ul>\n"
          "</div>\n",
          a -> normalize_weapon_speed ? "true" : "false",
          a -> weapon_power_mod,
          a -> weapon_multiplier );
      }
      os << "<div class=\"clear\">&#160;</div>\n";
    }

    os << "</td>\n"
       << "</tr>\n";
  }
}

// print_html_action_resource ===============================================

int print_html_action_resource( report::sc_html_stream& os, stats_t* s, int j )
{
  for ( size_t i = 0; i < s -> player -> action_list.size(); ++i )
  {
    action_t* a = s -> player -> action_list[ i ];
    if ( a -> stats != s ) continue;
    if ( ! a -> background ) break;
  }

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( s -> resource_gain.actual[ i ] > 0 )
    {
      os << "<tr";
      if ( j & 1 )
      {
        os << " class=\"odd\"";
      }
      ++j;
      os << ">\n";
      os.format(
        "<td class=\"left\">%s</td>\n"
        "<td class=\"left\">%s</td>\n"
        "<td class=\"right\">%.1f</td>\n"
        "<td class=\"right\">%.1f</td>\n"
        "<td class=\"right\">%.1f</td>\n",
        s -> resource_gain.name(),
        util::inverse_tokenize( util::resource_type_string( i ) ).c_str(),
        s -> resource_gain.count[ i ],
        s -> resource_gain.actual[ i ],
        s -> resource_gain.actual[ i ] / s -> resource_gain.count[ i ] );
      os.format(
        "<td class=\"right\">%.1f</td>\n"
        "<td class=\"right\">%.1f</td>\n",
        s->rpe[ i ],
        s->apr[ i ] );
      os << "</tr>\n";
    }
  }
  return j;
}

// print_html_gear ==========================================================

void print_html_gear( report::sc_html_stream& os, player_t* p )
{
  if ( p -> items.empty() )
    return;

  os.format(
    "<div class=\"player-section gear\">\n"
    "<h3 class=\"toggle\">Gear</h3>\n"
    "<div class=\"toggle-content hide\">\n"
    "<table class=\"sc\">\n"
    "<tr>\n"
    "<th>Source</th>\n"
    "<th>Slot</th>\n"
    "<th>Average Item Level: %.2f</th>\n"
    "</tr>\n",
     util::get_avg_itemlvl( p ) );

  for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
  {
    item_t& item = p -> items[ i ];

    std::string domain = p -> dbc.ptr ? "ptr" : "www";
    std::string item_string;
    if ( item.active() )
    {
      std::string rel_str = "";
      rel_str += "&amp;";
      bool gems = false;
      for ( size_t k = 0; k < item.parsed.gem_id.size(); k++ )
      {
        if ( item.parsed.gem_id[k] != 0 )
        {
          gems = true;
          break;
        }
      }
#if defined SC_USE_WEBKIT && defined SC_WINDOWS
      gems = false; // Webkit + Windows + Gems in tooltip = GUI lock up.
#endif
      if ( gems )
      {
        rel_str += "gems=";
        for ( size_t k = 0; k < item.parsed.gem_id.size(); k++ )
        {
          if ( item.parsed.gem_id[k] != 0 )
          {
            rel_str += util::to_string( item.parsed.gem_id[k] );
            if ( k < item.parsed.gem_id.size() && item.parsed.gem_id[ k + 1 ] != 0 )
              rel_str += ":";
          }
        }
        if ( item.parsed.enchant_id || item.parsed.bonus_id.size() > 0 )
          rel_str += "&amp;";
      }
      if ( item.parsed.enchant_id )
      {
        rel_str += "ench=";
        rel_str += util::to_string( item.parsed.enchant_id );
        if ( item.parsed.bonus_id.size() > 0 )
          rel_str += "&amp;";
      }
      if ( item.parsed.bonus_id.size() )
      {
        rel_str += "bonus=";
        for ( size_t k = 0; k < item.parsed.bonus_id.size(); k++ )
        {
          rel_str += util::to_string( item.parsed.bonus_id[k] );
          if ( k + 1 < item.parsed.bonus_id.size() )
            rel_str += ":";
        }
      }
      item_string = ! item.parsed.data.id ? item.options_str : "<a href=\"http://" + domain + ".wowhead.com/item=" + util::to_string( item.parsed.data.id ) +
        rel_str + "\"" + " rel=" + rel_str + "\"" + ">" + item.encoded_item() + "</a>";
    }
    else
    {
      item_string = "empty";
    }

    os.format(
      "<tr>\n"
      "<th class=\"left\">%s</th>\n"
      "<th class=\"left\">%s</th>\n"
      "<td class=\"left\">%s</td>\n"
      "</tr>\n",
      item.source_str.c_str(),
      util::inverse_tokenize( item.slot_name() ).c_str(),
      item_string.c_str() );
    if ( item.active() )
    {
      std::string item_sim_desc = "ilevel: " + util::to_string( item.item_level() );

      if ( item.parsed.data.item_class == ITEM_CLASS_WEAPON )
      {
        item_sim_desc += ", weapon: { " + item.weapon_stats_str() + " }";
      }

      if ( item.has_stats() )
      {
        item_sim_desc += ", stats: { ";
        item_sim_desc += item.item_stats_str();
        if ( item.parsed.suffix_stats.size() > 0 )
        {
          item_sim_desc += ", " + item.suffix_stats_str();
        }

        item_sim_desc += " }";
      }

      if ( item.parsed.gem_stats.size() > 0 )
      {
        item_sim_desc += ", gems: { ";
        item_sim_desc += item.gem_stats_str();
        if ( item.socket_color_match() && item.parsed.socket_bonus_stats.size() > 0 )
        {
          item_sim_desc += ", ";
          item_sim_desc += item.socket_bonus_stats_str();
        }
        item_sim_desc += " }";
      }

      if ( item.parsed.enchant_stats.size() > 0 )
      {
        item_sim_desc += ", enchant: { ";
        item_sim_desc += item.enchant_stats_str();
        item_sim_desc += " }";
      }
      else if ( ! item.parsed.encoded_enchant.empty() )
      {
        item_sim_desc += ", enchant: " + item.parsed.encoded_enchant;
      }

      os.format(
        "<tr>\n"
        "<th class=\"left\" colspan=\"2\"></th>\n"
        "<td class=\"left small\">%s</td>\n"
        "</tr>\n",
        item_sim_desc.c_str() );
    }
  }

  os << "</table>\n"
     << "</div>\n"
     << "</div>\n";
}

// print_html_profile =======================================================

void print_html_profile ( report::sc_html_stream& os, player_t* a )
{
  if ( a -> collected_data.fight_length.mean() > 0 )
  {
    std::string profile_str;
    a -> create_profile( profile_str, SAVE_ALL );
    profile_str = util::encode_html( profile_str );
    util::replace_all( profile_str, "\n", "<br>" );

    os << "<div class=\"player-section profile\">\n"
       << "<h3 class=\"toggle\">Profile</h3>\n"
       << "<div class=\"toggle-content hide\">\n"
       << "<div class=\"subsection force-wrap\">\n"
       << "<p>" << profile_str << "</p>\n"
       << "</div>\n"
       << "</div>\n"
       << "</div>\n";
  }
}


// print_html_stats =========================================================

void print_html_stats ( report::sc_html_stream& os, player_t* a )
{
  player_collected_data_t::buffed_stats_t& buffed_stats = a -> collected_data.buffed_stats_snapshot;
  std::array<double, ATTRIBUTE_MAX> hybrid_attributes;
  range::fill( hybrid_attributes, 0 );

  if ( ! a -> items.empty() )
  {
    for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
    {
      for ( attribute_e j = ATTR_STAMINA; ++j < ATTRIBUTE_MAX; )
      {
        hybrid_attributes[ a -> convert_hybrid_stat( static_cast<stat_e>( j ) ) ] += a -> items[ i ].stats.attribute[ j ];
      }
    }
  }


  if ( a -> collected_data.fight_length.mean() > 0 )
  {
    int j = 1;

    os << "<div class=\"player-section stats\">\n"
       << "<h3 class=\"toggle\">Stats</h3>\n"
       << "<div class=\"toggle-content hide\">\n"
       << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th><a href=\"#help-stats-raid-buffed\" class=\"help\">Raid-Buffed</a></th>\n"
       << "<th><a href=\"#help-stats-unbuffed\" class=\"help\">Unbuffed</a></th>\n"
       << "<th><a href=\"#help-gear-amount\" class=\"help\">Gear Amount</a></th>\n"
       << "</tr>\n";

    for ( attribute_e i = ATTRIBUTE_NONE; ++i < ATTR_AGI_INT; )
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">%s</th>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        util::inverse_tokenize( util::attribute_type_string( i ) ).c_str(),
        util::floor( buffed_stats.attribute[ i ] ),
        util::floor( a -> get_attribute( i ) ),
        util::floor( a -> initial.stats.attribute[ i ] ) );
      // append hybrid attributes as a parenthetical if appropriate
      if ( hybrid_attributes[ i ] > 0 )
        os.format( " (%.0f)", hybrid_attributes[ i ]);

      os.format( "</td>\n</tr>\n");

      j++;
    }
    for ( resource_e i = RESOURCE_NONE; ++i < RESOURCE_MAX; )
    {
      if ( a -> resources.max[ i ] > 0 )
        os.format(
          "<tr%s>\n"
          "<th class=\"left\">%s</th>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          util::inverse_tokenize( util::resource_type_string( i ) ).c_str(),
          buffed_stats.resource[ i ],
          a -> resources.max[ i ],
          0.0 );
      j++;
    }
    if ( buffed_stats.spell_power > 0 )
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Spell Power</th>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        buffed_stats.spell_power,
        a -> composite_spell_power( SCHOOL_MAX ) * a -> composite_spell_power_multiplier(),
        a -> initial.stats.spell_power );
      j++;
    }
    if ( a -> composite_melee_crit() == a -> composite_spell_crit() )
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Crit</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * buffed_stats.attack_crit,
        100 * a -> composite_melee_crit(),
        a -> composite_melee_crit_rating() );
      j++;
    }
    else
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Melee Crit</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * buffed_stats.attack_crit,
        100 * a -> composite_melee_crit(),
        a -> composite_melee_crit_rating() );
      j++;
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Spell Crit</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * buffed_stats.spell_crit,
        100 * a -> composite_spell_crit(),
        a -> composite_spell_crit_rating() );
      j++;
    }
    if ( a -> composite_melee_haste() == a -> composite_spell_haste() )
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Haste</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * ( 1 / buffed_stats.attack_haste - 1 ), // Melee/Spell haste have been merged into a single stat.
        100 * ( 1 / a -> composite_melee_haste() - 1 ),
        a -> composite_melee_haste_rating() );
      j++;
    }
    else
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Melee Haste</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * ( 1 / buffed_stats.attack_haste - 1 ),
        100 * ( 1 / a -> composite_melee_haste() - 1 ),
        a -> composite_melee_haste_rating() );
      j++;
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Spell Haste</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * ( 1 / buffed_stats.spell_haste - 1 ),
        100 * ( 1 / a -> composite_spell_haste() - 1 ),
        a -> composite_spell_haste_rating() );
      j++;
    }
    if ( a -> composite_spell_speed() != a -> composite_spell_haste() )
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Spell Speed</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * ( 1 / buffed_stats.spell_speed - 1 ),
        100 * ( 1 / a -> composite_spell_speed() - 1 ),
        a -> composite_spell_haste_rating() );
      j++;
    }
    if ( a -> composite_melee_speed() != a -> composite_melee_haste() )
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Swing Speed</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * ( 1 / buffed_stats.attack_speed - 1 ),
        100 * ( 1 / a -> composite_melee_speed() - 1 ),
        a -> composite_melee_haste_rating() );
      j++;
    }
    os.format(
      "<tr%s>\n"
      "<th class=\"left\">Multistrike</th>\n"
      "<td class=\"right\">%.2f%%</td>\n"
      "<td class=\"right\">%.2f%%</td>\n"
      "<td class=\"right\">%.0f</td>\n"
      "</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * buffed_stats.multistrike,
      100 * a -> composite_multistrike(),
      a -> composite_multistrike_rating() );
    j++;
    os.format(
      "<tr%s>\n"
      "<th class=\"left\">Damage / Heal Versatility</th>\n"
      "<td class=\"right\">%.2f%%</td>\n"
      "<td class=\"right\">%.2f%%</td>\n"
      "<td class=\"right\">%.0f</td>\n"
      "</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * buffed_stats.damage_versatility,
      100 * a -> composite_damage_versatility(),
      a -> composite_damage_versatility_rating() );
    j++;
    if ( a -> primary_role() == ROLE_TANK )
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Mitigation Versatility</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * buffed_stats.mitigation_versatility,
        100 * a -> composite_mitigation_versatility(),
        a -> composite_mitigation_versatility_rating() );
      j++;
    }
    if ( buffed_stats.manareg_per_second > 0 )
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">ManaReg per Second</th>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">0</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        buffed_stats.manareg_per_second,
        a -> mana_regen_per_second() );
    }
    j++;
    if ( buffed_stats.attack_power > 0 )
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Attack Power</th>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        buffed_stats.attack_power,
        a -> composite_melee_attack_power() * a -> composite_attack_power_multiplier(),
        a -> initial.stats.attack_power );
      j++;
    }
    os.format(
      "<tr%s>\n"
      "<th class=\"left\">Mastery</th>\n"
      "<td class=\"right\">%.2f%%</td>\n"
      "<td class=\"right\">%.2f%%</td>\n"
      "<td class=\"right\">%.0f</td>\n"
      "</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100.0 * buffed_stats.mastery_value,
      100.0 * a -> cache.mastery_value(),
      a -> composite_mastery_rating() );
    j++;
    if ( buffed_stats.mh_attack_expertise > 7.5 )
    {
      if ( a -> dual_wield() )
      {
        os.format(
          "<tr%s>\n"
          "<th class=\"left\">Expertise</th>\n"
          "<td class=\"right\">%.2f%% / %.2f%%</td>\n"
          "<td class=\"right\">%.2f%% / %.2f%% </td>\n"
          "<td class=\"right\">%.0f </td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          100 * buffed_stats.mh_attack_expertise,
          100 * buffed_stats.oh_attack_expertise,
          100 * a -> composite_melee_expertise( &( a -> main_hand_weapon ) ),
          100 * a -> composite_melee_expertise( &( a -> off_hand_weapon ) ),
          a -> composite_expertise_rating() );
        j++;
      }
      else
      {
        os.format(
          "<tr%s>\n"
          "<th class=\"left\">Expertise</th>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%% </td>\n"
          "<td class=\"right\">%.0f </td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          100 * buffed_stats.mh_attack_expertise,
          100 * a -> composite_melee_expertise( &( a -> main_hand_weapon ) ),
          a -> composite_expertise_rating() );
        j++;
      }
    }
    os.format(
      "<tr%s>\n"
      "<th class=\"left\">Armor</th>\n"
      "<td class=\"right\">%.0f</td>\n"
      "<td class=\"right\">%.0f</td>\n"
      "<td class=\"right\">%.0f</td>\n"
      "</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      buffed_stats.armor,
      a -> composite_armor(),
      a -> initial.stats.armor );
    j++;
    if ( buffed_stats.bonus_armor > 0 )
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Bonus Armor</th>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        buffed_stats.bonus_armor,
        a -> composite_bonus_armor(),
        a -> initial.stats.bonus_armor );
      j++;
    }
    if ( buffed_stats.run_speed > 0 )
    {
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Run Speed</th>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        buffed_stats.run_speed,
        a -> composite_run_speed(),
        a -> composite_speed_rating() );
      j++;
    }
    if ( a -> primary_role() == ROLE_TANK )
    {
      if ( buffed_stats.avoidance > 0 )
      {
        os.format(
          "<tr%s>\n"
          "<th class=\"left\">Avoidance</th>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          buffed_stats.avoidance,
          a -> composite_avoidance(),
          a -> composite_avoidance_rating() );
        j++;
      }
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Tank-Miss</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * buffed_stats.miss,
        100 * ( a -> cache.miss() ),
        0.0 );
      j++;
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Tank-Dodge</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * buffed_stats.dodge,
        100 * ( a -> composite_dodge() ),
        a -> composite_dodge_rating() );
      j++;
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Tank-Parry</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * buffed_stats.parry,
        100 * ( a -> composite_parry() ),
        a -> composite_parry_rating() );
      j++;
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Tank-Block</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * buffed_stats.block,
        100 * a -> composite_block(),
        a -> composite_block_rating() );
      j++;
      os.format(
        "<tr%s>\n"
        "<th class=\"left\">Tank-Crit</th>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * buffed_stats.crit,
        100 * a -> cache.crit_avoidance(),
        0.0 );
      j++;
    }

    os << "</table>\n"
       << "</div>\n"
       << "</div>\n";
  }
}


// print_html_talents_player ================================================

void print_html_talents( report::sc_html_stream& os, player_t* p )
{
  if ( p -> collected_data.fight_length.mean() > 0 )
  {
    os << "<div class=\"player-section talents\">\n"
       << "<h3 class=\"toggle\">Talents</h3>\n"
       << "<div class=\"toggle-content hide\">\n"
       << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th>Level</th>\n"
       << "<th></th>\n"
       << "<th></th>\n"
       << "<th></th>\n"
       << "</tr>\n";

    for ( uint32_t row = 0; row < MAX_TALENT_ROWS; row++ )
    {
      if ( row == 6 )
      {
        os.format(
          "<tr>\n"
          "<th class=\"left\">%d</th>\n",
          100 );
      }
      else
      {
        os.format(
          "<tr>\n"
          "<th class=\"left\">%d</th>\n",
          ( row + 1 ) * 15 );
      }
      for ( uint32_t col = 0; col < MAX_TALENT_COLS; col++ )
      {
        talent_data_t* t = talent_data_t::find( p -> type, row, col, p -> specialization(), p -> dbc.ptr );
        std::string name = "none";
        if ( t && t -> name_cstr() )
        {
          name = t -> name_cstr();
          if ( t -> specialization() != SPEC_NONE )
          {
            name += " (";
            name += util::specialization_string( t -> specialization() );
            name += ")";
          }
        }
        if ( p -> talent_points.has_row_col( row, col ) )
        {
          os.format(
            "<td class=\"filler\">%s</td>\n",
            name.c_str() );
        }
        else
        {
          os.format(
            "<td>%s</td>\n",
            name.c_str() );
        }
      }
      os << "</tr>\n";
    }

    os << "</table>\n"
       << "</div>\n"
       << "</div>\n";
  }
}

// print_html_player_scale_factor_table =====================================

void print_html_player_scale_factor_table( report::sc_html_stream& os, sim_t*, player_t* p, player_processed_report_information_t& ri, scale_metric_e sm )
{

  int colspan = static_cast< int >( p -> scaling_stats[ sm ].size() );
  std::vector<stat_e> scaling_stats = p -> scaling_stats[ sm ];

  os << "<table class=\"sc mt\">\n";

  os << "<tr>\n"
    << "<th colspan=\"" << util::to_string( 1 + colspan ) << "\"><a href=\"#help-scale-factors\" class=\"help\">Scale Factors for " << p -> scaling_for_metric( sm ).name << "</a></th>\n"
    << "</tr>\n";

  os << "<tr>\n"
    << "<th></th>\n";

  for ( size_t i = 0; i < scaling_stats.size(); i++ )
  {
    os.format(
      "<th>%s</th>\n",
      util::stat_type_abbrev( scaling_stats[ i ] ) );
  }
  if ( p -> sim -> scaling -> scale_lag )
  {
    os << "<th>ms Lag</th>\n";
    colspan++;
  }
  os << "</tr>\n";
  os << "<tr>\n"
    << "<th class=\"left\">Scale Factors</th>\n";

  for ( size_t i = 0; i < scaling_stats.size(); i++ )
  {
    if ( std::abs( p -> scaling[ sm ].get_stat( scaling_stats[ i ] ) ) > 1.0e5 )
      os.format(
      "<td>%.*e</td>\n",
      p -> sim -> report_precision,
      p -> scaling[ sm ].get_stat( scaling_stats[ i ] ) );
    else
      os.format(
      "<td>%.*f</td>\n",
      p -> sim -> report_precision,
      p -> scaling[ sm ].get_stat( scaling_stats[ i ] ) );

  }
  if ( p -> sim -> scaling -> scale_lag )
    os.format(
    "<td>%.*f</td>\n",
    p -> sim -> report_precision,
    p -> scaling_lag[ sm ] );
  os << "</tr>\n";
  os << "<tr>\n"
    << "<th class=\"left\">Normalized</th>\n";

  for ( size_t i = 0; i < scaling_stats.size(); i++ )
    os.format(
    "<td>%.*f</td>\n",
    p -> sim -> report_precision,
    p -> scaling_normalized[ sm ].get_stat( scaling_stats[ i ] ) );
  os << "</tr>\n";
  os << "<tr>\n"
    << "<th class=\"left\">Scale Deltas</th>\n";

  for ( size_t i = 0; i < scaling_stats.size(); i++ )
  {
    double value = p -> sim -> scaling -> stats.get_stat( scaling_stats[ i ] );
    std::string prefix;
    if ( p -> sim -> scaling -> center_scale_delta == 1 &&
      scaling_stats[ i ] != STAT_SPIRIT &&
      scaling_stats[ i ] != STAT_HIT_RATING &&
      scaling_stats[ i ] != STAT_EXPERTISE_RATING )
    {
      value /= 2;
      prefix = "+/- ";
    }

    os.format( "<td>%s%.0f</td>\n", prefix.c_str(), value );

  }
  if ( p -> sim -> scaling -> scale_lag )
    os << "<td>100</td>\n";
  os << "</tr>\n";
  os << "<tr>\n"
    << "<th class=\"left\">Error</th>\n";

  for ( size_t i = 0; i < scaling_stats.size(); i++ )
    if ( std::abs( p -> scaling[ sm ].get_stat( scaling_stats[ i ] ) ) > 1.0e5 )
      os.format(
      "<td>%.*e</td>\n",
      p -> sim -> report_precision,
      p -> scaling_error[ sm ].get_stat( scaling_stats[ i ] ) );
    else
      os.format(
      "<td>%.*f</td>\n",
      p -> sim -> report_precision,
      p -> scaling_error[ sm ].get_stat( scaling_stats[ i ] ) );

  if ( p -> sim -> scaling -> scale_lag )
    os.format(
    "<td>%.*f</td>\n",
    p -> sim -> report_precision,
    p -> scaling_lag_error[ sm ] );
  os << "</tr>\n";

  os.format(
    "<tr class=\"left\">\n"
    "<th>Gear Ranking</th>\n"
    "<td colspan=\"%i\" class=\"filler\">\n"
    "<ul class=\"float\">\n",
    colspan );
  if ( !ri.gear_weights_wowhead_std_link[ sm ].empty() )
    os.format(
    "<li><a href=\"%s\" class=\"ext\">wowhead</a></li>\n",
    ri.gear_weights_wowhead_std_link[ sm ].c_str() );
#if LOOTRANK_ENABLED == 1
  if ( !ri.gear_weights_lootrank_link[ sm ].empty() )
    os.format(
    "<li><a href=\"%s\" class=\"ext\">lootrank</a></li>\n",
    ri.gear_weights_lootrank_link[ sm ].c_str() );
#endif
  os << "</ul>\n";
  os << "</td>\n";
  os << "</tr>\n";

  // Optimizers section
  os.format(
    "<tr class=\"left\">\n"
    "<th>Optimizers</th>\n"
    "<td colspan=\"%i\" class=\"filler\">\n"
    "<ul class=\"float\">\n",
    colspan );

  // askmrrobot
  if ( !ri.gear_weights_askmrrobot_link[ sm ].empty() )
  {
    os.format(
      "<li><a href=\"%s\" class=\"ext\">askmrrobot</a></li>\n",
      ri.gear_weights_askmrrobot_link[ sm ].c_str() );
  }

  // close optimizers section
  os << "</ul>\n";
  os << "</td>\n";
  os << "</tr>\n";

  // Text Ranking
  os.format(
    "<tr class=\"left\">\n"
    "<th><a href=\"#help-scale-factor-ranking\" class=\"help\">Ranking</a></th>\n"
    "<td colspan=\"%i\" class=\"filler\">\n"
    "<ul class=\"float\">\n"
    "<li>",
    colspan );

  for ( size_t i = 0; i < scaling_stats.size(); i++ )
  {
    if ( i > 0 )
    {
      // holy hell this was hard to read - splitting this out into human-readable code
      double separation = fabs( p -> scaling[ sm ].get_stat( scaling_stats[ i - 1 ] ) - p -> scaling[ sm ].get_stat( scaling_stats[ i ] ) );
      double error_est = sqrt( p -> scaling_compare_error[ sm ].get_stat( scaling_stats[ i - 1 ] ) * p -> scaling_compare_error[ sm ].get_stat( scaling_stats[ i - 1 ] ) / 4 
                       + p -> scaling_compare_error[ sm ].get_stat( scaling_stats[ i ] ) * p -> scaling_compare_error[ sm ].get_stat( scaling_stats[ i ] ) / 4 );
      if ( separation > (error_est * 2 ) )
        os << " > ";
      else
        os << " ~= ";
    }

    os.format( "%s", util::stat_type_abbrev( scaling_stats[ i ] ) );
  }

  os << "</table>\n";

}


// print_html_player_scale_factors ==========================================

void print_html_player_scale_factors( report::sc_html_stream& os, sim_t* sim, player_t* p, player_processed_report_information_t& ri )
{

  if ( !p -> is_pet() )
  {
    if ( p -> sim -> scaling -> has_scale_factors() )
    {
      // print the scale factors for the metric the user is requesting
      scale_metric_e default_sm = p -> sim -> scaling -> scaling_metric;
      print_html_player_scale_factor_table( os, sim, p, ri, default_sm );

      // create a collapsable section containing scale factors for other metrics
      os << "<h3 class=\"toggle\">Scale Factors for other metrics</h3>\n"
         << "<div class=\"toggle-content hide\">\n";

      for ( scale_metric_e sm = SCALE_METRIC_DPS; sm < SCALE_METRIC_MAX; sm++ )
      {
        if ( sm != default_sm && sm != SCALE_METRIC_DEATHS )
          print_html_player_scale_factor_table( os, sim, p, ri, sm );
      }
      
      os << "</div>\n";
      
      // produce warning if sim iterations are too low to produce reliable scale factors
      if ( sim -> iterations < 10000 )
      {
        os << "<div class=\"alert\">\n"
           << "<h3>Warning</h3>\n"
           << "<p>Scale Factors generated using less than 10,000 iterations may vary significantly from run to run.</p>\n<br>";
        // Scale factor warnings:
        if ( sim -> scaling -> scale_factor_noise > 0 &&
             sim -> scaling -> scale_factor_noise < p -> scaling_lag_error[ default_sm ] / fabs( p -> scaling_lag[ default_sm ] ) )
          os.format( "<p>Player may have insufficient iterations (%d) to calculate scale factor for lag (error is >%.0f%% delta score)</p>\n",
                     sim -> iterations, sim -> scaling -> scale_factor_noise * 100.0 );
        for ( size_t i = 0; i < p -> scaling_stats[ default_sm ].size(); i++ )
        {
          scale_metric_e sm = p -> sim -> scaling -> scaling_metric;
          double value = p -> scaling[ sm ].get_stat( p -> scaling_stats[ default_sm ][ i ] );
          double error = p -> scaling_error[ sm ].get_stat( p -> scaling_stats[ default_sm ][ i ] );
          if ( sim -> scaling -> scale_factor_noise > 0 &&
               sim -> scaling -> scale_factor_noise < error / fabs( value ) )
            os.format( "<p>Player may have insufficient iterations (%d) to calculate scale factor for stat %s (error is >%.0f%% delta score)</p>\n",
                       sim -> iterations, util::stat_type_string( p -> scaling_stats[ default_sm ][ i ] ), sim -> scaling -> scale_factor_noise * 100.0 );
        }
        os   << "</div>\n";
      }
    }
  }
  os << "</div>\n"
     << "</div>\n";
}

bool print_html_sample_sequence_resource( const player_t* p, const player_collected_data_t::action_sequence_data_t* data, resource_e r )
{
  if ( r == RESOURCE_ECLIPSE && p -> specialization() == DRUID_BALANCE ) // Eclipse dips into the negatives.
    return true;

  if ( data -> resource_snapshot[ r ] < 0 )
    return false;

  if ( p -> role == ROLE_TANK && r == RESOURCE_HEALTH )
      return true;

  if ( p -> resources.is_infinite( r ) )
    return false;

  return true;
}

void print_html_sample_sequence_string_entry( report::sc_html_stream& os,
                                             player_collected_data_t::action_sequence_data_t* data,
                                             player_t* p,
                                             bool precombat = false )
{
  // Skip waiting on the condensed list
  if ( ! data -> action )
    return;

  std::string targetname = data -> action -> harmful ? data -> target -> name() : "none";
  std::array<char,100> time;
  if ( precombat )
    util::snformat( time.data(), time.size(), "Pre" );
  else
    util::snformat( time.data(), time.size(), "%d:%02d.%03d",
                    (int) data->time.total_minutes(),
                    (int) data->time.total_seconds() % 60,
                    (int) data->time.total_millis() % 1000 );
  os.format(
    "<span class=\"%s_seq_target_%s\" title=\"[%s] %s%s\n|",
    p -> name(),
    targetname.c_str(),
    time.data(),
    data -> action -> name(),
    ( targetname == "none" ? "" : " @ " + targetname ).c_str() );

  for ( resource_e r = RESOURCE_HEALTH; r < RESOURCE_MAX; ++r )
  {
    if ( print_html_sample_sequence_resource( p, data, r ) ) // Eclipse currently dips into the negatives.
    {
      if ( r == RESOURCE_HEALTH || r == RESOURCE_MANA )
        os.format( " %d%%", ( int ) ( ( data -> resource_snapshot[ r ] / data -> resource_max_snapshot[ r ] ) * 100 ) );
      else
        os.format( " %.1f", data -> resource_snapshot[ r ] );
      os.format( " %s |", util::resource_type_string( r ) );
    }
  }

  for ( size_t b = 0; b < data -> buff_list.size(); ++b )
  {
    buff_t* buff = data -> buff_list[ b ].first;
    int stacks = data -> buff_list[ b ].second;
    if ( ! buff -> constant )
    {
      os.format( "\n%s", buff -> name() );
      if ( stacks > 1 )
        os.format( "(%d)", stacks );
    }
  }

  os.format( "\">%c</span>", data -> action ? data -> action -> marker : 'W' );
}
// print_html_sample_sequence_table_entry =====================================

void print_html_sample_sequence_table_entry( report::sc_html_stream& os,
                                             player_collected_data_t::action_sequence_data_t* data,
                                             player_t* p,
                                             bool precombat = false )
{
  os << "<tr>\n";

  if ( precombat )
    os << "<td class=\"right\">Pre</td>\n";
  else
    os.format( "<td class=\"right\">%d:%02d.%03d</td>\n",
               ( int ) data -> time.total_minutes(),
               ( int ) data -> time.total_seconds() % 60,
               ( int ) data -> time.total_millis() % 1000
               );

  if ( data -> action )
  {
    os.format( "<td></td>\n"
               "<td class=\"left\">%s</td>\n"
               "<td></td>\n"
               "<td class=\"left\">%s</td>\n"
               "<td></td>\n",
               data -> action -> name(),
               data -> target -> name()
               );
  }
  else
  {
    os.format( "<td></td>\n"
               "<td class=\"left\">Waiting</td>\n"
               "<td></td>\n"
               "<td class=\"left\">%.3f sec</td>\n"
               "<td></td>\n",
               data -> wait_time.total_seconds() );
  }

  os.format( "<td class=\"left\">" );

  bool first = true;
  for ( resource_e r = RESOURCE_HEALTH; r < RESOURCE_MAX; ++r )
  {
    if ( print_html_sample_sequence_resource( p, data, r ) ) // Eclipse currently dips into the negatives.
    {
      if ( first )
        first = false;
      else
        os.format( " | " );

      os.format( " %.1f/%.0f: <b>%.0f%%",
                 data -> resource_snapshot[ r ],
                 data -> resource_max_snapshot[ r ],
                 data -> resource_snapshot[ r ] / data -> resource_max_snapshot[ r ] * 100.0 );
      os.format( " %s</b>", util::resource_type_string( r ) );
    }
  }

  os.format( "</td>\n<td></td>\n<td class=\"left\">" );

  first = true;
  for ( size_t b = 0; b < data -> buff_list.size(); ++b )
  {
    buff_t* buff = data -> buff_list[ b ].first;
    int stacks = data -> buff_list[ b ].second;

    if ( ! buff -> constant )
    {
      if ( first )
        first = false;
      else
        os.format( ", " );

      os.format( "%s", buff -> name() );
      if ( stacks > 1 )
        os.format( "(%d)", stacks );
    }
  }

  os.format( "</td>\n" );
}

// print_html_player_action_priority_list =====================================

void print_html_player_action_priority_list( report::sc_html_stream& os, sim_t* sim, player_t* p )
{
  os << "<div class=\"player-section action-priority-list\">\n"
     << "<h3 class=\"toggle\">Action Priority List</h3>\n"
     << "<div class=\"toggle-content hide\">\n";

  action_priority_list_t* alist = 0;

  for ( size_t i = 0; i < p -> action_list.size(); ++i )
  {
    action_t* a = p -> action_list[ i ];
    if ( a -> signature_str.empty() || ! a -> marker ) continue;

    if ( ! alist || a -> action_list -> name_str != alist -> name_str )
    {
      if ( alist )
      {
        os << "</table>\n";
      }

      //alist = p -> find_action_priority_list( a -> action_list );
      alist = a -> action_list;

      if ( ! alist -> used ) continue;

      std::string als = util::encode_html( ( alist -> name_str == "default" ) ? "Default action list" : ( "actions." + alist -> name_str ).c_str() );
      if ( ! alist -> action_list_comment_str.empty() )
        als += "&#160;<small><em>" + util::encode_html( alist -> action_list_comment_str.c_str() ) + "</em></small>";
      os.format(
        "<table class=\"sc\">\n"
        "<tr>\n"
        "<th class=\"right\"></th>\n"
        "<th class=\"right\"></th>\n"
        "<th class=\"left\">%s</th>\n"
        "</tr>\n"
        "<tr>\n"
        "<th class=\"right\">#</th>\n"
        "<th class=\"right\">count</th>\n"
        "<th class=\"left\">action,conditions</th>\n"
        "</tr>\n",
        als.c_str() );
    }

    if ( ! alist -> used ) continue;

    os << "<tr";
    if ( ( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    std::string as = util::encode_html( a -> signature -> action_.c_str() );
    if ( ! a -> signature -> comment_.empty() )
      as += "<br/><small><em>" + util::encode_html( a -> signature -> comment_.c_str() ) + "</em></small>";

    os.format(
      "<th class=\"right\" style=\"vertical-align:top\">%c</th>\n"
      "<td class=\"left\" style=\"vertical-align:top\">%.2f</td>\n"
      "<td class=\"left\">%s</td>\n"
      "</tr>\n",
      a -> marker,
      a -> total_executions / ( double ) sim -> iterations,
      as.c_str() );
  }

  if ( alist )
  {
    os << "</table>\n";
  }

  // Sample Sequences

  if ( ! p -> collected_data.action_sequence.empty() )
  {

    std::vector<std::string> targets;

    targets.push_back( "none" );
    targets.push_back( p -> target -> name() );

    for ( size_t i = 0; i < p -> collected_data.action_sequence.size(); ++i )
    {
      player_collected_data_t::action_sequence_data_t* data = p -> collected_data.action_sequence[ i ];
      if ( ! data -> action || ! data -> action -> harmful ) continue;
      bool found = false;
      for ( size_t j = 0; j < targets.size(); ++j )
      {
        if ( targets[ j ] == data -> target -> name() )
        {
          found = true;
          break;
        }
      }
      if ( ! found ) targets.push_back( data -> target -> name() );
    }

    // Sample Sequence (text string)

    os << "<div class=\"subsection subsection-small\">\n"
       << "<h4>Sample Sequence</h4>\n"
       << "<div class=\"force-wrap mono\">\n";

    os << "<style type=\"text/css\" media=\"all\">\n";

    char colors[12][7] = { "eeeeee", "ffffff", "ff5555", "55ff55", "5555ff", "ffff55", "55ffff", "ff9999", "99ff99", "9999ff", "ffff99", "99ffff" };

    int j = 0;

    for ( size_t i = 0; i < targets.size(); ++i )
    {
      if ( j == 12 ) j = 2;
      os.format(
        ".%s_seq_target_%s { color: #%s; }\n",
        p -> name(),
        targets[ i ].c_str(),
        colors[ j ] );
      j++;
    }


    os << "</style>\n";

    for ( size_t i = 0; i < p -> collected_data.action_sequence_precombat.size(); ++i )
    {
      player_collected_data_t::action_sequence_data_t* data = p -> collected_data.action_sequence_precombat[ i ];

      print_html_sample_sequence_string_entry( os, data, p, true );
    }

    for ( size_t i = 0; i < p -> collected_data.action_sequence.size(); ++i )
    {
      player_collected_data_t::action_sequence_data_t* data = p -> collected_data.action_sequence[ i ];

      print_html_sample_sequence_string_entry( os, data, p );
    }

    os << "\n</div>\n"
       << "</div>\n";

    // Sample Sequence (table)
    
    // define collapsable section
    os << "<h3 class=\"toggle\">Sample Sequence Table</h3>\n"
       << "<div class=\"toggle-content hide\">\n";

    // create table header
    os.format(
      "<table class=\"sc\">\n"
      "<tr>\n"
      "<th class=\"center\">time</th>\n"
      "<th></th>\n"
      "<th class=\"center\">name</th>\n"
      "<th></th>\n"
      "<th class=\"center\">target</th>\n"
      "<th></th>\n"
      "<th class=\"center\">resources</th>\n"
      "<th></th>\n"
      "<th class=\"center\">buffs</th>\n"
      "</tr>\n");
    
    for ( size_t i = 0; i < p -> collected_data.action_sequence_precombat.size(); ++i )
    {
      player_collected_data_t::action_sequence_data_t* data = p -> collected_data.action_sequence_precombat[ i ];
      
      print_html_sample_sequence_table_entry( os, data, p, true );      
    }

    for ( size_t i = 0; i < p -> collected_data.action_sequence.size(); ++i )
    {
      player_collected_data_t::action_sequence_data_t* data = p -> collected_data.action_sequence[ i ];
      
      print_html_sample_sequence_table_entry( os, data, p );
    }

    // close table
    os << "</table>\n";

    // close collapsable section
    os << "</div>\n";
  }

  // End Action Priority List section
  os << "</div>\n"
     << "</div>\n";

}

// print_html_player_statistics =============================================

void print_html_player_statistics( report::sc_html_stream& os, player_t* p, player_processed_report_information_t& ri )
{
  // Statistics & Data Analysis

  os << "<div class=\"player-section gains\">\n"
     "<h3 class=\"toggle\">Statistics & Data Analysis</h3>\n"
     "<div class=\"toggle-content hide\">\n"
     "<table class=\"sc\">\n";

  int sd_counter = 0;
  report::print_html_sample_data( os, p -> sim, p -> collected_data.fight_length, "Fight Length", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.dps, "DPS", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.prioritydps, "Priority Target DPS", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.dpse, "DPS(e)", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.dmg, "Damage", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.dtps, "DTPS", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.hps, "HPS", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.hpse, "HPS(e)", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.heal, "Heal", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.htps, "HTPS", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.theck_meloree_index, "TMI", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.effective_theck_meloree_index, "ETMI", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.max_spike_amount, "MSD", sd_counter );

  for ( size_t i = 0; i < p -> sample_data_list.size(); ++i )
  {
    report::print_html_sample_data( os, p -> sim, *p -> sample_data_list[ i ], p -> sample_data_list[ i ] -> name_str, sd_counter );
  }
  os << "<tr>\n"
     "<td>\n";
  if ( ! ri.timeline_dps_error_chart.empty() )
    os << "<img src=\"" << ri.timeline_dps_error_chart << "\" alt=\"Timeline DPS Error Chart\" />\n";

  if ( ! ri.dps_error_chart.empty() )
    os << "<img src=\"" << ri.dps_error_chart << "\" alt=\"DPS Error Chart\" />\n";

  os << "</td>\n"
     "</tr>\n"
     "</table>\n"
     "</div>\n"
     "</div>\n";
}

void print_html_gain( report::sc_html_stream& os, gain_t* g, std::array<double, RESOURCE_MAX>& total_gains, int& j, bool report_overflow = true )
{
  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( g -> actual[ i ] != 0 || g -> overflow[ i ] != 0 )
    {
      double overflow_pct = 100.0 * g -> overflow[ i ] / ( g -> actual[ i ] + g -> overflow[ i ] );
      os << "<tr";
      if ( j & 1 )
      {
        os << " class=\"odd\"";
      }
      ++j;
      os << ">\n";
      os << "<td class=\"left\">" << g -> name() << "</td>\n";
      os << "<td class=\"left\">" << util::inverse_tokenize( util::resource_type_string( i ) ) << "</td>\n";
      os << "<td class=\"right\">" << g -> count[ i ] << "</td>\n";
      os.format( "<td class=\"right\">%.2f (%.2f%%)</td>\n",
                        g -> actual[ i ], g -> actual[ i ] ? g -> actual[ i ] / total_gains[ i ] * 100.0 : 0.0 );
      os << "<td class=\"right\">" << g -> actual[ i ] / g -> count[ i ] << "</td>\n";

      if ( report_overflow )
      {
        os << "<td class=\"right\">" << g -> overflow[ i ] << "</td>\n";
        os << "<td class=\"right\">" << overflow_pct << "%</td>\n";
      }
      os << "</tr>\n";
    }
  }
}

void get_total_player_gains( player_t& p, std::array<double, RESOURCE_MAX>& total_gains )
{
  for ( size_t i = 0; i < p.gain_list.size(); ++i )
  {
    // Wish std::array had += operator overload!
    for ( size_t j = 0; j < total_gains.size(); ++j )
      total_gains[ j ] += p.gain_list[ i ] -> actual[ j ];
  }
}
// print_html_player_resources ==============================================

void print_html_player_resources( report::sc_html_stream& os, player_t* p, player_processed_report_information_t& ri )
{

  // Resources Section

  os << "<div class=\"player-section gains\">\n";
  os << "<h3 class=\"toggle open\">Resources</h3>\n";
  os << "<div class=\"toggle-content\">\n";
  os << "<table class=\"sc mt\">\n";
  os << "<tr>\n";
  os << "<th>Resource Usage</th>\n";
  os << "<th>Type</th>\n";
  os << "<th>Count</th>\n";
  os << "<th>Total</th>\n";
  os << "<th>Average</th>\n";
  os << "<th>RPE</th>\n";
  os << "<th>APR</th>\n";
  os << "</tr>\n";

  os << "<tr>\n";
  os << "<th class=\"left small\">" << util::encode_html( p -> name() ) << "</th>\n";
  os << "<td colspan=\"6\" class=\"filler\"></td>\n";
  os << "</tr>\n";

  int k = 0;
  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    stats_t* s = p -> stats_list[ i ];
    if ( s -> rpe_sum > 0 )
    {
      k = print_html_action_resource( os, s, k );
    }
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    pet_t* pet = p -> pet_list[ i ];
    bool first = true;

    for ( size_t j = 0; j < pet -> stats_list.size(); ++j )
    {
      stats_t* s = pet -> stats_list[ j ];
      if ( s -> rpe_sum > 0 )
      {
        if ( first )
        {
          first = false;
          os << "<tr>\n";
          os << "<th class=\"left small\">pet - " << util::encode_html( pet -> name_str ) << "</th>\n";
          os << "<td colspan=\"6\" class=\"filler\"></td>\n";
          os << "</tr>\n";
        }
        k = print_html_action_resource( os, s, k );
      }
    }
  }

  os << "</table>\n";

  std::array<double, RESOURCE_MAX> total_player_gains = std::array<double, RESOURCE_MAX>();
  get_total_player_gains( *p, total_player_gains );
  bool has_gains = false;
  for ( size_t i = 0; i < RESOURCE_MAX; i++ )
  {
    if ( total_player_gains[ i ] > 0 )
    {
      has_gains = true;
      break;
    }
  }

  if ( has_gains )
  {
    // Resource Gains Section
    os << "<table class=\"sc\">\n";
    os << "<tr>\n";
    os << "<th>Resource Gains</th>\n";
    os << "<th>Type</th>\n";
    os << "<th>Count</th>\n";
    os << "<th>Total</th>\n";
    os << "<th>Average</th>\n";
    os << "<th colspan=\"2\">Overflow</th>\n";
    os << "</tr>\n";

    int j = 0;

    for ( size_t i = 0; i < p -> gain_list.size(); ++i )
    {
      gain_t* g = p -> gain_list[ i ];
      print_html_gain( os, g, total_player_gains, j );
    }
    for ( size_t i = 0; i < p -> pet_list.size(); ++i )
    {
      pet_t* pet = p -> pet_list[ i ];
      if ( pet -> collected_data.fight_length.mean() <= 0 ) continue;
      bool first = true;
      std::array<double, RESOURCE_MAX> total_pet_gains = std::array<double, RESOURCE_MAX>();
      get_total_player_gains( *pet, total_pet_gains );
      for ( size_t m = 0; m < pet -> gain_list.size(); ++m )
      {
        gain_t* g = pet -> gain_list[ m ];
        if ( first )
        {
          bool found = false;
          for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; r++ )
          {
            if ( g -> actual[ r ] != 0 || g -> overflow[ r ] != 0 )
            {
              found = true;
              break;
            }
          }
          if ( found )
          {
            first = false;
            os << "<tr>\n";
            os << "<th>pet - " << util::encode_html( pet -> name_str ) << "</th>\n";
            os << "<td colspan=\"6\" class=\"filler\"></td>\n";
            os << "</tr>\n";
          }
        }
        print_html_gain( os, g, total_pet_gains, j );
      }
    }
    os << "</table>\n";
  }

  // Resource Consumption Section
  os << "<table class=\"sc\">\n";
  os << "<tr>\n";
  os << "<th>Resource</th>\n";
  os << "<th>RPS-Gain</th>\n";
  os << "<th>RPS-Loss</th>\n";
  os << "</tr>\n";
  int j = 0;
  for ( resource_e rt = RESOURCE_NONE; rt < RESOURCE_MAX; ++rt )
  {
    double rps_gain = p -> collected_data.resource_gained[ rt ].mean() / p -> collected_data.fight_length.mean();
    double rps_loss = p -> collected_data.resource_lost[ rt ].mean() / p -> collected_data.fight_length.mean();
    if ( rps_gain <= 0 && rps_loss <= 0 )
      continue;

    os << "<tr";
    if ( j & 1 )
    {
      os << " class=\"odd\"";
    }
    ++j;
    os << ">\n";
    os << "<td class=\"left\">" << util::inverse_tokenize( util::resource_type_string( rt ) ) << "</td>\n";
    os << "<td class=\"right\">" << rps_gain << "</td>\n";
    os << "<td class=\"right\">" << rps_loss << "</td>\n";
    os << "</tr>\n";
  }
  os << "</table>\n";

  // Resource End Section
  os << "<table class=\"sc\">\n";
  os << "<tr>\n";
  os << "<th>Combat End Resource</th>\n";
  os << "<th> Mean </th>\n";
  os << "<th> Min </th>\n";
  os << "<th> Max </th>\n";
  os << "</tr>\n";
  j = 0;
  for ( resource_e rt = RESOURCE_NONE; rt < RESOURCE_MAX; ++rt )
  {
    if ( p -> resources.base[ rt ] <= 0 )
      continue;

    os << "<tr";
    if ( j & 1 )
    {
      os << " class=\"odd\"";
    }
    ++j;
    os << ">\n";
    os << "<td class=\"left\">" << util::inverse_tokenize( util::resource_type_string( rt ) ) << "</td>\n";
    os << "<td class=\"right\">" << p -> collected_data.combat_end_resource[ rt ].mean() << "</td>\n";
    os << "<td class=\"right\">" << p -> collected_data.combat_end_resource[ rt ].min() << "</td>\n";
    os << "<td class=\"right\">" << p -> collected_data.combat_end_resource[ rt ].max() << "</td>\n";
    os << "</tr>\n";
  }
  os << "</table>\n";


  os << "<div class=\"charts charts-left\">\n";
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    // hack hack. don't display RESOURCE_RUNE_<TYPE> yet. only shown in tabular data.  WiP
    if ( r == RESOURCE_RUNE_BLOOD || r == RESOURCE_RUNE_UNHOLY || r == RESOURCE_RUNE_FROST ) continue;
    double total_gain = 0;
    for ( size_t i = 0; i < p -> gain_list.size(); ++i )
    {
      gain_t* g = p -> gain_list[ i ];
      if ( g -> actual[ r ] > 0 )
        total_gain += g -> actual[ r ];
    }

    if ( total_gain > 0 )
    {
      if ( ! ri.gains_chart[ r ].empty() )
      {
        os << "<img src=\"" << ri.gains_chart[ r ] << "\" alt=\"Resource Gains Chart\" />\n";
      }
    }
  }
  os << "</div>\n";


  os << "<div class=\"charts\">\n";
  for ( unsigned r = RESOURCE_NONE + 1; r < RESOURCE_MAX; r++ )
  {
    if ( p -> resources.max[ r ] > 0 && ! ri.timeline_resource_chart[ r ].empty() )
    {
      os << "<img src=\"" << ri.timeline_resource_chart[ r ] << "\" alt=\"Resource Timeline Chart\" />\n";
    }
  }
  if ( p -> primary_role() == ROLE_TANK ) // Experimental, restrict to tanks for now
  {
    if ( ! ri.health_change_chart.empty() )
      os << "<img src=\"" << ri.health_change_chart << "\" alt=\"Health Change Timeline Chart\" />\n";
    if ( ! ri.health_change_sliding_chart.empty() )
      os << "<img src=\"" << ri.health_change_sliding_chart << "\" alt=\"Health Change Sliding Timeline Chart\" />\n";

    if ( ! p -> is_enemy() )
    {
      // Tmp Debug Visualization
      histogram tmi_hist;
      tmi_hist.create_histogram( p -> collected_data.theck_meloree_index, 50 );
      std::string dist_chart = chart::distribution( p -> sim -> print_styles, tmi_hist.data(), "TMI", p -> collected_data.theck_meloree_index.mean(), tmi_hist.min(), tmi_hist.max() );
      os << "<img src=\"" << dist_chart << "\" alt=\"Theck meloree distribution Chart\" />\n";
    }
  }
  os << "</div>\n";
  os << "<div class=\"clear\"></div>\n";

  os << "</div>\n";
  os << "</div>\n";
}

// print_html_player_charts =================================================

void print_html_player_charts( report::sc_html_stream& os, sim_t* sim, player_t* p, player_processed_report_information_t& ri )
{
  size_t num_players = sim -> players_by_name.size();

  os << "<div class=\"player-section\">\n"
     << "<h3 class=\"toggle open\">Charts</h3>\n"
     << "<div class=\"toggle-content\">\n"
     << "<div class=\"charts charts-left\">\n";

  if ( ! ri.action_dpet_chart.empty() )
  {
    std::string chart_str;
    if ( num_players == 1 )
      chart_str = "<img src=\"" + ri.action_dpet_chart + "\" alt=\"Action DPET Chart\" />\n";
    else
      chart_str = "<span class=\"chart-action-dpet\" title=\"Action DPET Chart\">" + ri.action_dpet_chart + "</span>\n";
    os << chart_str;
  }

  if ( ! ri.action_dmg_chart.empty() )
  {
    std::string chart_str;
    if ( num_players == 1 )
      chart_str = "<img src=\"" + ri.action_dmg_chart + "\" alt=\"Action Damage Chart\" />\n";
    else
      chart_str = "<span class=\"chart-action-dmg\" title=\"Action Damage Chart\">" + ri.action_dmg_chart + "</span>\n";
    os << chart_str;
  }

  if ( ! ri.scaling_dps_chart.empty() )
  {
    std::string chart_str;
    if ( num_players == 1 )
      chart_str = "<img src=\"" + ri.scaling_dps_chart + "\" alt=\"Scaling DPS Chart\" />\n";
    else
      chart_str = "<span class=\"chart-scaling-dps\" title=\"Scaling DPS Chart\">" + ri.scaling_dps_chart + "</span>\n";
    os << chart_str;
  }

  sc_timeline_t timeline_dps_taken;
  p -> collected_data.timeline_dmg_taken.build_derivative_timeline( timeline_dps_taken );
  std::string timeline_dps_takenchart = chart::timeline( p, timeline_dps_taken.data(), "dps_taken", timeline_dps_taken.mean(), "FDD017" );
  if ( ! timeline_dps_takenchart.empty() )
  {
    os << "<img src=\"" << timeline_dps_takenchart << "\" alt=\"DPS Taken Timeline Chart\" />\n";
  }

  os << "</div>\n"
     << "<div class=\"charts\">\n";

  if ( ! ri.reforge_dps_chart.empty() )
  {
    std::string chart_str;
    if ( p -> sim -> reforge_plot -> reforge_plot_stat_indices.size() == 2 )
    {
      if ( ri.reforge_dps_chart.length() < 2000 )
      {
        if ( num_players == 1 )
          chart_str = "<img src=\"" + ri.reforge_dps_chart + "\" alt=\"Reforge DPS Chart\" />\n";
        else
          chart_str = "<span class=\"chart-reforge-dps\" title=\"Reforge DPS Chart\">" + ri.reforge_dps_chart + "</span>\n";
      }
      else
        os << "<p> Reforge Chart: Can't display charts with more than 2000 characters.</p>\n";
    }
    else
    {
      if ( true )
        chart_str = "" + ri.reforge_dps_chart;
      else
      {
        if ( num_players == 1 )
          chart_str = "<iframe>" + ri.reforge_dps_chart + "</iframe>\n";
        else
          chart_str = "<span class=\"chart-reforge-dps\" title=\"Reforge DPS Chart\">" + ri.reforge_dps_chart + "</span>\n";
      }
    }
    if ( ! chart_str.empty() )
      os << chart_str;
  }

  if ( ! ri.scale_factors_chart.empty() )
  {
    std::string chart_str;
    if ( num_players == 1 )
      chart_str = "<img src=\"" + ri.scale_factors_chart + "\" alt=\"Scale Factors Chart\" />\n";
    else
      chart_str = "<span class=\"chart-scale-factors\" title=\"Scale Factors Chart\">" + ri.scale_factors_chart + "</span>\n";
    os << chart_str;
  }

  if ( ! ri.timeline_dps_chart.empty() )
  {
    std::string chart_str;
    if ( num_players == 1 )
      chart_str = "<img src=\"" + ri.timeline_dps_chart + "\" alt=\"DPS Timeline Chart\" />\n";
    else
      chart_str = "<span class=\"chart-timeline-dps\" title=\"DPS Timeline Chart\">" + ri.timeline_dps_chart + "</span>\n";
    os << chart_str;
  }

  std::string resolve_timeline_chart = chart::timeline( p,
                                                        p -> collected_data.resolve_timeline.merged_timeline.data(),
                                                        "Resolve",
                                                        p -> collected_data.resolve_timeline.merged_timeline.mean(),
                                                        "ff0000",
                                                        static_cast<size_t>( p -> collected_data.fight_length.max() ) );
  if ( ! resolve_timeline_chart.empty() )
  {
    os << "<img src=\"" << resolve_timeline_chart << "\" alt=\"Resolve Timeline Chart\" />\n";
  }

  if ( ! ri.distribution_dps_chart.empty() )
  {
    std::string chart_str;
    if ( num_players == 1 )
      chart_str = "<img src=\"" + ri.distribution_dps_chart + "\" alt=\"DPS Distribution Chart\" />\n";
    else
      chart_str = "<span class=\"chart-distribution-dps\" title=\"DPS Distribution Chart\">" + ri.distribution_dps_chart + "</span>\n";
    os << chart_str;
  }

  if ( ! ri.time_spent_chart.empty() )
  {
    std::string chart_str;
    if ( num_players == 1 )
      chart_str = "<img src=\"" + ri.time_spent_chart + "\" alt=\"Time Spent Chart\" />\n";
    else
      chart_str = "<span class=\"chart-time-spent\" title=\"Time Spent Chart\">" + ri.time_spent_chart + "</span>\n";
    os << chart_str;
  }

  for ( size_t i = 0; i < ri.timeline_stat_chart.size(); ++i )
  {
    if ( ri.timeline_stat_chart[ i ].length() > 0 )
    {
      std::string chart_str;
      if ( num_players == 1 )
        chart_str = "<img src=\"" + ri.timeline_stat_chart[ i ] + "\" alt=\"Stat Chart\" />\n";
      else
        chart_str = "<span class=\"chart-scaling-dps\" title=\"Stat Chart\">" + ri.timeline_stat_chart[ i ] + "</span>\n";
      os << chart_str;
    }
  }

  os << "</div>\n"
     << "<div class=\"clear\"></div>\n"
     << "</div>\n"
     << "</div>\n";
}

// This function MUST accept non-player buffs as well!
void print_html_player_buff( report::sc_html_stream& os, buff_t* b, int report_details, size_t i, bool constant_buffs = false )
{
  std::string buff_name;
  assert( b );
  if ( ! b ) return; // For release builds.

  if ( b -> player && b -> player -> is_pet() )
  {
    buff_name += b -> player -> name_str + '-';
  }
  buff_name += b -> name_str.c_str();

  os << "<tr";
  if ( i & 1 )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  if ( report_details )
  {
    wowhead::wowhead_e domain = SC_BETA ? wowhead::BETA : wowhead::LIVE;
    if ( ! SC_BETA )
    {
      if ( b -> player )
        domain = b -> player -> dbc.ptr ? wowhead::PTR : wowhead::LIVE;
      else
        domain = b -> sim -> dbc.ptr ? wowhead::PTR : wowhead::LIVE;
    }

    buff_name = wowhead::decorated_buff_name( buff_name, b, domain );
    os.format( "<td class=\"left\"><span class=\"toggle-details\">%s</span></td>\n", buff_name.c_str() );
  }
  else
    os.format( "<td class=\"left\">%s</td>\n", buff_name.c_str() );

  if ( !constant_buffs )
    os.format(
      "<td class=\"right\">%.1f</td>\n"
      "<td class=\"right\">%.1f</td>\n"
      "<td class=\"right\">%.1fsec</td>\n"
      "<td class=\"right\">%.1fsec</td>\n"
      "<td class=\"right\">%.2f%%</td>\n"
      "<td class=\"right\">%.2f%%</td>\n"
      "<td class=\"right\">%.1f(%.1f)</td>\n",
      b -> avg_start.pretty_mean(),
      b -> avg_refresh.pretty_mean(),
      b -> start_intervals.pretty_mean(),
      b -> trigger_intervals.pretty_mean(),
      b -> uptime_pct.pretty_mean(),
      ( b -> benefit_pct.sum() > 0 ? b -> benefit_pct.pretty_mean() : b -> uptime_pct.pretty_mean() ) ,
      b -> avg_overflow_count.mean(),
      b -> avg_overflow_total.mean() );


  os << "</tr>\n";


  if ( report_details )
  {
    os.format(
      "<tr class=\"details hide\">\n"
      "<td colspan=\"%d\" class=\"filler\">\n"
      "<table><tr>\n"
      "<td style=\"vertical-align: top;\" class=\"filler\">\n"
      "<h4>Buff details</h4>\n"
      "<ul>\n"
      "<li><span class=\"label\">buff initial source:</span>%s</li>\n"
      "<li><span class=\"label\">cooldown name:</span>%s</li>\n"
      "<li><span class=\"label\">max_stacks:</span>%.i</li>\n"
      "<li><span class=\"label\">duration:</span>%.2f</li>\n"
      "<li><span class=\"label\">cooldown:</span>%.2f</li>\n"
      "<li><span class=\"label\">default_chance:</span>%.2f%%</li>\n"
      "<li><span class=\"label\">default_value:</span>%.2f</li>\n"
      "</ul>\n",
      b -> constant ? 1 : 7,
      b -> source ? util::encode_html( b -> source -> name() ).c_str() : "",
      b -> cooldown -> name_str.c_str(),
      b -> max_stack(),
      b -> buff_duration.total_seconds(),
      b -> cooldown -> duration.total_seconds(),
      b -> default_chance * 100,
      b -> default_value );



    if ( stat_buff_t* stat_buff = dynamic_cast<stat_buff_t*>( b ) )
    {
      os << "<h4>Stat Buff details</h4>\n"
         << "<ul>\n";

      for ( size_t j = 0; j < stat_buff -> stats.size(); ++j )
      {
        os.format(
          "<li><span class=\"label\">stat:</span>%s</li>\n"
          "<li><span class=\"label\">amount:</span>%.2f</li>\n",
          util::stat_type_string( stat_buff -> stats[ j ].stat ),
          stat_buff -> stats[ j ].amount );
      }
      os << "</ul>\n";
    }

    os << "<h4>Stack Uptimes</h4>\n"
       << "<ul>\n";
    for ( unsigned int j = 0; j < b -> stack_uptime.size(); j++ )
    {
      double uptime = b -> stack_uptime[ j ] -> uptime_sum.mean();
      if ( uptime > 0 )
      {
        os.format(
          "<li><span class=\"label\">%s_%d:</span>%.2f%%</li>\n",
          b -> name_str.c_str(), j,
          uptime * 100.0 );
      }
    }
    os << "</ul>\n";

    if ( b -> trigger_pct.mean() > 0 )
    {
      os << "<h4>Trigger Attempt Success</h4>\n"
         << "<ul>\n";
      os.format(
        "<li><span class=\"label\">trigger_pct:</span>%.2f%%</li>\n",
        b -> trigger_pct.mean() );
      os << "</ul>\n";
    }
    os  << "</td>\n";

    // Spelldata
    if ( b -> data().ok() )
    {
      os.format(
        "<td style=\"vertical-align: top;\" class=\"filler\">\n"
        "<h4>Spelldata details</h4>\n"
        "<ul>\n"
        "<li><span class=\"label\">id:</span>%i</li>\n"
        "<li><span class=\"label\">name:</span>%s</li>\n"
        "<li><span class=\"label\">tooltip:</span><span class=\"tooltip\">%s</span></li>\n"
        "<li><span class=\"label\">description:</span><span class=\"tooltip\">%s</span></li>\n"
        "<li><span class=\"label\">max_stacks:</span>%.i</li>\n"
        "<li><span class=\"label\">duration:</span>%.2f</li>\n"
        "<li><span class=\"label\">cooldown:</span>%.2f</li>\n"
        "<li><span class=\"label\">default_chance:</span>%.2f%%</li>\n"
        "</ul>\n"
        "</td>\n",
        b -> data().id(),
        b -> data().name_cstr(),
        b -> player ? util::encode_html( pretty_spell_text( b -> data(), b -> data().tooltip(), *b -> player ) ).c_str() : b -> data().tooltip(),
        b -> player ? util::encode_html( pretty_spell_text( b -> data(), b -> data().desc(), *b -> player ) ).c_str() : b -> data().desc(),
        b -> data().max_stacks(),
        b -> data().duration().total_seconds(),
        b -> data().cooldown().total_seconds(),
        b -> data().proc_chance() * 100 );
    }
    os << "</tr>";

    if ( ! b -> constant && ! b -> overridden && b -> sim -> buff_uptime_timeline )
    {
      std::string uptime_chart = chart::timeline( b -> player, b -> uptime_array.data(), "Average Uptime", 0, "ff0000", static_cast<size_t>( b -> sim -> simulation_length.max() ) );
      if ( ! uptime_chart.empty() )
      {
        os << "<tr><td colspan=\"2\" class=\"filler\"><img src=\"" << uptime_chart << "\" alt=\"Average Uptime Timeline Chart\" />\n</td></tr>\n";
      }
    }
    os << "</table></td>\n";

    os << "</tr>\n";
  }
}

// print_html_player_buffs ==================================================

void print_html_player_buffs( report::sc_html_stream& os, player_t* p, player_processed_report_information_t& ri )
{
  // Buff Section
  os << "<div class=\"player-section buffs\">\n"
     << "<h3 class=\"toggle open\">Buffs</h3>\n"
     << "<div class=\"toggle-content\">\n";

  // Dynamic Buffs table
  os << "<table class=\"sc mb\">\n"
     << "<tr>\n"
     << "<th class=\"left\"><a href=\"#help-dynamic-buffs\" class=\"help\">Dynamic Buffs</a></th>\n"
     << "<th>Start</th>\n"
     << "<th>Refresh</th>\n"
     << "<th>Interval</th>\n"
     << "<th>Trigger</th>\n"
     << "<th>Up-Time</th>\n"
     << "<th>Benefit</th>\n"
     << "<th>Overflow</th>\n"
     << "</tr>\n";

  for ( size_t i = 0; i < ri.dynamic_buffs.size(); i++ )
  {
    buff_t* b = ri.dynamic_buffs[ i ];

    print_html_player_buff( os, b, p -> sim -> report_details, i );
  }
  os << "</table>\n";

  // constant buffs
  if ( !p -> is_pet() )
  {
    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th class=\"left\"><a href=\"#help-constant-buffs\" class=\"help\">Constant Buffs</a></th>\n"
       << "</tr>\n";
    for ( size_t i = 0; i < ri.constant_buffs.size(); i++ )
    {
      buff_t* b = ri.constant_buffs[ i ];

      print_html_player_buff( os, b, p -> sim->report_details, i, true );
    }
    os << "</table>\n";
  }


  os << "</div>\n"
     << "</div>\n";

}

void print_html_player_custom_section( report::sc_html_stream& os, player_t* p, player_processed_report_information_t& /*ri*/ )
{

  p->report_extension -> html_customsection( os );


}

// print_html_player ========================================================

void print_html_player_description( report::sc_html_stream& os, sim_t* sim, player_t* p, int j )
{
  int num_players = ( int ) sim -> players_by_name.size();

  // Player Description
  os << "<div id=\"player" << p -> index << "\" class=\"player section";
  if ( num_players > 1 && j == 0 && ! sim -> scaling -> has_scale_factors() && ! p -> is_enemy() )
  {
    os << " grouped-first";
  }
  else if ( p -> is_enemy() && j == ( int ) sim -> targets_by_name.size() - 1 )
  {
    os << " final grouped-last";
  }
  else if ( num_players == 1 )
  {
    os << " section-open";
  }
  os << "\">\n";

  if ( ! p -> report_information.thumbnail_url.empty() )
  {
    os.format(
      "<a href=\"%s\" class=\"toggle-thumbnail ext%s\"><img src=\"%s\" alt=\"%s\" class=\"player-thumbnail\"/></a>\n",
      p -> origin_str.c_str(), ( num_players == 1 ) ? "" : " hide",
      p -> report_information.thumbnail_url.c_str(), p -> name_str.c_str() );
  }

  os << "<h2 class=\"toggle";
  if ( num_players == 1 )
  {
    os << " open";
  }

  const std::string n = util::encode_html( p -> name() );
  if ( ( p -> collected_data.dps.mean() >= p -> collected_data.hps.mean() && sim -> num_enemies > 1 ) || (  p -> primary_role() == ROLE_TANK && sim -> num_enemies > 1 ) )
  {
    os.format( "\">%s&#160;:&#160;%.0f dps, %.0f dps to main target",
      n.c_str(),
      p -> collected_data.dps.mean(),
      p -> collected_data.prioritydps.mean() );
  }
  else if ( p -> collected_data.dps.mean() >= p -> collected_data.hps.mean() || p -> primary_role() == ROLE_TANK )
  {
    os.format( "\">%s&#160;:&#160;%.0f dps",
      n.c_str(),
      p -> collected_data.dps.mean() );
  }
  else
    os.format( "\">%s&#160;:&#160;%.0f hps (%.0f aps)",
               n.c_str(),
               p -> collected_data.hps.mean() + p -> collected_data.aps.mean(),
               p -> collected_data.aps.mean() );

  // if player tank, print extra metrics
  if ( p -> primary_role() == ROLE_TANK && ! p -> is_enemy() )
  {
    // print DTPS & HPS
    os.format( ", %.0f dtps", p -> collected_data.dtps.mean() );
    os.format( ", %.0f hps (%.0f aps)",  
               p -> collected_data.hps.mean() + p -> collected_data.aps.mean(), 
               p -> collected_data.aps.mean() );
    // print TMI
    double tmi_display = p -> collected_data.theck_meloree_index.mean();
    if ( tmi_display >= 1.0e7 )
      os.format( ", %.2fM TMI", tmi_display / 1.0e6 );
    else if ( std::abs( tmi_display ) <= 999.9 )
      os.format( ", %.3fk TMI", tmi_display / 1.0e3 );
    else
      os.format( ", %.1fk TMI", tmi_display / 1.0e3 );
    // if we're using a non-standard window, append that to the label appropriately (i.e. TMI-4.0 for a 4.0-second window)
    if ( p -> tmi_window != 6.0 )
      os.format( "-%1.1f", p -> tmi_window );

    if ( sim -> show_etmi || sim -> player_no_pet_list.size() > 1 )
    {
      double etmi_display = p -> collected_data.effective_theck_meloree_index.mean();
      if ( etmi_display >= 1.0e7 )
        os.format( ", %.1fk ETMI", etmi_display / 1.0e6 );
      else if ( std::abs( etmi_display ) <= 999.9 )
        os.format( ", %.3fk ETMI", etmi_display / 1.0e3 );
      else
        os.format( ", %.1fk ETMI", etmi_display / 1.0e3 );
      // if we're using a non-standard window, append that to the label appropriately (i.e. TMI-4.0 for a 4.0-second window)
      if ( p -> tmi_window != 6.0 )
        os.format( "-%1.1f", p -> tmi_window );
    }

    os << "\n";
  }
  os << "</h2>\n";

  os << "<div class=\"toggle-content";
  if ( num_players > 1 )
  {
    os << " hide";
  }
  os << "\">\n";

  os << "<ul class=\"params\">\n";
  if ( p -> dbc.ptr )
  {
#ifdef SC_USE_PTR
    os << "<li><b>PTR activated</b></li>\n";
#else
    os << "<li><b>BETA activated</b></li>\n";
#endif
  }
  const char* pt;
  if ( p -> is_pet() )
    pt = util::pet_type_string( p -> cast_pet() -> pet_type );
  else
    pt = util::player_type_string( p -> type );
  os.format(
    "<li><b>Race:</b> %s</li>\n"
    "<li><b>Class:</b> %s</li>\n",
    util::inverse_tokenize( p -> race_str ).c_str(),
    util::inverse_tokenize( pt ).c_str()
  );

  if ( p -> specialization() != SPEC_NONE )
    os.format(
      "<li><b>Spec:</b> %s</li>\n",
      util::inverse_tokenize( dbc::specialization_string( p -> specialization() ) ).c_str() );

  os.format(
    "<li><b>Level:</b> %d</li>\n"
    "<li><b>Role:</b> %s</li>\n"
    "<li><b>Position:</b> %s</li>\n"
    "</ul>\n"
    "<div class=\"clear\"></div>\n",
    p -> level, util::inverse_tokenize( util::role_type_string( p -> primary_role() ) ).c_str(), p -> position_str.c_str() );
}

// print_html_player_results_spec_gear ======================================

void print_html_player_results_spec_gear( report::sc_html_stream& os, sim_t* sim, player_t* p )
{
  const player_collected_data_t& cd = p -> collected_data;

  // Main player table
  os << "<div class=\"player-section results-spec-gear mt\">\n";
  if ( p -> is_pet() )
  {
    os << "<h3 class=\"toggle open\">Results</h3>\n";
  }
  else
  {
    os << "<h3 class=\"toggle open\">Results, Spec and Gear</h3>\n";
  }
  os << "<div class=\"toggle-content\">\n";

  // Table for DPS, HPS, APS
  os << "<table class=\"sc\">\n"
     << "<tr>\n";
  // First, make the header row
  // Damage
  if ( cd.dps.mean() > 0 )
  {
    os << "<th><a href=\"#help-dps\" class=\"help\">DPS</a></th>\n"
       << "<th><a href=\"#help-dpse\" class=\"help\">DPS(e)</a></th>\n"
       << "<th><a href=\"#help-error\" class=\"help\">DPS Error</a></th>\n"
       << "<th><a href=\"#help-range\" class=\"help\">DPS Range</a></th>\n"
       << "<th><a href=\"#help-dpr\" class=\"help\">DPR</a></th>\n";
  }
  // spacer
  if ( cd.hps.mean() > 0 && cd.dps.mean() > 0 )
    os << "<th>&#160;</th>\n";
  // Heal
  if ( cd.hps.mean() > 0 )
  {
    os << "<th><a href=\"#help-hps\" class=\"help\">HPS</a></th>\n"
       << "<th><a href=\"#help-hpse\" class=\"help\">HPS(e)</a></th>\n"
       << "<th><a href=\"#help-error\" class=\"help\">HPS Error</a></th>\n"
       << "<th><a href=\"#help-range\" class=\"help\">HPS Range</a></th>\n"
       << "<th><a href=\"#help-hpr\" class=\"help\">HPR</a></th>\n";
  }
  // spacer
  if ( cd.hps.mean() > 0 && cd.aps.mean() > 0 )
    os << "<th>&#160;</th>\n";
  // Absorb
  if ( cd.aps.mean() > 0 )
  {
    os << "<th><a href=\"#help-aps\" class=\"help\">APS</a></th>\n"
       << "<th><a href=\"#help-error\" class=\"help\">APS Error</a></th>\n"
       << "<th><a href=\"#help-range\" class=\"help\">APS Range</a></th>\n"
       << "<th><a href=\"#help-hpr\" class=\"help\">APR</a></th>\n";
  }
  // end row, begin new row
  os << "</tr>\n"
     << "<tr>\n";

  // Now do the data row
  if ( cd.dps.mean() > 0 )
  {
    double range = ( p -> collected_data.dps.percentile( 0.5 + sim -> confidence / 2 ) - p -> collected_data.dps.percentile( 0.5 - sim -> confidence / 2 ) );
    double dps_error = sim_t::distribution_mean_error( *sim, p -> collected_data.dps );
    os.format(
      "<td>%.1f</td>\n"
      "<td>%.1f</td>\n"
      "<td>%.1f / %.3f%%</td>\n"
      "<td>%.1f / %.1f%%</td>\n"
      "<td>%.1f</td>\n",
      cd.dps.mean(),
      cd.dpse.mean(),
      dps_error,
      cd.dps.mean() ? dps_error * 100 / cd.dps.mean() : 0,
      range,
      cd.dps.mean() ? range / cd.dps.mean() * 100.0 : 0,
      p -> dpr );
  }
  // Spacer
  if ( cd.dps.mean() > 0 && cd.hps.mean() > 0 )
    os << "<td>&#160;&#160;&#160;&#160;&#160;</td>\n";
  // Heal
  if ( cd.hps.mean() > 0 )
  {
    double range = ( cd.hps.percentile( 0.5 + sim -> confidence / 2 ) - cd.hps.percentile( 0.5 - sim -> confidence / 2 ) );
    double hps_error = sim_t::distribution_mean_error( *sim, p -> collected_data.hps );
    os.format(
      "<td>%.1f</td>\n"
      "<td>%.1f</td>\n"
      "<td>%.2f / %.2f%%</td>\n"
      "<td>%.0f / %.1f%%</td>\n"
      "<td>%.1f</td>\n",
      cd.hps.mean(),
      cd.hpse.mean(),
      hps_error,
      cd.hps.mean() ? hps_error * 100 / cd.hps.mean() : 0,
      range,
      cd.hps.mean() ? range / cd.hps.mean() * 100.0 : 0,
      p -> hpr );
    
  }
  // Spacer
  if ( cd.aps.mean() > 0 && cd.hps.mean() > 0 )
    os << "<td>&#160;&#160;&#160;&#160;&#160;</td>\n";
  // Absorb
  if ( cd.aps.mean() > 0 )
  {
    double range = ( cd.aps.percentile( 0.5 + sim -> confidence / 2 ) - cd.aps.percentile( 0.5 - sim -> confidence / 2 ) );
    double aps_error = sim_t::distribution_mean_error( *sim, p -> collected_data.aps );
    os.format(
      "<td>%.1f</td>\n"
      "<td>%.2f / %.2f%%</td>\n"
      "<td>%.0f / %.1f%%</td>\n"
      "<td>%.1f</td>\n",
      cd.aps.mean(),
      aps_error,
      cd.aps.mean() ? aps_error * 100 / cd.aps.mean() : 0,
      range,
      cd.aps.mean() ? range / cd.aps.mean() * 100.0 : 0,
      p -> hpr );
    
  }
  // close table
  os << "</tr>\n"
     << "</table>\n";
    
  // Tank table
  if ( p -> primary_role() == ROLE_TANK && ! p -> is_enemy() )
  {
    os << "<table class=\"sc mt\">\n"
       // experimental first row for stacking the tables - wasn't happy with how it looked, may return to it later
       //<< "<tr>\n" // first row
       //<< "<th colspan=\"3\"><a href=\"#help-dtps\" class=\"help\">DTPS</a></th>\n"
       //<< "<td>&nbsp&nbsp&nbsp&nbsp&nbsp</td>\n"
       //<< "<th colspan=\"5\"><a href=\"#help-tmi\" class=\"help\">TMI</a></th>\n"
       //<< "<td>&nbsp&nbsp&nbsp&nbsp&nbsp</td>\n"
       //<< "<th colspan=\"4\"><a href=\"#help-msd\" class=\"help\">MSD</a></th>\n"
       //<< "</tr>\n" // end first row
       << "<tr>\n"  // start second row
       << "<th><a href=\"#help-dtps\" class=\"help\">DTPS</a></th>\n"
       << "<th><a href=\"#help-error\" class=\"help\">DTPS Error</a></th>\n"
       << "<th><a href=\"#help-range\" class=\"help\">DTPS Range</a></th>\n"
       << "<th>&#160;</th>\n"
       << "<th><a href=\"#help-theck-meloree-index\" class=\"help\">TMI</a></th>\n"
       << "<th><a href=\"#help-error\" class=\"help\">TMI Error</a></th>\n"
       << "<th><a href=\"#help-tmi\" class=\"help\">TMI Min</a></th>\n"
       << "<th><a href=\"#help-tmi\" class=\"help\">TMI Max</a></th>\n"
       << "<th><a href=\"#help-tmirange\" class=\"help\">TMI Range</a></th>\n"
       << "<th>&nbsp</th>\n"
       << "<th><a href=\"#help-msd" << p -> actor_index << "\" class=\"help\">MSD Mean</a></th>\n"
       << "<th><a href=\"#help-msd" << p -> actor_index << "\" class=\"help\">MSD Min</a></th>\n"
       << "<th><a href=\"#help-msd" << p -> actor_index << "\" class=\"help\">MSD Max</a></th>\n"
       << "<th><a href=\"#help-max-spike-damage-frequency\" class=\"help\">MSD Freq.</a></th>\n"
       << "<th>&nbsp</th>\n"
       << "<th><a href=\"#help-tmiwin\" class=\"help\">Window</a></th>\n"
       << "<th><a href=\"#help-tmi-bin-size\" class=\"help\">Bin Size</a></th>\n"
       << "</tr>\n" // end second row
       << "<tr>\n"; // start third row

    double dtps_range = ( cd.dtps.percentile( 0.5 + sim -> confidence / 2 ) - cd.dtps.percentile( 0.5 - sim -> confidence / 2 ) );
    double dtps_error = sim_t::distribution_mean_error( *sim, p -> collected_data.dtps );
    os.format(
      "<td>%.1f</td>\n"
      "<td>%.2f / %.2f%%</td>\n"
      "<td>%.0f / %.1f%%</td>\n",
      cd.dtps.mean(),
      dtps_error, cd.dtps.mean() ? dtps_error * 100 / cd.dtps.mean() : 0,
      dtps_range, cd.dtps.mean() ? dtps_range / cd.dtps.mean() * 100.0 : 0 );
    
    // spacer
    os << "<td>&nbsp&nbsp&nbsp&nbsp&nbsp</td>\n";
    
    double tmi_error = sim_t::distribution_mean_error( *sim, p -> collected_data.theck_meloree_index );
    double tmi_range = ( cd.theck_meloree_index.percentile( 0.5 + sim -> confidence / 2 ) - cd.theck_meloree_index.percentile( 0.5 - sim -> confidence / 2 ) );

    // print TMI
    if ( std::abs( cd.theck_meloree_index.mean() ) > 1.0e8 )
      os.format( "<td>%1.3e</td>\n", cd.theck_meloree_index.mean() );
    else
      os.format( "<td>%.1fk</td>\n", cd.theck_meloree_index.mean() / 1e3 );

    // print TMI error/variance
    if ( tmi_error > 1.0e6 )
    {
      os.format( "<td>%1.2e / %.2f%%</td>\n",
                 tmi_error, cd.theck_meloree_index.mean() ? tmi_error * 100.0 / cd.theck_meloree_index.mean() : 0.0 );
    }
    else
    {
      os.format( "<td>%.0f / %.2f%%</td>\n",
                 tmi_error, cd.theck_meloree_index.mean() ? tmi_error * 100.0 / cd.theck_meloree_index.mean() : 0.0 );
    }

    // print  TMI min/max
    if ( std::abs( cd.theck_meloree_index.min() ) > 1.0e8 )
      os.format( "<td>%1.2e</td>\n", cd.theck_meloree_index.min() );
    else
      os.format( "<td>%.1fk</td>\n", cd.theck_meloree_index.min() / 1e3 );
    
    if ( std::abs( cd.theck_meloree_index.max() ) > 1.0e8 )
      os.format( "<td>%1.2e</td>\n", cd.theck_meloree_index.max() );
    else
      os.format( "<td>%.1fk</td>\n", cd.theck_meloree_index.max() / 1e3 );

    // print TMI range
    if ( tmi_range > 1.0e8 )
    {
      os.format( "<td>%1.2e / %.1f%%</td>\n",
                  tmi_range, cd.theck_meloree_index.mean() ? tmi_range * 100.0 / cd.theck_meloree_index.mean() : 0.0 );
    }
    else
    {
      os.format( "<td>%.1fk / %.1f%%</td>\n",
                  tmi_range / 1e3, cd.theck_meloree_index.mean() ? tmi_range * 100.0 / cd.theck_meloree_index.mean() : 0.0 );
    }
        
    // spacer
    os << "<td>&nbsp&nbsp&nbsp&nbsp&nbsp</td>\n";

    // print Max Spike Size stats
    os.format( "<td>%.1f%%</td>\n", cd.max_spike_amount.mean() );
    os.format( "<td>%.1f%%</td>\n", cd.max_spike_amount.min() );
    os.format( "<td>%.1f%%</td>\n", cd.max_spike_amount.max() );
    
    // print rough estimate of spike frequency
    os.format( "<td>%.1f</td>\n", cd.theck_meloree_index.mean() ? std::exp( cd.theck_meloree_index.mean() / 1e3 / cd.max_spike_amount.mean() ) : 0.0 );
    
    // spacer
    os << "<td>&nbsp&nbsp&nbsp&nbsp&nbsp</td>\n";

    // print TMI window and bin size
    os.format( "<td>%.2fs</td>\n", p -> tmi_window);
    os.format( "<td>%.2fs</td>\n", sim -> tmi_bin_size );

    // End defensive table
    os << "</tr>\n"
       << "</table>\n";
  }

  // Resources/Activity Table
  if ( ! p -> is_pet() )
  {
    os << "<table class=\"sc ra\">\n"
       << "<tr>\n"
       << "<th><a href=\"#help-rps-out\" class=\"help\">RPS Out</a></th>\n"
       << "<th><a href=\"#help-rps-in\" class=\"help\">RPS In</a></th>\n"
       << "<th>Primary Resource</th>\n"
       << "<th><a href=\"#help-waiting\" class=\"help\">Waiting</a></th>\n"
       << "<th><a href=\"#help-apm\" class=\"help\">APM</a></th>\n"
       << "<th>Active</th>\n"
       << "<th>Skill</th>\n"
       << "</tr>\n"
       << "<tr>\n";

    os.format(
      "<td>%.1f</td>\n"
      "<td>%.1f</td>\n"
      "<td>%s</td>\n"
      "<td>%.2f%%</td>\n"
      "<td>%.1f</td>\n"
      "<td>%.1f%%</td>\n"
      "<td>%.0f%%</td>\n"
      "</tr>\n"
      "</table>\n",
      p -> rps_loss,
      p -> rps_gain,
      util::inverse_tokenize( util::resource_type_string( p -> primary_resource() ) ).c_str(),
      cd.fight_length.mean() ? 100.0 * cd.waiting_time.mean() / cd.fight_length.mean() : 0,
      cd.fight_length.mean() ? 60.0 * cd.executed_foreground_actions.mean() / cd.fight_length.mean() : 0,
      sim -> simulation_length.mean() ? cd.fight_length.mean() / sim -> simulation_length.mean() * 100.0 : 0,
      p -> initial.skill * 100.0 );
  }


  // Spec and gear
  if ( ! p -> is_pet() )
  {
    os << "<table class=\"sc mt\">\n";
    if ( ! p -> origin_str.empty() )
    {
      std::string origin_url = util::encode_html( p -> origin_str );
      os.format(
        "<tr class=\"left\">\n"
        "<th><a href=\"#help-origin\" class=\"help\">Origin</a></th>\n"
        "<td><a href=\"%s\" class=\"ext\">%s</a></td>\n"
        "</tr>\n",
        p -> origin_str.c_str(),
        origin_url.c_str() );
    }

    if ( ! p -> talents_str.empty() )
    {
      os.format(
        "<tr class=\"left\">\n"
        "<th>Talents</th>\n"
        "<td><ul class=\"float\">\n" );
      for ( uint32_t row = 0; row < MAX_TALENT_ROWS; row++ )
      {
        for ( uint32_t col = 0; col < MAX_TALENT_COLS; col++ )
        {
          talent_data_t* t = talent_data_t::find( p -> type, row, col, p -> specialization(), p -> dbc.ptr );
          std::string name = "none";
          if ( t && t -> name_cstr() )
          {
            name = t -> name_cstr();
            if ( t -> specialization() != SPEC_NONE )
            {
              name += " (";
              name += util::specialization_string( t -> specialization() );
              name += ")";
            }
          }
          if ( p -> talent_points.has_row_col( row, col ) )
            os.format( "<li><strong>%d</strong>:&#160;%s</li>\n", row == 6 ? 100 : ( row + 1 ) * 15, name.c_str() );
        }
      }
      std::string url_string = p -> talents_str;
      if ( ! util::str_in_str_ci( url_string, "http" ) )
        url_string = util::create_blizzard_talent_url( p );

      std::string enc_url = util::encode_html( url_string );
      os.format(
        "<li><a href=\"%s\" class=\"ext\">Talent Calculator</a></li>\n",
        enc_url.c_str() );

      os << "</ul></td>\n";
      os << "</tr>\n";
    }

    // Glyphs
    if ( !p -> glyph_list.empty() )
    {
      os.format(
        "<tr class=\"left\">\n"
        "<th>Glyphs</th>\n"
        "<td>\n"
        "<ul class=\"float\">\n" );
      for ( size_t i = 0; i < p -> glyph_list.size(); ++i )
      {
        os << "<li>" << p -> glyph_list[ i ] -> name_cstr() << "</li>\n";
      }
      os << "</ul>\n"
         << "</td>\n"
         << "</tr>\n";
    }

    // Professions
    if ( ! p -> professions_str.empty() )
    {
      os.format(
        "<tr class=\"left\">\n"
        "<th>Professions</th>\n"
        "<td>\n"
        "<ul class=\"float\">\n" );
      for ( profession_e i = PROFESSION_NONE; i < PROFESSION_MAX; ++i )
      {
        if ( p -> profession[ i ] <= 0 )
          continue;

        os << "<li>" << util::profession_type_string( i ) << ": " << p -> profession[ i ] << "</li>\n";
      }
      os << "</ul>\n"
         << "</td>\n"
         << "</tr>\n";
    }

    os << "</table>\n";
  }
}

// print_html_player_abilities ==============================================

bool is_output_stat( unsigned mask, bool child, const stats_t* s )
{
  if ( s -> quiet )
    return false;

  if ( ! ( ( 1 << s -> type ) & mask ) )
    return false;

  if ( s -> num_executes.mean() <= 0.001 && s -> compound_amount == 0 && ! s -> player -> sim -> debug )
    return false;

  // If we are checking output on a non-child stats object, only return false
  // when parent is found, if the actor of the parent stats object, and this
  // object are the same.  The parent and child object types also must be
  // equal.
  if ( ! child && s -> parent && s -> parent -> player == s -> player && s -> parent -> type == s -> type )
    return false;


  return true;
}

void output_player_action( report::sc_html_stream& os,
                                  unsigned& row,
                                  unsigned cols,
                                  unsigned mask,
                                  stats_t* s,
                                  player_t* actor )
{
  if ( ! is_output_stat( mask, false, s ) )
    return;

  print_html_action_info( os, mask, s, row++, cols );

  for ( size_t child_idx = 0, end = s -> children.size(); child_idx < end; child_idx++ )
  {
    stats_t* child_stats = s -> children[ child_idx ];
    if ( ! is_output_stat( mask, true, child_stats ) )
      continue;

    print_html_action_info( os, mask, child_stats, row++, cols, actor );
  }
}

void output_player_damage_summary( report::sc_html_stream& os, player_t* actor )
{
  if ( actor -> collected_data.dmg.max() == 0 && ! actor -> sim -> debug )
    return;

  // Number of static columns in table
  const int static_columns = 5;
  // Number of dynamically changing columns
  int n_optional_columns = 6;

  // Abilities Section - Damage
  os << "<table class=\"sc\">\n"
      << "<tr>\n"
      << "<th class=\"left small\">Damage Stats</th>\n"
      << "<th class=\"small\"><a href=\"#help-dps\" class=\"help\">DPS</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-dps-pct\" class=\"help\">DPS%</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-Count\" class=\"help\">Execute</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-interval\" class=\"help\">Interval</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-dpe\" class=\"help\">DPE</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-dpet\" class=\"help\">DPET</a></th>\n"
      // Optional columns begin here
      << "<th class=\"small\"><a href=\"#help-type\" class=\"help\">Type</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-count\" class=\"help\">Count</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-hit\" class=\"help\">Hit</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-crit\" class=\"help\">Crit</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-avg\" class=\"help\">Avg</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-crit-pct\" class=\"help\">Crit%</a></th>\n";

  if ( player_has_avoidance( actor, MASK_DMG ) )
  {
    os << "<th class=\"small\"><a href=\"#help-miss-pct\" class=\"help\">Avoid%</a></th>\n";
    n_optional_columns++;
  }

  if ( player_has_glance( actor, MASK_DMG ) )
  {
    os << "<th class=\"small\"><a href=\"#help-glance-pct\" class=\"help\">G%</a></th>\n";
    n_optional_columns++;
  }

  if ( player_has_block( actor, MASK_DMG ) )
  {
    os << "<th class=\"small\"><a href=\"#help-block-pct\" class=\"help\">B%</a></th>\n";
    n_optional_columns++;
  }

  if ( player_has_multistrike( actor, MASK_DMG ) )
  {
    os << "<th class=\"small\"><a href=\"#help-count\" class=\"help\">M-Count</a></th>\n"
       << "<th class=\"small\"><a href=\"#help-hit\" class=\"help\">M-Hit</a></th>\n"
       << "<th class=\"small\"><a href=\"#help-crit\" class=\"help\">M-Crit</a></th>\n"
       << "<th class=\"small\"><a href=\"#help-crit-pct\" class=\"help\">M-Crit%</a></th>\n";
    n_optional_columns += 4;
  }

  if ( player_has_tick_results( actor, MASK_DMG ) )
  {
    os << "<th class=\"small\"><a href=\"#help-ticks-uptime-pct\" class=\"help\">Up%</a></th>\n"
       << "</tr>\n";
    n_optional_columns++;
  }

  os << "<tr>\n"
      << "<th class=\"left small\">" << util::encode_html( actor -> name() ) << "</th>\n"
      << "<th class=\"right small\">" << util::to_string( actor -> collected_data.dps.mean(), 0 ) << "</th>\n"
      << "<td colspan=\"" << ( static_columns + n_optional_columns ) << "\" class=\"filler\"></td>\n"
      << "</tr>\n";

  unsigned n_row = 0;
  for ( size_t i = 0; i < actor -> stats_list.size(); ++i )
  {
    if ( actor -> stats_list[ i ] -> compound_amount > 0 )
      output_player_action( os, n_row, n_optional_columns, MASK_DMG, actor -> stats_list[ i ], actor );
  }

  // Print pet statistics
  for ( size_t i = 0; i < actor -> pet_list.size(); ++i )
  {
    pet_t* pet = actor -> pet_list[ i ];

    bool first = true;
    for ( size_t m = 0; m < pet -> stats_list.size(); ++m )
    {
      stats_t* s = pet -> stats_list[ m ];

      if ( ! is_output_stat( MASK_DMG, false, s ) )
        continue;

      if ( s -> parent && s -> parent -> player == pet )
        continue;

      if ( s -> compound_amount == 0 )
        continue;

      if ( first )
      {
        first = false;
        os.format(
          "<tr>\n"
          "<th class=\"left small\">pet - %s</th>\n"
          "<th class=\"right small\">%.0f / %.0f</th>\n"
          "<td colspan=\"%d\" class=\"filler\"></td>\n"
          "</tr>\n",
          pet -> name_str.c_str(),
          pet -> collected_data.dps.mean(),
          pet -> collected_data.dpse.mean(), static_columns + n_optional_columns );
      }

      output_player_action( os, n_row, n_optional_columns, MASK_DMG, s, pet );
    }
  }

  os << "</table>\n";
}

void output_player_heal_summary( report::sc_html_stream& os, player_t* actor )
{
  if ( actor -> collected_data.heal.max() == 0 && ! actor -> sim -> debug )
    return;

  // Number of static columns in table
  const int static_columns = 5;
  // Number of dynamically changing columns
  int n_optional_columns = 6;

  // Abilities Section - Heal
  os << "<table class=\"sc\">\n"
      << "<tr>\n"
      << "<th class=\"left small\">Healing and Absorb Stats</th>\n"
      << "<th class=\"small\"><a href=\"#help-dps\" class=\"help\">HPS</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-dps-pct\" class=\"help\">HPS%</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-count\" class=\"help\">Execute</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-interval\" class=\"help\">Interval</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-dpe\" class=\"help\">HPE</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-dpet\" class=\"help\">HPET</a></th>\n"
      // Optional columns begin here
      << "<th class=\"small\"><a href=\"#help-type\" class=\"help\">Type</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-count\" class=\"help\">Count</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-hit\" class=\"help\">Hit</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-crit\" class=\"help\">Crit</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-avg\" class=\"help\">Avg</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-crit-pct\" class=\"help\">Crit%</a></th>\n";

  if ( player_has_multistrike( actor, MASK_HEAL | MASK_ABSORB ) )
  {
    os << "<th class=\"small\"><a href=\"#help-count\" class=\"help\">M-Count</a></th>\n"
       << "<th class=\"small\"><a href=\"#help-hit\" class=\"help\">M-Hit</a></th>\n"
       << "<th class=\"small\"><a href=\"#help-crit\" class=\"help\">M-Crit</a></th>\n"
       << "<th class=\"small\"><a href=\"#help-crit-pct\" class=\"help\">M-Crit%</a></th>\n";
    n_optional_columns += 4;
  }

  if ( player_has_tick_results( actor, MASK_HEAL | MASK_ABSORB ) )
  {
    os << "<th class=\"small\"><a href=\"#help-ticks-uptime-pct\" class=\"help\">Up%</a></th>\n"
       << "</tr>\n";
    n_optional_columns++;
  }

  os << "<tr>\n"
      << "<th class=\"left small\">" << util::encode_html( actor -> name() ) << "</th>\n"
      << "<th class=\"right small\">" << util::to_string( actor -> collected_data.hps.mean(), 0 ) << "</th>\n"
      << "<td colspan=\"" << ( static_columns + n_optional_columns ) << "\" class=\"filler\"></td>\n"
      << "</tr>\n";

  unsigned n_row = 0;
  for ( size_t i = 0; i < actor -> stats_list.size(); ++i )
  {
    if ( actor -> stats_list[ i ] -> compound_amount > 0 )
      output_player_action( os, n_row, n_optional_columns, MASK_HEAL | MASK_ABSORB, actor -> stats_list[ i ], actor );
  }

  // Print pet statistics
  for ( size_t i = 0; i < actor -> pet_list.size(); ++i )
  {
    pet_t* pet = actor -> pet_list[ i ];

    bool first = true;
    for ( size_t m = 0; m < pet -> stats_list.size(); ++m )
    {
      stats_t* s = pet -> stats_list[ m ];

      if ( ! is_output_stat( MASK_HEAL | MASK_ABSORB, false, s ) )
        continue;

      if ( s -> parent && s -> parent -> player == pet )
        continue;
      
      if ( s -> compound_amount == 0 )
        continue;

      if ( first )
      {
        first = false;
        os.format(
          "<tr>\n"
          "<th class=\"left small\">pet - %s</th>\n"
          "<th class=\"right small\">%.0f / %.0f</th>\n"
          "<td colspan=\"%d\" class=\"filler\"></td>\n"
          "</tr>\n",
          pet -> name_str.c_str(),
          pet -> collected_data.dps.mean(),
          pet -> collected_data.dpse.mean(), static_columns + n_optional_columns );
      }

      output_player_action( os, n_row, n_optional_columns, MASK_HEAL | MASK_ABSORB, s, pet );
    }
  }

  os << "</table>\n";
}

void output_player_simple_ability_summary( report::sc_html_stream& os, player_t* actor )
{
  if ( actor -> collected_data.dmg.max() == 0 && ! actor -> sim -> debug )
    return;

  // Number of static columns in table
  const int static_columns = 2;
  // Number of dynamically changing columns
  int n_optional_columns = 0;

  // Abilities Section - Simple Actions
  os << "<table class=\"sc\">\n"
      << "<tr>\n"
      << "<th class=\"left small\">Simple Action Stats</th>\n"
      << "<th class=\"small\"><a href=\"#help-count\" class=\"help\">Execute</a></th>\n"
      << "<th class=\"small\"><a href=\"#help-interval\" class=\"help\">Interval</a></th>\n";

  os << "<tr>\n"
      << "<th class=\"left small\">" << util::encode_html( actor -> name() ) << "</th>\n"
      << "<td colspan=\"" << ( static_columns + n_optional_columns ) << "\" class=\"filler\"></td>\n"
      << "</tr>\n";

  unsigned n_row = 0;
  for ( size_t i = 0; i < actor -> stats_list.size(); ++i )
  {
    if ( actor -> stats_list[ i ] -> compound_amount == 0 )
      output_player_action( os, n_row, n_optional_columns, MASK_DMG | MASK_HEAL | MASK_ABSORB, actor -> stats_list[ i ], actor );
  }

  // Print pet statistics
  for ( size_t i = 0; i < actor -> pet_list.size(); ++i )
  {
    pet_t* pet = actor -> pet_list[ i ];

    bool first = true;
    for ( size_t m = 0; m < pet -> stats_list.size(); ++m )
    {
      stats_t* s = pet -> stats_list[ m ];

      if ( ! is_output_stat( MASK_DMG | MASK_HEAL | MASK_ABSORB, false, s ) )
        continue;

      if ( s -> parent && s -> parent -> player == pet )
        continue;

      if ( s -> compound_amount > 0 )
        continue;

      if ( first )
      {
        first = false;
        os.format(
          "<tr>\n"
          "<th class=\"left small\">pet - %s</th>\n"
          "<td colspan=\"%d\" class=\"filler\"></td>\n"
          "</tr>\n",
          pet -> name_str.c_str(), 1 + static_columns + n_optional_columns );
      }

      output_player_action( os, n_row, n_optional_columns, MASK_DMG | MASK_HEAL | MASK_ABSORB, s, pet );
    }
  }

  os << "</table>\n";
}

void print_html_player_abilities( report::sc_html_stream& os, player_t* p )
{
  // open section
  os << "<div class=\"player-section\">\n"
     << "<h3 class=\"toggle open\">Abilities</h3>\n"
     << "<div class=\"toggle-content\">\n";

  output_player_damage_summary( os, p );
  output_player_heal_summary( os, p );
  output_player_simple_ability_summary( os, p );

  // close section
  os << "</div>\n"
     << "</div>\n";
}

// print_html_player_benefits_uptimes =======================================

void print_html_player_benefits_uptimes( report::sc_html_stream& os, player_t* p )
{
  os << "<div class=\"player-section benefits\">\n"
     << "<h3 class=\"toggle\">Benefits & Uptimes</h3>\n"
     << "<div class=\"toggle-content hide\">\n"
     << "<table class=\"sc\">\n"
     << "<tr>\n"
     << "<th>Benefits</th>\n"
     << "<th>%</th>\n"
     << "</tr>\n";
  int i = 1;

  for ( size_t j = 0; j < p -> benefit_list.size(); ++j )
  {
    benefit_t* u = p -> benefit_list[ j ];
    if ( u -> ratio.mean() > 0 )
    {
      os << "<tr";
      if ( !( i & 1 ) )
      {
        os << " class=\"odd\"";
      }
      os << ">\n";
      os.format(
        "<td class=\"left\">%s</td>\n"
        "<td class=\"right\">%.1f%%</td>\n"
        "</tr>\n",
        u -> name(),
        u -> ratio.mean() );
      i++;
    }
  }

  for ( size_t m = 0; m < p -> pet_list.size(); ++m )
  {
    pet_t* pet = p -> pet_list[ m ];
    for ( size_t j = 0; j < p -> benefit_list.size(); ++j )
    {
      benefit_t* u = p -> benefit_list[ j ];
      if ( u -> ratio.mean() > 0 )
      {
        std::string benefit_name;
        benefit_name += pet -> name_str + '-';
        benefit_name += u -> name();

        os << "<tr";
        if ( !( m & 1 ) )
        {
          os << " class=\"odd\"";
        }
        os << ">\n";
        os.format(
          "<td class=\"left\">%s</td>\n"
          "<td class=\"right\">%.1f%%</td>\n"
          "</tr>\n",
          benefit_name.c_str(),
          u -> ratio.mean() );
        i++;
      }
    }
  }

  os << "<tr>\n"
     << "<th>Uptimes</th>\n"
     << "<th>%</th>\n"
     << "</tr>\n";

  for ( size_t j = 0; j < p -> uptime_list.size(); ++j )
  {
    uptime_t* u = p -> uptime_list[ j ];
    if ( u -> uptime_sum.mean() > 0 )
    {
      os << "<tr";
      if ( !( i & 1 ) )
      {
        os << " class=\"odd\"";
      }
      os << ">\n";
      os.format(
        "<td class=\"left\">%s</td>\n"
        "<td class=\"right\">%.1f%%</td>\n"
        "</tr>\n",
        u -> name(),
        u -> uptime_sum.mean() * 100.0 );
      i++;
    }
  }

  for ( size_t m = 0; m < p -> pet_list.size(); ++m )
  {
    pet_t* pet = p -> pet_list[ m ];
    for ( size_t j = 0; j < p -> uptime_list.size(); ++j )
    {
      uptime_t* u = p -> uptime_list[ j ];
      if ( u -> uptime_sum.mean() > 0 )
      {
        std::string uptime_name;
        uptime_name += pet -> name_str + '-';
        uptime_name += u -> name();

        os << "<tr";
        if ( !( m & 1 ) )
        {
          os << " class=\"odd\"";
        }
        os << ">\n";
        os.format(
          "<td class=\"left\">%s</td>\n"
          "<td class=\"right\">%.1f%%</td>\n"
          "</tr>\n",
          uptime_name.c_str(),
          u -> uptime_sum.mean() * 100.0 );

        i++;
      }
    }
  }


  os << "</table>\n"
     << "</div>\n"
     << "</div>\n";
}

// print_html_player_procs ==================================================

void print_html_player_procs( report::sc_html_stream& os, std::vector<proc_t*> pr )
{
  // Procs Section
  os << "<div class=\"player-section procs\">\n"
     << "<h3 class=\"toggle\">Procs</h3>\n"
     << "<div class=\"toggle-content hide\">\n"
     << "<table class=\"sc\">\n"
     << "<tr>\n"
     << "<th></th>\n"
     << "<th>Count</th>\n"
     << "<th>Interval</th>\n"
     << "</tr>\n";
  {
    int i = 1;
    for ( size_t j = 0; j < pr.size(); ++j )
    {
      proc_t* proc = pr[ j ];
      if ( proc -> count.mean() > 0 )
      {
        os << "<tr";
        if ( !( i & 1 ) )
        {
          os << " class=\"odd\"";
        }
        os << ">\n";
        os.format(
          "<td class=\"left\">%s</td>\n"
          "<td class=\"right\">%.1f</td>\n"
          "<td class=\"right\">%.1fsec</td>\n"
          "</tr>\n",
          proc -> name(),
          proc -> count.mean(),
          proc -> interval_sum.mean() );
        i++;
      }
    }
  }
  os << "</table>\n"
     << "</div>\n"
     << "</div>\n";



}

// print_html_player_deaths =================================================

void print_html_player_deaths( report::sc_html_stream& os, player_t* p, player_processed_report_information_t& ri )
{
  // Death Analysis

  const extended_sample_data_t& deaths = p -> collected_data.deaths;

  if ( deaths.size() > 0 )
  {
    std::string distribution_deaths_str                = "";
    if ( ! ri.distribution_deaths_chart.empty() )
    {
      distribution_deaths_str = "<img src=\"" + ri.distribution_deaths_chart + "\" alt=\"Deaths Distribution Chart\" />\n";
    }

    os << "<div class=\"player-section gains\">\n"
       << "<h3 class=\"toggle\">Deaths</h3>\n"
       << "<div class=\"toggle-content hide\">\n"
       << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th></th>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">death count</td>\n"
       << "<td class=\"right\">" << deaths.size() << "</td>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">death count pct</td>\n"
       << "<td class=\"right\">"
       << ( double ) deaths.size() / p -> sim -> iterations * 100
       << "</td>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">avg death time</td>\n"
       << "<td class=\"right\">" << deaths.mean() << "</td>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">min death time</td>\n"
       << "<td class=\"right\">" << deaths.min() << "</td>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">max death time</td>\n"
       << "<td class=\"right\">" << deaths.max() << "</td>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">dmg taken</td>\n"
       << "<td class=\"right\">" << p -> collected_data.dmg_taken.mean() << "</td>\n"
       << "</tr>\n";

    os << "</table>\n";

    os << "<div class=\"clear\"></div>\n";

    os << "" << distribution_deaths_str << "\n";

    os << "</div>\n"
       << "</div>\n";
  }
}

// print_html_player_ =======================================================

void print_html_player_( report::sc_html_stream& os, sim_t* sim, player_t* p, int player_index )
{
  report::generate_player_charts( p, p -> report_information );
  report::generate_player_buff_lists( p, p -> report_information );

  print_html_player_description( os, sim, p, player_index );

  print_html_player_results_spec_gear( os, sim, p );

  print_html_player_scale_factors( os, sim, p, p -> report_information );

  print_html_player_charts( os, sim, p, p -> report_information );

  print_html_player_abilities( os, p );

  print_html_player_buffs( os, p, p -> report_information );

  print_html_player_custom_section( os, p, p -> report_information );

  print_html_player_resources( os, p, p -> report_information );

  print_html_player_benefits_uptimes( os, p );

  print_html_player_procs( os, p -> proc_list );

  print_html_player_deaths( os, p, p -> report_information );

  print_html_player_statistics( os, p, p -> report_information );

  print_html_player_action_priority_list( os, sim, p );

  print_html_stats( os, p );

  print_html_gear( os, p );

  print_html_talents( os, p );

  print_html_profile( os, p );

  // print_html_player_gear_weights( os, p, p -> report_information );

  os << "</div>\n"
     << "</div>\n\n";
}

} // UNNAMED NAMESPACE ====================================================

namespace report {

void print_html_player( report::sc_html_stream& os, player_t* p, int player_index )
{
  print_html_player_( os, p -> sim, p, player_index );
}

} // END report NAMESPACE
