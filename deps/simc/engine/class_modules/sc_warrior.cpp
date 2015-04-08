// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace
{ // UNNAMED NAMESPACE
// ==========================================================================
// Warrior
// ==========================================================================

struct warrior_t;

enum warrior_stance { STANCE_BATTLE = 2, STANCE_DEFENSE = 4, STANCE_GLADIATOR = 8 };

struct warrior_td_t: public actor_pair_t
{
  dot_t* dots_bloodbath;
  dot_t* dots_deep_wounds;
  dot_t* dots_ravager;
  dot_t* dots_rend;

  buff_t* debuffs_colossus_smash;
  buff_t* debuffs_charge;
  buff_t* debuffs_demoralizing_shout;
  buff_t* debuffs_taunt;

  warrior_t& warrior;
  warrior_td_t( player_t* target, warrior_t& p );
};

struct warrior_t: public player_t
{
public:
  event_t* heroic_charge;
  int initial_rage;
  bool warrior_fixed_time;
  player_t* last_target_charged;
  bool stance_dance; // This is just a small optimization, if stance swapping is never required (99% of simulations),
                     // then there's no reason to waste cpu cycles checking stance states inside of base_t::execute()
                     // base_t::ready() continues to check if the ability is usable in the current stance, and if it is not usable in that stance
                     // stance_swap is called. base_t::execute() adds an additional check, which will try and swap the player back to their original stance if possible
  bool player_override_stance_dance;
                     // When the player sets this to true, it will prevent stance_dance from being set to true.
                     // This will allow full control over stance swapping.
  bool gladiator; //Check a bunch of crap to see if this guy wants to be gladiator dps or not.

  // Active
  struct active_t
  {
    action_t* blood_craze;
    action_t* bloodbath_dot;
    action_t* deep_wounds;
    action_t* enhanced_rend;
    action_t* rallying_cry_heal;
    action_t* second_wind;
    attack_t* sweeping_strikes;
    attack_t* aoe_sweeping_strikes;
    heal_t* t16_2pc;
    warrior_stance stance;
  } active;

  // Buffs
  struct buffs_t
  {
    // All Warriors
    buff_t* battle_stance;
    buff_t* berserker_rage;
    buff_t* bladestorm;
    buff_t* charge_movement;
    buff_t* defensive_stance;
    buff_t* heroic_leap_movement;
    buff_t* intervene_movement;
    buff_t* shield_charge_movement;
    // Talents
    buff_t* avatar;
    buff_t* bloodbath;
    buff_t* enraged_regeneration;
    buff_t* gladiator_stance;
    buff_t* ravager;
    buff_t* ravager_protection;
    buff_t* debuffs_slam;
    buff_t* sudden_death;
    // Glyphs
    buff_t* enraged_speed;
    buff_t* hamstring;
    buff_t* heroic_leap_glyph;
    buff_t* raging_blow_glyph;
    buff_t* raging_wind;
    buff_t* rude_interruption;
    // Arms and Fury
    buff_t* die_by_the_sword;
    buff_t* rallying_cry;
    buff_t* recklessness;
    // Fury and Prot
    buff_t* enrage;
    // Fury Only
    buff_t* bloodsurge;
    buff_t* meat_cleaver;
    buff_t* raging_blow;
    // Arms only
    buff_t* colossus_smash;
    buff_t* sweeping_strikes;
    // Prot only
    absorb_buff_t* shield_barrier;
    buff_t* blood_craze;
    buff_t* last_stand;
    buff_t* shield_block;
    buff_t* shield_charge;
    buff_t* shield_wall;
    buff_t* sword_and_board;
    buff_t* ultimatum;
    buff_t* unyielding_strikes;

    // Tier bonuses
    buff_t* pvp_2pc_arms;
    buff_t* pvp_2pc_fury;
    buff_t* pvp_2pc_prot;
    buff_t* tier15_2pc_tank;
    buff_t* tier16_reckless_defense;
    buff_t* tier16_4pc_death_sentence;
    buff_t* tier17_2pc_arms;
    buff_t* tier17_4pc_fury;
    buff_t* tier17_4pc_fury_driver;
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    // All Warriors
    cooldown_t* charge;
    cooldown_t* heroic_leap;
    cooldown_t* rage_from_charge;
    cooldown_t* revenge;
    cooldown_t* stance_cooldown;
    cooldown_t* stance_swap;
    // Talents
    cooldown_t* avatar;
    cooldown_t* bladestorm;
    cooldown_t* bloodbath;
    cooldown_t* dragon_roar;
    cooldown_t* shockwave;
    cooldown_t* storm_bolt;
    // Fury And Arms
    cooldown_t* die_by_the_sword;
    cooldown_t* recklessness;
    // Arms only
    cooldown_t* colossus_smash;
    // Prot Only
    cooldown_t* demoralizing_shout;
    cooldown_t* last_stand;
    cooldown_t* rage_from_crit_block;
    cooldown_t* shield_slam;
    cooldown_t* shield_wall;
  } cooldown;

  // Gains
  struct gains_t
  {
    // All Warriors
    gain_t* avoided_attacks;
    gain_t* charge;
    gain_t* enrage;
    gain_t* melee_main_hand;
    // Fury and Prot
    gain_t* defensive_stance;
    // Fury and Arms
    gain_t* drawn_sword_glyph;
    // Fury Only
    gain_t* bloodthirst;
    gain_t* melee_off_hand;
    // Arms Only
    gain_t* melee_crit;
    gain_t* sweeping_strikes;
    gain_t* taste_for_blood;
    // Prot Only
    gain_t* critical_block;
    gain_t* revenge;
    gain_t* shield_slam;
    gain_t* sword_and_board;
    // Tier bonuses
    gain_t* tier15_4pc_tank;
    gain_t* tier16_2pc_melee;
    gain_t* tier16_4pc_tank;
    gain_t* tier17_4pc_arms;
  } gain;

  // Spells
  struct spells_t
  {
    // All Warriors
    const spell_data_t* charge;
    const spell_data_t* intervene;
    const spell_data_t* headlong_rush;
    const spell_data_t* heroic_leap;
    const spell_data_t* t17_prot_2p;
  } spell;

  // Glyphs
  struct glyphs_t
  {
    // All Warriors
    const spell_data_t* bull_rush;
    const spell_data_t* cleave;
    const spell_data_t* death_from_above;
    const spell_data_t* enraged_speed;
    const spell_data_t* hamstring;
    const spell_data_t* heroic_leap;
    const spell_data_t* long_charge;
    const spell_data_t* rude_interruption;
    const spell_data_t* unending_rage;
    const spell_data_t* victory_rush;
    // Fury and Arms
    const spell_data_t* drawn_sword;
    const spell_data_t* rallying_cry;
    const spell_data_t* recklessness;
    const spell_data_t* wind_and_thunder;
    // Fury only
    const spell_data_t* bloodthirst;
    const spell_data_t* raging_blow;
    const spell_data_t* raging_wind;
    // Arms only
    const spell_data_t* sweeping_strikes;
    // Arms and Prot
    const spell_data_t* resonating_power;
    // Prot only
    const spell_data_t* shield_wall;
  } glyphs;

  // Mastery
  struct mastery_t
  {
    const spell_data_t* critical_block; //Protection
    const spell_data_t* unshackled_fury; //Fury
    const spell_data_t* weapons_master; //Arms
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* bloodsurge_wasted;
    proc_t* delayed_auto_attack;

    //Tier bonuses
    proc_t* t17_2pc_arms;
    proc_t* t17_2pc_fury;
  } proc;

  struct realppm_t
  {
    std::shared_ptr<real_ppm_t> sudden_death;
  } rppm;

  // Spec Passives
  struct spec_t
  {
    //All specs
    const spell_data_t* execute;
    //Arms-only
    const spell_data_t* colossus_smash;
    const spell_data_t* mortal_strike;
    const spell_data_t* rend;
    const spell_data_t* seasoned_soldier;
    const spell_data_t* sweeping_strikes;
    const spell_data_t* weapon_mastery;
    //Arms and Prot
    const spell_data_t* thunder_clap;
    //Arms and Fury
    const spell_data_t* die_by_the_sword;
    const spell_data_t* inspiring_presence;
    const spell_data_t* shield_barrier;
    const spell_data_t* rallying_cry;
    const spell_data_t* recklessness;
    const spell_data_t* whirlwind;
    //Fury-only
    const spell_data_t* bloodsurge;
    const spell_data_t* bloodthirst;
    const spell_data_t* crazed_berserker;
    const spell_data_t* cruelty;
    const spell_data_t* meat_cleaver;
    const spell_data_t* piercing_howl;
    const spell_data_t* raging_blow;
    const spell_data_t* singleminded_fury;
    const spell_data_t* titans_grip;
    const spell_data_t* wild_strike;
    // Fury and Prot
    const spell_data_t* enrage;
    //Prot-only
    const spell_data_t* bastion_of_defense;
    const spell_data_t* bladed_armor;
    const spell_data_t* blood_craze;
    const spell_data_t* deep_wounds;
    const spell_data_t* demoralizing_shout;
    const spell_data_t* devastate;
    const spell_data_t* last_stand;
    const spell_data_t* mocking_banner;
    const spell_data_t* protection; // Weird spec passive that increases damage of bladestorm/execute.
    const spell_data_t* resolve;
    const spell_data_t* revenge;
    const spell_data_t* riposte;
    const spell_data_t* shield_block;
    const spell_data_t* shield_mastery;
    const spell_data_t* shield_slam;
    const spell_data_t* shield_wall;
    const spell_data_t* sword_and_board;
    const spell_data_t* ultimatum;
    const spell_data_t* unwavering_sentinel;
  } spec;

  // Talents
  struct talents_t
  {
    const spell_data_t* juggernaut;
    const spell_data_t* double_time;
    const spell_data_t* warbringer;

    const spell_data_t* enraged_regeneration;
    const spell_data_t* second_wind;
    const spell_data_t* impending_victory;

    const spell_data_t* taste_for_blood;
    const spell_data_t* unyielding_strikes;
    const spell_data_t* sudden_death;
    const spell_data_t* unquenchable_thirst;
    const spell_data_t* heavy_repercussions;
    const spell_data_t* slam;
    const spell_data_t* furious_strikes;

    const spell_data_t* storm_bolt;
    const spell_data_t* shockwave;
    const spell_data_t* dragon_roar;

    const spell_data_t* mass_spell_reflection;
    const spell_data_t* safeguard;
    const spell_data_t* vigilance;

    const spell_data_t* avatar;
    const spell_data_t* bloodbath;
    const spell_data_t* bladestorm;

    const spell_data_t* anger_management;
    const spell_data_t* ravager;
    const spell_data_t* siegebreaker; // Arms/Fury talent
    const spell_data_t* gladiators_resolve; // Protection talent
  } talents;

  // Perks
  struct
  {
    //All Specs
    const spell_data_t* improved_heroic_leap;
    //Arms and Fury
    const spell_data_t* improved_die_by_the_sword;
    const spell_data_t* improved_recklessness;
    //Arms only
    const spell_data_t* enhanced_rend;
    const spell_data_t* enhanced_sweeping_strikes;
    //Fury only
    const spell_data_t* enhanced_whirlwind;
    const spell_data_t* empowered_execute;
    //Protection only
    const spell_data_t* improved_block;
    const spell_data_t* improved_defensive_stance;
    const spell_data_t* improved_heroic_throw;
  } perk;

  warrior_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ):
    player_t( sim, WARRIOR, name, r ),
    heroic_charge( 0 ),
    buff( buffs_t() ),
    cooldown( cooldowns_t() ),
    gain( gains_t() ),
    glyphs( glyphs_t() ),
    mastery( mastery_t() ),
    proc( procs_t() ),
    rppm( realppm_t() ),
    spec( spec_t() ),
    talents( talents_t() )
  {
    // Active
    active.bloodbath_dot      = 0;
    active.blood_craze        = 0;
    active.deep_wounds        = 0;
    active.rallying_cry_heal  = 0;
    active.second_wind        = 0;
    active.sweeping_strikes   = 0;
    active.aoe_sweeping_strikes = 0;
    active.enhanced_rend      = 0;
    active.t16_2pc            = 0;
    active.stance             = STANCE_BATTLE;

    // Cooldowns
    cooldown.avatar                   = get_cooldown( "avatar" );
    cooldown.bladestorm               = get_cooldown( "bladestorm" );
    cooldown.bloodbath                = get_cooldown( "bloodbath" );
    cooldown.charge                   = get_cooldown( "charge" );
    cooldown.colossus_smash           = get_cooldown( "colossus_smash" );
    cooldown.demoralizing_shout       = get_cooldown( "demoralizing_shout" );
    cooldown.die_by_the_sword         = get_cooldown( "die_by_the_sword" );
    cooldown.dragon_roar              = get_cooldown( "dragon_roar" );
    cooldown.heroic_leap              = get_cooldown( "heroic_leap" );
    cooldown.last_stand               = get_cooldown( "last_stand" );
    cooldown.rage_from_charge         = get_cooldown( "rage_from_charge" );
    cooldown.rage_from_charge -> duration = timespan_t::from_seconds( 12.0 );
    cooldown.rage_from_crit_block     = get_cooldown( "rage_from_crit_block" );
    cooldown.rage_from_crit_block    -> duration = timespan_t::from_seconds( 3.0 );
    cooldown.recklessness             = get_cooldown( "recklessness" );
    cooldown.revenge                  = get_cooldown( "revenge" );
    cooldown.shield_slam              = get_cooldown( "shield_slam" );
    cooldown.shield_wall              = get_cooldown( "shield_wall" );
    cooldown.shockwave                = get_cooldown( "shockwave" );
    cooldown.stance_swap              = get_cooldown( "stance_swap" );
    cooldown.stance_swap             -> duration = timespan_t::from_seconds( 1.5 );
    cooldown.storm_bolt               = get_cooldown( "storm_bolt" );

    initial_rage = 0;
    warrior_fixed_time = true;
    last_target_charged = 0;
    stance_dance = false;
    player_override_stance_dance = false;
    gladiator = true; //Gladiator until proven otherwise.
    base.distance = 5.0;

    regen_type = REGEN_DISABLED;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_position();
  virtual void      init_procs();
  virtual void      init_resources( bool );
  virtual void      arise();
  virtual void      combat_begin();
  virtual double    composite_attribute( attribute_e attr ) const;
  virtual double    composite_rating_multiplier( rating_e rating ) const;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual double    composite_armor_multiplier() const;
  virtual double    composite_block() const;
  virtual double    composite_block_reduction() const;
  virtual double    composite_parry_rating() const;
  virtual double    composite_parry() const;
  virtual double    composite_melee_expertise( weapon_t* ) const;
  virtual double    composite_attack_power_multiplier() const;
  virtual double    composite_melee_attack_power() const;
  virtual double    composite_crit_block() const;
  virtual double    composite_crit_avoidance() const;
  virtual double    composite_melee_speed() const;
  virtual double    composite_melee_crit() const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_player_critical_damage_multiplier() const;
  virtual void      teleport( double yards, timespan_t duration );
  virtual void      trigger_movement( double distance, movement_direction_e direction );
  virtual void      interrupt();
  virtual void      reset();
  virtual void      moving();
  virtual void      init_rng();
  virtual void      create_options();
  virtual action_t* create_proc_action( const std::string& name );
  virtual bool      create_profile( std::string& profile_str, save_e type, bool save_html );
  virtual void      invalidate_cache( cache_e );
  virtual double    temporary_movement_modifier() const;

  void              apl_precombat();
  void              apl_default();
  void              apl_fury();
  void              apl_arms();
  void              apl_prot();
  void              apl_glad();
  virtual void      init_action_list();

  virtual action_t*  create_action( const std::string& name, const std::string& options );
  virtual resource_e primary_resource() const { return RESOURCE_RAGE; }
  virtual role_e     primary_role() const;
  virtual stat_e     convert_hybrid_stat( stat_e s ) const;
  virtual void       assess_damage( school_e, dmg_e, action_state_t* s );
  virtual void       copy_from( player_t* source );

  // Custom Warrior Functions
  void enrage();
  void stance_swap();

  target_specific_t<warrior_td_t*> target_data;

  virtual warrior_td_t* get_target_data( player_t* target ) const
  {
    warrior_td_t*& td = target_data[target];

    if ( !td )
    {
      td = new warrior_td_t( target, const_cast<warrior_t&>( *this ) );
    }
    return td;
  }
};

namespace
{ // UNNAMED NAMESPACE
// Template for common warrior action code. See priest_action_t.
template <class Base>
struct warrior_action_t: public Base
{
  int stancemask;
  bool headlongrush;
  bool headlongrushgcd;
  bool recklessness;
  bool weapons_master;
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef warrior_action_t base_t;

  warrior_action_t( const std::string& n, warrior_t* player,
                    const spell_data_t* s = spell_data_t::nil() ):
                    ab( n, player, s ),
                    stancemask( STANCE_BATTLE | STANCE_DEFENSE | STANCE_GLADIATOR ),
                    headlongrush( ab::data().affected_by( player -> spell.headlong_rush -> effectN( 1 ) ) ),
                    headlongrushgcd( ab::data().affected_by( player -> spell.headlong_rush -> effectN( 2 ) ) ),
                    recklessness( ab::data().affected_by( player -> spec.recklessness -> effectN( 1 ) ) ),
                    weapons_master( ab::data().affected_by( player -> mastery.weapons_master -> effectN( 1 ) ) )
  {
    ab::may_crit = true;
  }

  virtual ~warrior_action_t() {}

  warrior_t* p()
  {
    return debug_cast<warrior_t*>( ab::player );
  }
  const warrior_t* p() const
  {
    return debug_cast<warrior_t*>( ab::player );
  }

  warrior_td_t* td( player_t* t ) const
  {
    return p() -> get_target_data( t );
  }

  virtual double target_armor( player_t* t ) const
  {
    double a = ab::target_armor( t );

    a *= 1.0 - td( t ) -> debuffs_colossus_smash -> current_value;

    return a;
  }

  virtual double action_multiplier() const
  {
    double am = ab::action_multiplier();

    if ( weapons_master )
    {
      am *= 1.0 + ab::player -> cache.mastery_value();
    }

    return am;
  }

  virtual double composite_crit() const
  {
    double cc = ab::composite_crit();

    if ( recklessness )
      cc += p() -> buff.recklessness -> value();

    return cc;
  }

  virtual void execute()
  {
    if ( p() -> stance_dance )
    { // Part of automatic stance swapping, if the player is not in the default stance for the spec this will force them back to it.
      // This is done because blizzard has enabled some automated swapping in game, and because most players use macros to swap back to the proper stance.
      if ( p() -> cooldown.stance_swap -> up() )
      {
        switch ( p() -> active.stance )
        {
        case STANCE_DEFENSE:
        { // If the player is currently in defensive stance, should not be in defensive stance, and the ability can be used in battle stance
          if ( p() -> specialization() != WARRIOR_PROTECTION && ( ( stancemask & STANCE_BATTLE ) != 0 ) )
            p() -> stance_swap(); // Swap back. The stance isn't actually swapped for about 0.2-0.3~ seconds in game, which is modeled here by not officially swapping stances
          break;                  // until the buff has been applied, and the buff has a 0.25~ second delay.
        }
        case STANCE_BATTLE:
        { // If player is in battle stance, is a tank, can use the ability in defensive stance, and is really reallly a tank 
          // and not some strange person trying to dps in battle stance without gladiator's resolve :p
          if ( p() -> specialization() == WARRIOR_PROTECTION && ( ( stancemask & STANCE_DEFENSE ) != 0 ) && p() -> primary_role() == ROLE_TANK )
            p() -> stance_swap(); // Swap back to defensive stance.
          break;
        }
        default:break;
        }
      }
      p() -> stance_dance = false; // Reset variable.
    }
    ab::execute();
  }

  virtual timespan_t gcd() const
  {
    timespan_t t = ab::action_t::gcd();

    if ( t == timespan_t::zero() )
      return t;

    if ( headlongrushgcd )
      t *= ab::player -> cache.attack_haste();
    if ( t < ab::min_gcd )
      t = ab::min_gcd;

    return t;
  }

  virtual double cooldown_reduction() const
  {
    double cdr = ab::cooldown_reduction();

    if ( headlongrush )
    {
      cdr *= ab::player -> cache.attack_haste();
    }

    return cdr;
  }

  virtual bool ready()
  {
    if ( !ab::ready() )
      return false;

    if ( p() -> current.distance_to_move > ab::range && ab::range != -1 )
      // -1 melee range implies that the ability can be used at any distance from the target. Battle shout, stance swaps, etc.
      return false;

    // Attack available in current stance?
    if ( ( stancemask & p() -> active.stance ) == 0 )
    {
      if ( p() -> cooldown.stance_swap -> up() && !p() -> player_override_stance_dance )
      {
        p() -> stance_swap(); // Force the stance swap, the sim will try to use it again in 0.1 seconds.
      }
      return false;
    }

    return true;
  }

  bool usable_moving() const
  { // All warrior abilities are usable while moving, the issue is being in range.
    return true;
  }

  virtual void impact( action_state_t* s )
  {
    ab::impact( s );

    if ( ab::sim -> log || ab::sim -> debug )
    {
      ab::sim -> out_debug.printf(
        "Strength: %4.4f, AP: %4.4f, Crit: %4.4f%%, Crit Dmg Mult: %4.4f,  Mastery: %4.4f%%, Multistrike: %4.4f%%, Haste: %4.4f%%, Versatility: %4.4f%%, Bonus Armor: %4.4f, Tick Multiplier: %4.4f, Direct Multiplier: %4.4f, Action Multiplier: %4.4f",
        p() -> cache.strength(),
        p() -> cache.attack_power() * p() -> composite_attack_power_multiplier(),
        p() -> cache.attack_crit() * 100,
        p() -> composite_player_critical_damage_multiplier(),
        p() -> cache.mastery_value() * 100,
        p() -> cache.multistrike() * 100,
        ( ( 1 / ( p() -> cache.attack_haste() ) ) - 1 ) * 100,
        p() -> cache.damage_versatility() * 100,
        p() -> cache.bonus_armor(),
        s -> composite_ta_multiplier(),
        s -> composite_da_multiplier(),
        s -> action -> action_multiplier() );
    }
  }

  virtual void tick( dot_t* d )
  {
    ab::tick( d );

    if ( ab::sim -> log || ab::sim -> debug )
    {
      ab::sim -> out_debug.printf(
        "Strength: %4.4f, AP: %4.4f, Crit: %4.4f%%, Crit Dmg Mult: %4.4f,  Mastery: %4.4f%%, Multistrike: %4.4f%%, Haste: %4.4f%%, Versatility: %4.4f%%, Bonus Armor: %4.4f, Tick Multiplier: %4.4f, Direct Multiplier: %4.4f, Action Multiplier: %4.4f",
        p() -> cache.strength(),
        p() -> cache.attack_power() * p() -> composite_attack_power_multiplier(),
        p() -> cache.attack_crit() * 100,
        p() -> composite_player_critical_damage_multiplier(),
        p() -> cache.mastery_value() * 100,
        p() -> cache.multistrike() * 100,
        ( ( 1 / ( p() -> cache.attack_haste() ) ) - 1 ) * 100,
        p() -> cache.damage_versatility() * 100,
        p() -> cache.bonus_armor(),
        d -> state -> composite_ta_multiplier(),
        d -> state -> composite_da_multiplier(),
        d -> state -> action -> action_ta_multiplier() );
    }
  }

  virtual void consume_resource()
  {
    ab::consume_resource();

    double rage = ab::resource_consumed;

    if ( p() -> talents.anger_management -> ok() )
    {
      anger_management( rage );
    }

    if ( ab::result_is_miss( ab::execute_state -> result ) && rage > 0 && !ab::aoe )
      p() -> resource_gain( RESOURCE_RAGE, rage*0.8, p() -> gain.avoided_attacks );
  }

  virtual void anger_management( double rage )
  {
    if ( rage > 0 )
    {
      //Anger management takes the amount of rage spent and reduces the cooldown of abilities by 1 second per 30 rage.
      rage /= p() -> talents.anger_management -> effectN( 1 ).base_value();
      rage *= -1;

      // All specs
      p() -> cooldown.heroic_leap -> adjust( timespan_t::from_seconds( rage ) );
      // Fourth Tier Talents
      if ( p() -> talents.storm_bolt -> ok() )
        p() -> cooldown.storm_bolt -> adjust( timespan_t::from_seconds( rage ) );
      else if ( p() -> talents.dragon_roar -> ok() )
        p() -> cooldown.dragon_roar -> adjust( timespan_t::from_seconds( rage ) );
      else if ( p() -> talents.shockwave -> ok() )
        p() -> cooldown.shockwave -> adjust( timespan_t::from_seconds( rage ) );
      // Sixth tier talents
      if ( p() -> talents.bladestorm -> ok() )
        p() -> cooldown.bladestorm -> adjust( timespan_t::from_seconds( rage ) );
      else if ( p() -> talents.bloodbath -> ok() )
        p() -> cooldown.bloodbath -> adjust( timespan_t::from_seconds( rage ) );
      else if ( p() -> talents.avatar -> ok() )
        p() -> cooldown.avatar -> adjust( timespan_t::from_seconds( rage ) );

      if ( p() -> specialization() != WARRIOR_PROTECTION )
      {
        p() -> cooldown.recklessness -> adjust( timespan_t::from_seconds( rage ) );
        p() -> cooldown.die_by_the_sword -> adjust( timespan_t::from_seconds( rage ) );
      }
      else
      {
        p() -> cooldown.demoralizing_shout -> adjust( timespan_t::from_seconds( rage ) );
        p() -> cooldown.last_stand -> adjust( timespan_t::from_seconds( rage ) );
        p() -> cooldown.shield_wall -> adjust( timespan_t::from_seconds( rage ) );
      }
    }
  }
};

// ==========================================================================
// Warrior Heals
// ==========================================================================

struct warrior_heal_t: public warrior_action_t < heal_t >
{
  warrior_heal_t( const std::string& n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ):
    base_t( n, p, s )
  {
    may_crit = tick_may_crit = hasted_ticks = false;
    target = p;
  }
};

// ==========================================================================
// Warrior Attack
// ==========================================================================

struct warrior_attack_t: public warrior_action_t < melee_attack_t >
{
  warrior_attack_t( const std::string& n, warrior_t* p,
                    const spell_data_t* s = spell_data_t::nil() ):
                    base_t( n, p, s )
  {
    special = true;
  }

  virtual void   execute();

  virtual void   impact( action_state_t* s );

  virtual double calculate_weapon_damage( double attack_power )
  {
    double dmg = base_t::calculate_weapon_damage( attack_power );

    // Catch the case where weapon == 0 so we don't crash/retest below.
    if ( dmg == 0.0 )
      return 0;

    if ( weapon -> slot == SLOT_OFF_HAND )
    {
      dmg *= 1.0 + p() -> spec.crazed_berserker -> effectN( 2 ).percent();

      if ( p() -> main_hand_weapon.group() == WEAPON_1H &&
           p() -> off_hand_weapon.group() == WEAPON_1H )
           dmg *= 1.0 + p() -> spec.singleminded_fury -> effectN( 2 ).percent();
    }
    return dmg;
  }

  // helper functions

  void trigger_bloodbath_dot( player_t* t, double dmg )
  {
    residual_action::trigger(
      p() -> active.bloodbath_dot, // ignite spell
      t, // target
      p() -> buff.bloodbath -> data().effectN( 1 ).percent() * dmg );
  }
};

// trigger_bloodbath ========================================================

struct bloodbath_dot_t: public residual_action::residual_periodic_action_t < warrior_attack_t >
{
  bloodbath_dot_t( warrior_t* p ):
    base_t( "bloodbath", p, p -> find_spell( 113344 ) )
  {
    dual = true;
  }
};

// ==========================================================================
// Static Functions
// ==========================================================================

// trigger_sweeping_strikes =================================================

static void trigger_sweeping_strikes( action_state_t* s )
{
  struct sweeping_strikes_aoe_attack_t: public warrior_attack_t
  {
    sweeping_strikes_aoe_attack_t( warrior_t* p ):
      warrior_attack_t( "sweeping_strikes_attack", p, p -> spec.sweeping_strikes )
    {
      may_miss = may_dodge = may_parry = may_crit = may_block = callbacks = false;
      may_multistrike = 1; // Yep. It can multistrike.
      aoe = 1;
      school = SCHOOL_PHYSICAL;
      weapon = &p -> main_hand_weapon;
      weapon_multiplier = 0.5;
      base_costs[RESOURCE_RAGE] = 0; //Resource consumption already accounted for in the buff application.
      cooldown -> duration = timespan_t::zero(); // Cooldown accounted for in the buff.
    }

    size_t available_targets( std::vector< player_t* >& tl ) const
    {
      tl.clear();

      for ( size_t i = 0; i < sim -> target_non_sleeping_list.size(); i++ )
      {
        if ( sim -> target_non_sleeping_list[i] != target )
          tl.push_back( sim -> target_non_sleeping_list[i] );
      }

      return tl.size();
    }

    void execute()
    {
      warrior_attack_t::execute();
      if ( result_is_hit( execute_state -> result ) && p() -> glyphs.sweeping_strikes -> ok() )
        p() -> resource_gain( RESOURCE_RAGE, p() -> glyphs.sweeping_strikes -> effectN( 1 ).base_value(), p() -> gain.sweeping_strikes );
    }
  };

  struct sweeping_strikes_attack_t: public warrior_attack_t
  {
    double pct_damage;
    sweeping_strikes_attack_t( warrior_t* p ):
      warrior_attack_t( "sweeping_strikes_attack", p, p -> spec.sweeping_strikes )
    {
      may_miss = may_dodge = may_parry = may_crit = may_block = callbacks = false;
      may_multistrike = 1;
      aoe = 1;
      weapon_multiplier = 0;
      base_costs[RESOURCE_RAGE] = 0; //Resource consumption already accounted for in the buff application.
      cooldown -> duration = timespan_t::zero(); // Cooldown accounted for in the buff.
      pct_damage = data().effectN( 1 ).percent();
    }

    double composite_player_multiplier() const
    {
      return 1; // No double dipping
    }

    void execute()
    {
      base_dd_max *= pct_damage; //Deals 50% of original damage
      base_dd_min *= pct_damage;

      warrior_attack_t::execute();

      if ( result_is_hit( execute_state -> result ) && p() -> glyphs.sweeping_strikes -> ok() )
        p() -> resource_gain( RESOURCE_RAGE, p() -> glyphs.sweeping_strikes -> effectN( 1 ).base_value(), p() -> gain.sweeping_strikes );
    }

    size_t available_targets( std::vector< player_t* >& tl ) const
    {
      tl.clear();

      for ( size_t i = 0; i < sim -> target_non_sleeping_list.size(); i++ )
      {
        if ( sim -> target_non_sleeping_list[i] != target )
          tl.push_back( sim -> target_non_sleeping_list[i] );
      }

      return tl.size();
    }
  };

  warrior_t* p = debug_cast<warrior_t*>( s -> action -> player );

  if ( s -> action -> id == p -> spec.sweeping_strikes -> id() )
    return;

  if ( s -> action -> result_is_miss( s -> result ) )
    return;

  if ( s -> action -> sim -> active_enemies == 1 )
    return;

  if ( s -> result_total <= 0 )
    return;

  if ( !p -> active.sweeping_strikes )
  {
    p -> active.sweeping_strikes = new sweeping_strikes_attack_t( p );
    p -> active.sweeping_strikes -> init();
  }

  if ( !p -> active.aoe_sweeping_strikes )
  {
    p -> active.aoe_sweeping_strikes = new sweeping_strikes_aoe_attack_t( p );
    p -> active.aoe_sweeping_strikes -> init();
  }

  if ( !s -> action -> is_aoe() )
  {
    p -> active.sweeping_strikes -> base_dd_min = s -> result_total;
    p -> active.sweeping_strikes -> base_dd_max = s -> result_total;
    p -> active.sweeping_strikes -> execute();
  }
  else
  {
    // aoe abilities proc a sweeping strike that deals half the damage of a autoattack
    p -> active.aoe_sweeping_strikes -> execute();
  }

  return;
}

// ==========================================================================
// Warrior Attacks
// ==========================================================================

// warrior_attack_t::execute ================================================

void warrior_attack_t::execute()
{
  base_t::execute();
  if ( p() -> buff.sweeping_strikes -> up() )
    trigger_sweeping_strikes( execute_state );
  }

// warrior_attack_t::impact =================================================

void warrior_attack_t::impact( action_state_t* s )
{
  base_t::impact( s );

  if ( s -> result_amount > 0 )
  {
    if ( result_is_hit_or_multistrike( s -> result ) )
    {
      if ( p() -> buff.bloodbath -> up() && special )
        trigger_bloodbath_dot( s -> target, s -> result_amount );

      if ( p() -> active.second_wind )
      {
        if ( p() -> resources.current[RESOURCE_HEALTH] < p() -> resources.max[RESOURCE_HEALTH] * 0.35 )
        {
          p() -> active.second_wind -> base_dd_min = s -> result_amount;
          p() -> active.second_wind -> base_dd_max = s -> result_amount;
          p() -> active.second_wind -> execute();
        }
      }
      if ( p() -> active.rallying_cry_heal )
      {
        if ( p() -> buff.rallying_cry -> up() )
        {
          p() -> active.rallying_cry_heal -> base_dd_min = s -> result_amount;
          p() -> active.rallying_cry_heal -> base_dd_max = s -> result_amount;
          p() -> active.rallying_cry_heal -> execute();
        }
      }
    }
  }
}

// Melee Attack =============================================================

struct melee_t: public warrior_attack_t
{
  bool mh_lost_melee_contact, oh_lost_melee_contact, sudden_death;
  double base_rage_generation, arms_battle_stance, arms_defensive_stance, battle_stance;
  melee_t( const std::string& name, warrior_t* p ):
    warrior_attack_t( name, p, spell_data_t::nil() ),
    mh_lost_melee_contact( true ), oh_lost_melee_contact( true ),
    sudden_death( false ),
    base_rage_generation( 1.75 ),
    arms_battle_stance( 3.40 ), arms_defensive_stance( 1.60 ),
    battle_stance( 2.00 )
  {
    school = SCHOOL_PHYSICAL;
    special = false;
    background = repeating = auto_attack = may_glance = true;
    trigger_gcd = timespan_t::zero();
    if ( p -> dual_wield() )
      base_hit -= 0.19;
    if ( p -> talents.sudden_death -> ok() )
      sudden_death = true;
  }

  void reset()
  {
    warrior_attack_t::reset();
    mh_lost_melee_contact = true;
    oh_lost_melee_contact = true;
  }

  timespan_t execute_time() const
  {
    timespan_t t = warrior_attack_t::execute_time();
    if ( weapon -> slot == SLOT_MAIN_HAND && mh_lost_melee_contact )
      return timespan_t::zero(); // If contact is lost, the attack is instant.
    else if ( weapon -> slot == SLOT_OFF_HAND && oh_lost_melee_contact ) // Also used for the first attack.
      return timespan_t::zero();
    else
      return t;
  }

  void schedule_execute( action_state_t* s )
  {
    warrior_attack_t::schedule_execute( s );
    if ( weapon -> slot == SLOT_MAIN_HAND )
      mh_lost_melee_contact = false;
    else if ( weapon -> slot == SLOT_OFF_HAND )
      oh_lost_melee_contact = false;
  }

  void execute()
  {
    if ( p() -> current.distance_to_move > 5 )
    { // Cancel autoattacks, auto_attack_t will restart them when we're back in range.
      if ( weapon -> slot == SLOT_MAIN_HAND )
      {
        p() -> proc.delayed_auto_attack -> occur();
        mh_lost_melee_contact = true;
        player -> main_hand_attack -> cancel();
      }
      else
      {
        p() -> proc.delayed_auto_attack -> occur();
        oh_lost_melee_contact = true;
        player -> off_hand_attack -> cancel();
      }
    }
    else
    {
      warrior_attack_t::execute();
    }
  }

  void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) || result_is_block( s -> block_result ) )
    {
      trigger_rage_gain( s );
      if ( sudden_death && p() -> rppm.sudden_death -> trigger() )
      {
        p() -> buff.sudden_death -> trigger();
      }
      if ( p() -> active.enhanced_rend )
      {
        if ( td( s -> target ) -> dots_rend -> is_ticking() )
        {
          p() -> active.enhanced_rend -> target = s -> target;
          p() -> active.enhanced_rend -> execute();
        }
      }
    }

    if ( p() -> active.blood_craze )
    {
      if ( result_is_multistrike( s -> result ) )
      {
        p() -> buff.blood_craze -> trigger();
        residual_action::trigger(
          p() -> active.blood_craze, // ignite spell
          p(), // target
          p() -> spec.blood_craze -> effectN( 1 ).percent() * p() -> resources.max[RESOURCE_HEALTH] );
      }
    }
  }

  void trigger_rage_gain( action_state_t* s )
  {
    if ( p() -> active.stance != STANCE_BATTLE && p() -> specialization() != WARRIOR_ARMS )
      return;

    weapon_t*  w = weapon;
    double rage_gain = w -> swing_time.total_seconds() * base_rage_generation;

    if ( p() -> specialization() == WARRIOR_ARMS )
    {
      if ( p() -> active.stance == STANCE_BATTLE )
      {
        if ( s -> result == RESULT_CRIT )
          rage_gain *= rng().range( 7.4375, 7.875 ); // Wild random numbers appear! Accurate as of 2015/02/02
        else
          rage_gain *= arms_battle_stance;
      }
      else
      {
        if ( s -> result == RESULT_CRIT )
          rage_gain *= arms_battle_stance; // Grants battle stance rage if the crit occurs while in defensive.
        else
          rage_gain *= arms_defensive_stance;
      }
    }
    else
    {
      rage_gain *= battle_stance;
      if ( w -> slot == SLOT_OFF_HAND )
        rage_gain *= 0.5;
    }

    rage_gain = util::round( rage_gain, 1 );

    if ( p() -> specialization() == WARRIOR_ARMS && s -> result == RESULT_CRIT )
    {
      p() -> resource_gain( RESOURCE_RAGE,
        rage_gain,
        p() -> gain.melee_crit );
    }
    else
    {
      p() -> resource_gain( RESOURCE_RAGE,
        rage_gain,
        w -> slot == SLOT_OFF_HAND ? p() -> gain.melee_off_hand : p() -> gain.melee_main_hand );
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t: public warrior_attack_t
{
  auto_attack_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "auto_attack", p )
  {
    parse_options( options_str );
    stancemask = STANCE_GLADIATOR | STANCE_DEFENSE | STANCE_BATTLE;
    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    ignore_false_positive = true;
    range = 5;

    if ( p -> main_hand_weapon.type == WEAPON_2H && p -> has_shield_equipped() && p -> specialization() != WARRIOR_FURY )
    {
      sim -> errorf( "Player %s is using a 2 hander + shield while specced as Protection or Arms. Disabling autoattacks.", name() );
    }
    else
    {
      p -> main_hand_attack = new melee_t( "auto_attack_mh", p );
      p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
      p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;
    }

    if ( p -> off_hand_weapon.type != WEAPON_NONE && p -> specialization() == WARRIOR_FURY )
    {
      p -> off_hand_attack = new melee_t( "auto_attack_oh", p );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
      p -> off_hand_attack -> id = 1;
    }
    trigger_gcd = timespan_t::zero();
  }

  void execute()
  {
    if ( p() -> main_hand_attack -> execute_event == 0 )
      p() -> main_hand_attack -> schedule_execute();
    if ( p() -> off_hand_attack )
    {
      if ( p() -> off_hand_attack -> execute_event == 0 )
        p() -> off_hand_attack -> schedule_execute();
    }
  }

  bool ready()
  {
    bool ready = warrior_attack_t::ready();
    if ( ready ) // Range check
    {
      if ( p() -> main_hand_attack -> execute_event == 0 )
        return ready;
      if ( p() -> off_hand_attack )
      { // Don't check for execute_event if we don't have an offhand.
        if ( p() -> off_hand_attack -> execute_event == 0 )
          return ready;
      }
    }
    return false;
  }
};

// Bladestorm ===============================================================

struct bladestorm_tick_t: public warrior_attack_t
{
  bladestorm_tick_t( warrior_t* p, const std::string& name ):
    warrior_attack_t( name, p, p -> talents.bladestorm -> effectN( 1 ).trigger() )
  {
    dual = true;
    aoe = -1;
    weapon_multiplier *= 1.0 + p -> spec.seasoned_soldier -> effectN( 2 ).percent();
  }

  double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> has_shield_equipped() )
      am *= 1.0 + p() -> spec.protection -> effectN( 1 ).percent();

    return am;
  }
};

struct bladestorm_t: public warrior_attack_t
{
  attack_t* bladestorm_mh;
  attack_t* bladestorm_oh;
  bladestorm_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "bladestorm", p, p -> talents.bladestorm ),
    bladestorm_mh( new bladestorm_tick_t( p, "bladestorm_mh" ) ),
    bladestorm_oh( 0 )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;

    channeled = tick_zero = true;
    callbacks = interrupt_auto_attack = false;
    
    bladestorm_mh -> weapon = &( player -> main_hand_weapon );
    add_child( bladestorm_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE && player -> specialization() == WARRIOR_FURY )
    {
      bladestorm_oh = new bladestorm_tick_t( p, "bladestorm_oh" );
      bladestorm_oh -> weapon = &( player -> off_hand_weapon );
      add_child( bladestorm_oh );
    }
  }

  void execute()
  {
    warrior_attack_t::execute();
    p() -> buff.bladestorm -> trigger();
  }

  void tick( dot_t* d )
  {
    warrior_attack_t::tick( d );
    bladestorm_mh -> execute();

    if ( bladestorm_mh -> result_is_hit( execute_state -> result ) && bladestorm_oh )
      bladestorm_oh -> execute();
  }

  void last_tick( dot_t*d )
  {
    warrior_attack_t::last_tick( d );
    p() -> buff.bladestorm -> expire();
  }
  // Bladestorm is not modified by haste effects
  double composite_haste() const { return 1.0; }
};

// Bloodthirst Heal =========================================================

struct bloodthirst_heal_t: public warrior_heal_t
{
  double base_pct_heal;
  bloodthirst_heal_t( warrior_t* p ):
    warrior_heal_t( "bloodthirst_heal", p, p -> find_spell( 117313 ) ),
    base_pct_heal( 0 )
  {
    pct_heal = data().effectN( 1 ).percent();
    pct_heal *= 1.0 + p -> glyphs.bloodthirst -> effectN( 2 ).percent();
    base_pct_heal = pct_heal;
    background = true;
  }

  resource_e current_resource() const { return RESOURCE_NONE; }

  double calculate_direct_amount( action_state_t* state )
  {
    pct_heal = base_pct_heal;

    if ( p() -> buff.raging_blow_glyph -> up() )
    {
      pct_heal *= 1.0 + p() -> buff.raging_blow_glyph -> data().effectN( 1 ).percent();
      p() -> buff.raging_blow_glyph -> expire();
    }

    return warrior_heal_t::calculate_direct_amount( state );
  }
};

// Bloodthirst ==============================================================

struct bloodthirst_t: public warrior_attack_t
{
  bloodthirst_heal_t* bloodthirst_heal;
  double crit_chance;
  bloodthirst_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "bloodthirst", p, p -> spec.bloodthirst ),
    bloodthirst_heal( NULL )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_DEFENSE;

    crit_chance = data().effectN( 4 ).percent();

    if ( p -> talents.unquenchable_thirst -> ok() )
      cooldown -> duration = timespan_t::zero();

    weapon = &( p -> main_hand_weapon );
    bloodthirst_heal = new bloodthirst_heal_t( p );
    weapon_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 2 ).percent();
    weapon_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).percent();
  }

  double composite_crit() const
  {
    double c = warrior_attack_t::composite_crit();

    c += crit_chance;

    if ( p() -> buff.pvp_2pc_fury -> up() )
    {
      c += p() -> buff.pvp_2pc_fury -> default_value;
      p() -> buff.pvp_2pc_fury -> expire();
    }

    return c;
  }

  void execute()
  {
    warrior_attack_t::execute();
    p() -> buff.tier16_4pc_death_sentence -> trigger();

    if ( result_is_hit( execute_state -> result ) )
    {
      bloodthirst_heal -> execute();

      if ( execute_state -> result == RESULT_CRIT )
        p() -> enrage();

      int bloodsurge = 0;

      if ( p() -> buff.bloodsurge -> check() )
        bloodsurge = p() -> buff.bloodsurge -> current_stack;
      if ( p() -> buff.bloodsurge -> trigger( p() -> spec.bloodsurge -> effectN( 2 ).base_value() ) )
      {
        if ( bloodsurge > 0 )
        {
          do
          {
            p() -> proc.bloodsurge_wasted -> occur();
            bloodsurge--;
          } while ( bloodsurge > 0 );
        }
      }
    }

    p() -> resource_gain( RESOURCE_RAGE,
      data().effectN( 3 ).resource( RESOURCE_RAGE ),
      p() -> gain.bloodthirst );
  }
};

// Charge ===================================================================

struct charge_t: public warrior_attack_t
{
  bool first_charge;
  double movement_speed_increase;
  double min_range;
  double rage_gain;
  charge_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "charge", p, p -> spell.charge ),
    first_charge( true ), 
    movement_speed_increase( 5.0 ),

    min_range( data().min_range() ),
    rage_gain( data().effectN( 2 ).resource( RESOURCE_RAGE ) )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    ignore_false_positive = use_off_gcd = true;
    rage_gain += p -> glyphs.bull_rush -> effectN( 2 ).resource( RESOURCE_RAGE );

    range += p -> glyphs.long_charge -> effectN( 1 ).base_value();
    movement_directionality = MOVEMENT_OMNI;

    cooldown -> duration = data().cooldown();
    if ( p -> talents.double_time -> ok() )
      cooldown -> charges = p -> talents.double_time -> effectN( 1 ).base_value();
    else if ( p -> talents.juggernaut -> ok() )
      cooldown -> duration += p -> talents.juggernaut -> effectN( 1 ).time_value();
  }

  void execute()
  {
    if ( p() -> current.distance_to_move > 5 )
    {
      p() -> buff.charge_movement -> trigger( 1, movement_speed_increase, 1, timespan_t::from_seconds(
        p() -> current.distance_to_move / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() + movement_speed_increase ) ) ) );
      p() -> current.moving_away = 0;
    }

    warrior_attack_t::execute();

    p() -> buff.pvp_2pc_arms -> trigger();
    p() -> buff.pvp_2pc_fury -> trigger();
    p() -> buff.pvp_2pc_prot -> trigger();

    if ( first_charge )
      first_charge = !first_charge;

    if ( p() -> cooldown.rage_from_charge -> up() && !td( execute_state -> target ) -> debuffs_charge -> up() )
    { // Blizz hack, not mine. Charge will not grant rage unless the last target charged was different than the current. 
      p() -> cooldown.rage_from_charge -> start();
      p() -> resource_gain( RESOURCE_RAGE, rage_gain, p() -> gain.charge );
      td( execute_state -> target ) -> debuffs_charge -> trigger();
      if ( p() -> last_target_charged )
        td( p() -> last_target_charged ) -> debuffs_charge -> expire();
      p() -> last_target_charged = execute_state -> target;
    }
  }

  void reset()
  {
    action_t::reset();
    first_charge = true;
  }

  bool ready()
  {
    if ( first_charge == true ) // Assumes that we charge into combat, instead of setting initial distance to 20 yards.
      return warrior_attack_t::ready();

    if ( p() -> current.distance_to_move < min_range ) // Cannot charge if too close to the target.
      return false;

    if ( p() -> buff.charge_movement -> check() || p() -> buff.heroic_leap_movement -> check()
      || p() -> buff.intervene_movement -> check() || p() -> buff.shield_charge -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Colossus Smash ===========================================================

struct colossus_smash_t: public warrior_attack_t
{
  colossus_smash_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "colossus_smash", p, p -> spec.colossus_smash )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE;

    weapon = &( player -> main_hand_weapon );
    base_costs[RESOURCE_RAGE] *= 1.0 + p -> sets.set( WARRIOR_ARMS, T17, B4 ) -> effectN( 2 ).percent();
  }

  double target_armor( player_t* t ) const
  {
      return attack_t::target_armor( t ); // Skip warrior target armor so that multistrikes from colossus smash do not benefit from colossus smash.
  } // If they ever bring back a resetting colossus smash, this will need to be adjusted.

  void execute()
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      td( execute_state -> target ) -> debuffs_colossus_smash -> trigger( 1, data().effectN( 2 ).percent() );
      p() -> buff.colossus_smash -> trigger();

      if ( p() -> sets.has_set_bonus( WARRIOR_ARMS, T17, B4 ) )
      {
        p() -> resource_gain( RESOURCE_RAGE, p() -> sets.set( WARRIOR_ARMS, T17, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE ),
        p() -> gain.tier17_4pc_arms );
      }
      if ( p() -> sets.set( WARRIOR_ARMS, T17, B2 ) )
      {
        if ( p() -> buff.tier17_2pc_arms -> trigger() )
          p() -> proc.t17_2pc_arms -> occur();
      }
    }
  }
};

// Demoralizing Shout =======================================================

struct demoralizing_shout: public warrior_attack_t
{
  demoralizing_shout( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "demoralizing_shout", p, p -> spec.demoralizing_shout )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
  }

  void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );
    if ( result_is_hit( s -> result ) )
      td( s -> target ) -> debuffs_demoralizing_shout -> trigger( 1, data().effectN( 1 ).percent() );
  }
};

// Devastate ================================================================

struct devastate_t: public warrior_attack_t
{
  devastate_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "devastate", p, p -> spec.devastate )
  {
    parse_options( options_str );
    stancemask = STANCE_GLADIATOR | STANCE_DEFENSE | STANCE_BATTLE;
    weapon = &( p -> main_hand_weapon );
  }

  void execute()
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> buff.sword_and_board -> trigger() )
        p() -> cooldown.shield_slam -> reset( true );
      if ( p() -> active.deep_wounds )
      {
        p() -> active.deep_wounds -> target = execute_state -> target;
        p() -> active.deep_wounds -> execute();
      }
      if ( p() -> talents.unyielding_strikes -> ok() )
      {
        if ( p() -> buff.unyielding_strikes -> current_stack != p() -> buff.unyielding_strikes -> max_stack() )
          p() -> buff.unyielding_strikes -> trigger( 1 );
      }

      if ( execute_state -> result == RESULT_CRIT )
        p() -> enrage();
    }
  }

  bool ready()
  {
    if ( !p() -> has_shield_equipped() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Dragon Roar ==============================================================

struct dragon_roar_t: public warrior_attack_t
{
  dragon_roar_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "dragon_roar", p, p -> talents.dragon_roar )
  {
    parse_options( options_str );
    aoe = -1;
    may_dodge = may_parry = may_block = false;
    range = p -> find_spell( 118000 ) -> effectN( 1 ).radius_max();
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
  }

  double target_armor( player_t* ) const { return 0; }

  double composite_crit() const { return 1.0; }

  double composite_target_crit( player_t* ) const { return 0.0; }
};

// Execute ==================================================================

struct execute_off_hand_t: public warrior_attack_t
{
  execute_off_hand_t( warrior_t* p, const char* name, const spell_data_t* s ):
    warrior_attack_t( name, p, s )
  {
    dual = true;
    may_miss = may_dodge = may_parry = may_block = false;
    weapon = &( p -> off_hand_weapon );

    if ( p -> wod_hotfix )
    { weapon_multiplier = 3.5; } // Hotfix from 2015-02-27, Fury only.
    if ( p -> main_hand_weapon.group() == WEAPON_1H &&
         p -> off_hand_weapon.group() == WEAPON_1H )
    { weapon_multiplier *= 1.0 + p -> spec.singleminded_fury -> effectN( 3 ).percent(); }

    weapon_multiplier *= 1.0 + p -> perk.empowered_execute -> effectN( 1 ).percent();
  }
};

struct execute_t: public warrior_attack_t
{
  execute_off_hand_t* oh_attack;
  double sudden_death_rage;
  execute_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "execute", p, p -> spec.execute )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    weapon = &( p -> main_hand_weapon );

    sudden_death_rage = 30;

    if ( p -> wod_hotfix && p -> specialization() != WARRIOR_ARMS )
    { weapon_multiplier = 3.5; } // Hotfix from 2015-02-27, Fury/Prot only.

    if ( p -> spec.crazed_berserker -> ok() )
    {
      oh_attack = new execute_off_hand_t( p, "execute_oh", p -> find_spell( 163558 ) );
      add_child( oh_attack );
      if ( p -> main_hand_weapon.group() == WEAPON_1H &&
           p -> off_hand_weapon.group() == WEAPON_1H )
           weapon_multiplier *= 1.0 + p -> spec.singleminded_fury -> effectN( 3 ).percent();
    }
    else if ( p -> specialization() == WARRIOR_ARMS )
    {
      if ( p -> wod_hotfix )
      { weapon_multiplier = 1.35; } // Was hotfixed sometime during PTR.
      sudden_death_rage = 10;
    }

    weapon_multiplier *= 1.0 + p -> perk.empowered_execute -> effectN( 1 ).percent();
  }

  double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> mastery.weapons_master -> ok() )
    {
      if ( target -> health_percentage() < 20 || p() -> buff.tier16_4pc_death_sentence -> check() )
      {
        if ( p() -> buff.sudden_death -> check() )
        { // If the target is below 20% hp, arms execute with sudden death procced will consume x rage, but hit the target as if it had x + 10 rage. 
          am *= 4.0 * std::min( 40.0, ( p() -> resources.current[RESOURCE_RAGE] + sudden_death_rage ) ) / 40;
        }
        else
        {
          am *= 4.0 * std::min( 40.0, ( p() -> resources.current[RESOURCE_RAGE] ) ) / 40;
        }
      }
    }
    else if ( p() -> has_shield_equipped() )
    { am *= 1.0 + p() -> spec.protection -> effectN( 2 ).percent(); }

    return am;
  }

  double cost() const
  {
    double c = warrior_attack_t::cost();

    if ( p() -> mastery.weapons_master -> ok() )
      c = std::min( 40.0, std::max( p() -> resources.current[RESOURCE_RAGE], c ) );

    if ( p() -> buff.tier16_4pc_death_sentence -> up() && target -> health_percentage() < 20 )
      c *= 1.0 + p() -> buff.tier16_4pc_death_sentence -> data().effectN( 2 ).percent();

    if ( p() -> buff.sudden_death -> up() && c > 0.0 )
    {
      if ( p() -> mastery.weapons_master -> ok() && target -> health_percentage() < 20 )
      {
        c -= 10;
      }
      else
      {
        c *= 1.0 + p() -> buff.sudden_death -> data().effectN( 2 ).percent(); // Tier 16 4 piece overrides sudden death.
      }
    }

    return c;
  }

  void execute()
  {
    warrior_attack_t::execute();

    if ( p() -> spec.crazed_berserker -> ok() && result_is_hit( execute_state -> result ) &&
         p() -> off_hand_weapon.type != WEAPON_NONE ) // If MH fails to land, or if there is no OH weapon for Fury, oh attack does not execute.
         oh_attack -> execute();

    p() -> buff.sudden_death -> expire(); // Consumes both buffs
    p() -> buff.tier16_4pc_death_sentence -> expire();
  }

  void anger_management( double )
  {
    double newrage = resource_consumed;
    if ( p() -> buff.sudden_death -> check() )
      newrage += sudden_death_rage;

    base_t::anger_management( newrage );
  }

  bool ready()
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    if ( p() -> buff.sudden_death -> check() || p() -> buff.tier16_4pc_death_sentence -> check() )
      return warrior_attack_t::ready();

    // Call warrior_attack_t::ready() first for proper targeting support.
    if ( warrior_attack_t::ready() && target -> health_percentage() < 20 )
      return true;
    else
      return false;
  }
};

// Hamstring ==============================================================

struct hamstring_t: public warrior_attack_t
{
  hamstring_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "hamstring", p, p -> find_class_spell( "Hamstring" ) )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    weapon = &( p -> main_hand_weapon );
  }
};

// Heroic Strike ============================================================

struct heroic_strike_t: public warrior_attack_t
{
  double anger_management_rage;
  heroic_strike_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "heroic_strike", p, p -> find_class_spell( "Heroic Strike" ) )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;

    weapon = &( player -> main_hand_weapon );

    if ( p -> glyphs.cleave -> ok() )
      aoe = 2;

    anger_management_rage = base_costs[RESOURCE_RAGE];
    use_off_gcd = true;
  }

  double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.shield_charge -> up() )
      am *= 1.0 + p() -> buff.shield_charge -> default_value;

    return am;
  }

  void anger_management( double )
  {
    if ( p() -> buff.ultimatum -> check() ) // Ultimatum does not grant cooldown reduction from anger management.
      return;

    base_t::anger_management( anger_management_rage ); // However, unyielding strikes grants cooldown reduction based on the original cost of heroic strike.
  }

  double cost() const
  {
    double c = warrior_attack_t::cost();

    if ( p() -> buff.unyielding_strikes -> up() )
      c += p() -> buff.unyielding_strikes -> current_stack * p() -> buff.unyielding_strikes -> default_value;

    if ( p() -> buff.ultimatum -> check() )
      c *= 1 + p() -> buff.ultimatum -> data().effectN( 1 ).percent();

    return c;
  }

  double composite_crit() const
  {
    double cc = warrior_attack_t::composite_crit();

    if ( p() -> buff.ultimatum -> check() )
      cc += p() -> buff.ultimatum -> data().effectN( 2 ).percent();

    return cc;
  }

  void execute()
  {
    warrior_attack_t::execute();

    p() -> buff.ultimatum -> expire();
  }

  bool ready()
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Heroic Throw ==============================================================

struct heroic_throw_t: public warrior_attack_t
{
  heroic_throw_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "heroic_throw", p, p -> find_class_spell( "Heroic Throw" ) )
  {
    parse_options( options_str );

    weapon = &( player -> main_hand_weapon );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    may_dodge = may_parry = may_block = false;
    if ( p -> perk.improved_heroic_throw -> ok() )
      cooldown -> duration = timespan_t::zero();
  }

  bool ready()
  {
    if ( p() -> current.distance_to_move > range ||
         p() -> current.distance_to_move < data().min_range() ) // Cannot heroic throw unless target is in range.
         return false;

    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Heroic Leap ==============================================================

struct heroic_leap_t: public warrior_attack_t
{
  const spell_data_t* heroic_leap_damage;
  heroic_leap_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "heroic_leap", p, p -> spell.heroic_leap ),
    heroic_leap_damage( p -> find_spell( 52174 ) )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    ignore_false_positive = true;
    aoe = -1;
    may_dodge = may_parry = may_miss = may_block = false;
    movement_directionality = MOVEMENT_OMNI;
    base_teleport_distance = data().max_range();
    base_teleport_distance += p -> glyphs.death_from_above -> effectN( 2 ).base_value();
    range = -1;
    attack_power_mod.direct = heroic_leap_damage -> effectN( 1 ).ap_coeff();

    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> glyphs.death_from_above -> effectN( 1 ).time_value();
    cooldown -> duration *= p -> buffs.cooldown_reduction -> default_value;
  }

  timespan_t travel_time() const
  {
    return timespan_t::from_seconds( 0.25 );
  }

  void execute()
  {
    warrior_attack_t::execute();
    if ( p() -> current.distance_to_move > 0 && !p() -> buff.heroic_leap_movement -> check() )
    {
      double speed;
      speed = std::min( p() -> current.distance_to_move, base_teleport_distance ) / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() ) ) / travel_time().total_seconds();
      p() -> buff.heroic_leap_movement -> trigger( 1, speed, 1, travel_time() );
    }
  }

  void impact( action_state_t* s )
  {
    if ( p() -> current.distance_to_move > heroic_leap_damage -> effectN( 1 ).radius() )
      s -> result_amount = 0;

    warrior_attack_t::impact( s );

    p() -> buff.heroic_leap_glyph -> trigger();
  }

  bool ready()
  {
    if ( p() -> buff.intervene_movement -> check() || p() -> buff.charge_movement -> check() || p() -> buff.shield_charge -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// 2 Piece Tier 16 Tank Set Bonus ===========================================

struct tier16_2pc_tank_heal_t: public warrior_heal_t
{
  tier16_2pc_tank_heal_t( warrior_t* p ):
    warrior_heal_t( "tier16_2pc_tank_heal", p )
  {}
  resource_e current_resource() const { return RESOURCE_NONE; }
};

// Impending Victory ========================================================

struct impending_victory_heal_t: public warrior_heal_t
{
  double base_pct_heal;
  impending_victory_heal_t( warrior_t* p ):
    warrior_heal_t( "impending_victory_heal", p, p -> find_spell( 118340 ) )
  {
    pct_heal = data().effectN( 1 ).percent();
    base_pct_heal = pct_heal;
    background = true;
  }

  double calculate_direct_amount( action_state_t* state )
  {
    pct_heal = base_pct_heal;

    if ( p() -> buff.tier15_2pc_tank -> check() )
      pct_heal += p() -> buff.tier15_2pc_tank -> value();

    return warrior_heal_t::calculate_direct_amount( state );
  }

  resource_e current_resource() const { return RESOURCE_NONE; }
};

struct impending_victory_t: public warrior_attack_t
{
  impending_victory_heal_t* impending_victory_heal;

  impending_victory_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "impending_victory", p, p -> talents.impending_victory ),
    impending_victory_heal( new impending_victory_heal_t( p ) )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    parse_effect_data( data().effectN( 2 ) ); //Both spell effects list an ap coefficient, #2 is correct.
  }

  void execute()
  {
    warrior_attack_t::execute();

    impending_victory_heal -> execute();

    p() -> buff.tier15_2pc_tank -> decrement();
  }
};

// Intervene ===============================================================
// Note: Conveniently ignores that you can only intervene a friendly target.
// For the time being, we're just going to assume that there is a friendly near the target
// that we can intervene to. Maybe in the future with a more complete movement system, we will
// fix this to work in a raid simulation that includes multiple melee.

struct intervene_t: public warrior_attack_t
{
  double movement_speed_increase;
  intervene_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "intervene", p, p -> spell.intervene ),
    movement_speed_increase( 5.0 )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    ignore_false_positive = true;
    movement_directionality = MOVEMENT_OMNI;
  }

  void execute()
  {
    warrior_attack_t::execute();
    if ( p() -> current.distance_to_move > 0 )
    {
      p() -> buff.intervene_movement -> trigger( 1, movement_speed_increase, 1,
                                                 timespan_t::from_seconds( p() -> current.distance_to_move / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() + movement_speed_increase ) ) ) );
      p() -> current.moving_away = 0;
    }
  }

  bool ready()
  {
    if ( p() -> buff.heroic_leap_movement -> check() || p() -> buff.charge_movement -> check() || p() -> buff.shield_charge -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Mortal Strike ============================================================

struct heroic_charge_movement_ticker_t: public event_t
{
  timespan_t duration;
  warrior_t* warrior;
  heroic_charge_movement_ticker_t( sim_t& s, warrior_t*p, timespan_t d = timespan_t::zero() ):
    event_t( s, p ), warrior( p )
  {
    if ( d > timespan_t::zero() )
      duration = d;
    else
      duration = next_execute();

    add_event( duration );
    if ( sim().debug ) sim().out_debug.printf( "New movement event" );
  }

  timespan_t next_execute() const
  {
    timespan_t min_time = timespan_t::max();
    bool any_movement = false;
    timespan_t time_to_finish = warrior -> time_to_move();
    if ( time_to_finish != timespan_t::zero() )
    {
      any_movement = true;

      if ( time_to_finish < min_time )
        min_time = time_to_finish;
    }

    if ( min_time > timespan_t::from_seconds( 0.05 ) ) // Update a little more than usual, since we're moving a lot faster.
      min_time = timespan_t::from_seconds( 0.05 );

    if ( !any_movement )
      return timespan_t::zero();
    else
      return min_time;
  }

  void execute()
  {
    if ( warrior -> time_to_move() > timespan_t::zero() )
      warrior -> update_movement( duration );

    timespan_t next = next_execute();
    if ( next > timespan_t::zero() )
      warrior -> heroic_charge = new ( sim() ) heroic_charge_movement_ticker_t( sim(), warrior, next );
    else
    {
      warrior -> heroic_charge = 0;
      warrior -> buff.heroic_leap_movement -> expire();
    }
  }
};

struct heroic_charge_t: public warrior_attack_t
{
  action_t*leap;
  heroic_charge_movement_ticker_t* charge;
  heroic_charge_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "heroic_charge", p, spell_data_t::nil() )
  {
    parse_options( options_str );
    stancemask = STANCE_DEFENSE | STANCE_GLADIATOR | STANCE_BATTLE;
    leap = new heroic_leap_t( p, options_str );
    trigger_gcd = timespan_t::zero();
    use_off_gcd = true;
    ignore_false_positive = true;
    callbacks = may_crit = false;
  }

  void execute()
  {
    warrior_attack_t::execute();

    if ( p() -> cooldown.heroic_leap -> up() )
    {// We are moving 10 yards, and heroic leap always executes in 0.25 seconds.
      // Do some hacky math to ensure it will only take 0.25 seconds, since it will certainly
      // be the highest temporary movement speed buff.
      double speed;
      speed = 10 / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() ) ) / 0.25;
      p() -> buff.heroic_leap_movement -> trigger( 1, speed, 1, timespan_t::from_millis( 250 ) );
      leap -> execute();
      p() -> trigger_movement( 10.0, MOVEMENT_BOOMERANG ); // Leap 10 yards out, because it's impossible to precisely land 8 yards away.
      p() -> heroic_charge = new ( *sim ) heroic_charge_movement_ticker_t( *sim, p() );
    }
    else
    {
      p() -> trigger_movement( 9.0, MOVEMENT_BOOMERANG );
      p() -> heroic_charge = new ( *sim ) heroic_charge_movement_ticker_t( *sim, p() );
    }
  }

  bool ready()
  {
    if ( p() -> cooldown.rage_from_charge -> up() && p() -> cooldown.charge -> up() && !p() -> buffs.raid_movement -> check() )
      return warrior_attack_t::ready();
    else
      return false;
  }
};

// Mortal Strike ============================================================

struct mortal_strike_t: public warrior_attack_t
{
  mortal_strike_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "mortal_strike", p, p -> spec.mortal_strike )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_DEFENSE;

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();
    weapon_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).percent();
  }

  void execute()
  {
    warrior_attack_t::execute();
    if ( result_is_hit( execute_state -> result ) )
    {
      if ( sim -> overrides.mortal_wounds )
        execute_state -> target -> debuffs.mortal_wounds -> trigger();
    }
    p() -> buff.tier16_4pc_death_sentence -> trigger();
  }

  double composite_crit() const
  {
    double cc = warrior_attack_t::composite_crit();

    if ( p() -> buff.pvp_2pc_arms -> up() )
    {
      cc += p() -> buff.pvp_2pc_arms -> default_value;
      p() -> buff.pvp_2pc_arms -> expire();
    }

    return cc;
  }

  double cooldown_reduction() const
  {
    double cdr = warrior_attack_t::cooldown_reduction();

    if ( p() -> buff.tier17_2pc_arms -> up() )
      cdr *= 1.0 + p() -> buff.tier17_2pc_arms -> data().effectN( 1 ).percent();

    return cdr;
  }

  bool ready()
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Pummel ===================================================================

struct pummel_t: public warrior_attack_t
{
  pummel_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "pummel", p, p -> find_class_spell( "Pummel" ) )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    ignore_false_positive = true;

    may_miss = may_block = may_dodge = may_parry = callbacks = false;
  }

  void execute()
  {
    warrior_attack_t::execute();
    //p() -> buff.rude_interruption -> trigger();
  }
};

// Raging Blow ==============================================================

struct raging_blow_attack_t: public warrior_attack_t
{
  raging_blow_attack_t( warrior_t* p, const char* name, const spell_data_t* s ):
    warrior_attack_t( name, p, s )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    dual = true;
  }

  void execute()
  {
    aoe = p() -> buff.meat_cleaver -> stack();
    if ( aoe ) ++aoe;

    warrior_attack_t::execute();
  }
  
  void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );
    if ( s -> result == RESULT_CRIT )
    { // Can proc off MH/OH individually from each meat cleaver hit.
      if ( rng().roll( p() -> sets.set( WARRIOR_FURY, T17, B2 ) -> proc_chance() ) )
      {
        p() -> enrage();
        p() -> proc.t17_2pc_fury -> occur();
      }
    }
  }
};

struct raging_blow_t: public warrior_attack_t
{
  raging_blow_attack_t* mh_attack;
  raging_blow_attack_t* oh_attack;

  raging_blow_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "raging_blow", p, p -> spec.raging_blow ),
    mh_attack( NULL ), oh_attack( NULL )
  {
    // Parent attack is only to determine miss/dodge/parry
    weapon_multiplier = attack_power_mod.direct = 0;
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_DEFENSE;

    mh_attack = new raging_blow_attack_t( p, "raging_blow_mh", data().effectN( 1 ).trigger() );
    mh_attack -> weapon = &( p -> main_hand_weapon );
    add_child( mh_attack );

    oh_attack = new raging_blow_attack_t( p, "raging_blow_oh", data().effectN( 2 ).trigger() );
    oh_attack -> weapon = &( p -> off_hand_weapon );
    add_child( oh_attack );
  }

  void execute()
  {
    // check attack
    attack_t::execute();
    if ( result_is_hit( execute_state -> result ) )
    {
      mh_attack -> execute();
      oh_attack -> execute();
      if ( mh_attack -> execute_state -> result == RESULT_CRIT &&
           oh_attack -> execute_state -> result == RESULT_CRIT )
           p() -> buff.raging_blow_glyph -> trigger();
      p() -> buff.raging_wind -> trigger();
      p() -> buff.meat_cleaver -> expire();
    }
    p() -> buff.raging_blow -> decrement(); // Raging blow buff decrements even if the attack doesn't land.
  }

  bool ready()
  {
    if ( !p() -> buff.raging_blow -> check() )
      return false;

    // Needs weapons in both hands
    if ( p() -> main_hand_weapon.type == WEAPON_NONE ||
         p() -> off_hand_weapon.type == WEAPON_NONE )
         return false;

    return warrior_attack_t::ready();
  }
};

// Revenge ==================================================================

struct revenge_t: public warrior_attack_t
{
  stats_t* absorb_stats;
  double rage_gain;
  revenge_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "revenge", p, p -> spec.revenge ),
    absorb_stats( 0 ), rage_gain( 0 )
  {
    parse_options( options_str );
    stancemask = STANCE_GLADIATOR | STANCE_DEFENSE;
    base_add_multiplier = data().effectN( 3 ).percent();
    aoe = 3;
    rage_gain = data().effectN( 2 ).resource( RESOURCE_RAGE );
  }

  double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.shield_charge -> up() )
      am *= 1.0 + p() -> buff.shield_charge -> default_value;

    return am;
  }

  void execute()
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> resource_gain( RESOURCE_RAGE, rage_gain, p() -> gain.revenge );

      if ( p() -> sets.has_set_bonus( SET_TANK, T15, B4 ) )
      {
        if ( td( target ) -> debuffs_demoralizing_shout -> up() )
          p() -> resource_gain( RESOURCE_RAGE,
          rage_gain * p() -> sets.set( SET_TANK, T15, B4 ) -> effectN( 1 ).percent(),
          p() -> gain.tier15_4pc_tank );
      }
      if ( rng().roll( p() -> sets.set( SET_TANK, T15, B2 ) -> proc_chance() ) )
        p() -> buff.tier15_2pc_tank -> trigger();
    }
  }

  bool ready()
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Blood Craze ==============================================================

struct blood_craze_t: public residual_action::residual_periodic_action_t < warrior_heal_t >
{
  blood_craze_t( warrior_t* p ):
    base_t( "blood_craze", p, p -> spec.blood_craze )
  {
    hasted_ticks = harmful = false;
    background = true;
    base_tick_time = p -> find_spell( 159363 ) -> effectN( 1 ).period();
    dot_duration = p -> find_spell( 159363 ) -> duration();
    dot_behavior = DOT_REFRESH;
  }
};

// Second Wind ==============================================================

struct second_wind_t: public warrior_heal_t
{
  double heal_pct;
  second_wind_t( warrior_t* p ):
    warrior_heal_t( "second_wind", p, p -> talents.second_wind )
  {
    callbacks = false;
    background = true;
    heal_pct = data().effectN( 1 ).percent();
  }

  void execute()
  {
    base_dd_max *= heal_pct;
    base_dd_min *= heal_pct;

    warrior_heal_t::execute();
  }
};

// Enraged Regeneration ===============================================

struct enraged_regeneration_t: public warrior_heal_t
{
  enraged_regeneration_t( warrior_t* p, const std::string& options_str ):
    warrior_heal_t( "enraged_regeneration", p, p -> talents.enraged_regeneration )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    dot_duration = data().duration();
    range = -1;
    base_tick_time = data().effectN( 2 ).period();

    pct_heal = data().effectN( 1 ).percent();
    tick_pct_heal = data().effectN( 2 ).percent();
  }

  void execute()
  {
    warrior_heal_t::execute();
    p() -> buff.enraged_regeneration -> trigger();
  }
};

// Rend ==============================================================

struct rend_burst_t: public warrior_attack_t
{
  rend_burst_t( warrior_t* p ):
    warrior_attack_t( "rend_burst", p, p -> find_spell( 94009 ) )
  {
    dual = true;
  }

  double target_armor( player_t* ) const
  {
    return 0.0;
  }
};

struct rend_t: public warrior_attack_t
{
  rend_burst_t* burst;
  rend_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "rend", p, p -> spec.rend ),
    burst( new rend_burst_t( p ) )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_DEFENSE;
    dot_behavior = DOT_REFRESH;
    tick_may_crit = true;
    add_child( burst );
  }

  void impact( action_state_t* s )
  {
    if ( result_is_hit( s -> result ) )
    {
      dot_t* dot = get_dot( s -> target );
      if ( dot -> is_ticking() && dot -> remains() < dot_duration * 0.3 )
        burst -> execute();
    }
    warrior_attack_t::impact( s );
  }

  void tick( dot_t* d )
  {
    warrior_attack_t::tick( d );
    if ( p() -> talents.taste_for_blood -> ok() )
    {
      p() -> resource_gain( RESOURCE_RAGE,
        p() -> talents.taste_for_blood -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE ),
        p() -> gain.taste_for_blood );
    }
  }

  void last_tick( dot_t* d )
  {
    if ( d -> ticks_left() == 0 && !target -> is_sleeping() )
    {
      burst -> execute();
    }
    warrior_attack_t::last_tick( d );
  }

  bool ready()
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Siegebreaker ===============================================================

struct siegebreaker_off_hand_t: public warrior_attack_t
{
  siegebreaker_off_hand_t( warrior_t* p, const char* name, const spell_data_t* s ):
    warrior_attack_t( name, p, s )
  {
    may_dodge = may_parry = may_block = may_miss = false;
    dual = true;
    weapon = &( p -> off_hand_weapon );
  }
};

struct siegebreaker_t: public warrior_attack_t
{
  siegebreaker_off_hand_t* oh_attack;
  siegebreaker_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "siegebreaker", p, p -> talents.siegebreaker ),
    oh_attack( 0 )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_DEFENSE;
    may_dodge = may_parry = may_block = false;
    weapon = &( p -> main_hand_weapon );
    if ( p -> specialization() == WARRIOR_FURY )
    {
      oh_attack = new siegebreaker_off_hand_t( p, "siegebreaker_oh", data().effectN( 5 ).trigger() );
      add_child( oh_attack );
    }
  }

  void execute()
  {
    warrior_attack_t::execute(); // for fury, this is the MH attack

    if ( oh_attack && result_is_hit( execute_state -> result ) &&
         p() -> off_hand_weapon.type != WEAPON_NONE ) // If MH fails to land, OH does not execute.
         oh_attack -> execute();
  }

  bool ready()
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Shield Slam ==============================================================

struct shield_charge_2pc_t: public warrior_attack_t
{
  shield_charge_2pc_t( warrior_t* p ):
    warrior_attack_t( "shield_charge_t17_2pc_proc", p, p -> find_spell( 156321 ) )
  {
    background = true;
    base_costs[ RESOURCE_RAGE ] = 0;
    cooldown -> duration = timespan_t::zero();
  }

  void execute()
  {
    warrior_attack_t::execute();
    if ( p() -> buff.shield_charge -> check() )
      p() -> buff.shield_charge -> extend_duration( p(), p() -> buff.shield_charge -> data().duration() );
    else
      p() -> buff.shield_charge -> trigger();
  }
};

struct shield_block_2pc_t: public warrior_attack_t
{
  shield_block_2pc_t( warrior_t* p ):
    warrior_attack_t( "shield_block_t17_2pc_proc", p, p -> find_class_spell( "Shield Block" ) )
  {
    background = true;
    base_costs[RESOURCE_RAGE] = 0;
    cooldown -> duration = timespan_t::zero();
  }

  void execute()
  {
    warrior_attack_t::execute();
    if ( p() -> buff.shield_block -> check() )
      p() -> buff.shield_block -> extend_duration( p(), p() -> buff.shield_block -> data().duration() );
    else
      p() -> buff.shield_block -> trigger();
  }
};

struct shield_slam_t: public warrior_attack_t
{
  double rage_gain;
  attack_t* shield_charge_2pc;
  attack_t* shield_block_2pc;
  shield_slam_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "shield_slam", p, p -> spec.shield_slam ),
    rage_gain( 0.0 ),
    shield_charge_2pc( new shield_charge_2pc_t( p ) ),
    shield_block_2pc( new shield_block_2pc_t( p ) )
  {
    parse_options( options_str );
    stancemask = STANCE_GLADIATOR | STANCE_DEFENSE | STANCE_BATTLE;
    rage_gain = data().effectN( 3 ).resource( RESOURCE_RAGE );

    attack_power_mod.direct = 0.366; // Low level value for shield slam.
    if ( p -> level >= 80 )
      attack_power_mod.direct += 0.426; // Adds 42.6% ap once the character is level 80
    if ( p -> level >= 85 )
      attack_power_mod.direct += 2.46; // Adds another 246% ap at level 85
    //Shield slam is just the best.
  }

  double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.shield_charge -> up() )
    {
      am *= 1.0 + p() -> buff.shield_charge -> default_value;
      am *= 1.0 + p() -> talents.heavy_repercussions -> effectN( 1 ).percent();
    }
    else if ( p() -> buff.shield_block -> up() )
      am *= 1.0 + p() -> talents.heavy_repercussions -> effectN( 1 ).percent();

    return am;
  }

  double composite_crit() const
  {
    double c = warrior_attack_t::composite_crit();

    if ( p() -> buff.pvp_2pc_prot -> up() )
    {
      c += p() -> buff.pvp_2pc_prot -> default_value;
      p() -> buff.pvp_2pc_prot -> expire();
    }

    return c;
  }

  void execute()
  {
    warrior_attack_t::execute();

    if ( rng().roll( p() -> sets.set( WARRIOR_PROTECTION, T17, B2 ) -> proc_chance() ) )
    {
      if ( p() -> active.stance == STANCE_GLADIATOR )
        shield_charge_2pc -> execute();
      else
        shield_block_2pc -> execute();
    }

    double rage_from_snb = 0;

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> active.stance != STANCE_BATTLE )
      {
        p() -> resource_gain( RESOURCE_RAGE,
          rage_gain,
          p() -> gain.shield_slam );

        if ( p() -> buff.sword_and_board -> up() )
        {
          rage_from_snb = p() -> buff.sword_and_board -> data().effectN( 1 ).resource( RESOURCE_RAGE );
          p() -> resource_gain( RESOURCE_RAGE,
            rage_from_snb,
            p() -> gain.sword_and_board );
        }
        p() -> buff.sword_and_board -> expire();
      }
      if ( rng().roll( p() -> sets.set( SET_TANK, T15, B2 ) -> proc_chance() ) )
        p() -> buff.tier15_2pc_tank -> trigger();

      if ( execute_state -> result == RESULT_CRIT )
      {
        p() -> enrage();
        p() -> buff.ultimatum -> trigger();
      }
    }

    if ( td( execute_state -> target ) -> debuffs_demoralizing_shout -> up() && p() -> sets.has_set_bonus( SET_TANK, T15, B4 ) )
      p() -> resource_gain( RESOURCE_RAGE,
      ( rage_gain + rage_from_snb ) * p() -> sets.set( SET_TANK, T15, B4 ) -> effectN( 1 ).percent(),
      p() -> gain.tier15_4pc_tank );
  }

  bool ready()
  {
    if ( !p() -> has_shield_equipped() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Slam =====================================================================

struct slam_t: public warrior_attack_t
{
  slam_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "slam", p, p -> talents.slam )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_DEFENSE;
    weapon = &( p -> main_hand_weapon );
    base_costs[RESOURCE_RAGE] = 10;
  }

  void execute()
  {
    warrior_attack_t::execute();

    p() -> buff.debuffs_slam -> trigger( 1 );
  }

  double cost() const
  {
    double c = warrior_attack_t::cost();

    c *= 1.0 + p() -> buff.debuffs_slam -> current_stack;

    return c;
  }

  double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.debuffs_slam -> up() )
      am *= 1.0 + ( ( static_cast<double>( p() -> buff.debuffs_slam -> current_stack ) ) / 2.0 );

    return am;
  }

  bool ready()
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Shockwave ================================================================

struct shockwave_t: public warrior_attack_t
{
  shockwave_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "shockwave", p, p -> talents.shockwave )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    may_dodge = may_parry = may_block = false;
    range = data().effectN( 2 ).radius_max();
    aoe = -1;
  }

  void update_ready( timespan_t cd_duration )
  {
    cd_duration = cooldown -> duration;

    if ( execute_state -> n_targets >= 3 )
    {
      if ( cd_duration > timespan_t::from_seconds( 20 ) )
        cd_duration += timespan_t::from_seconds( -20 );
      else
        cd_duration = timespan_t::zero();
    }
    warrior_attack_t::update_ready( cd_duration );
  }
};

// Storm Bolt ===============================================================

struct storm_bolt_off_hand_t: public warrior_attack_t
{
  storm_bolt_off_hand_t( warrior_t* p, const char* name, const spell_data_t* s ):
    warrior_attack_t( name, p, s )
  {
    may_dodge = may_parry = may_block = may_miss = false;
    dual = true;
    weapon = &( p -> off_hand_weapon );
    // assume the target is stun-immune
    weapon_multiplier *= 4.00;
  }
};

struct storm_bolt_t: public warrior_attack_t
{
  storm_bolt_off_hand_t* oh_attack;
  storm_bolt_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "storm_bolt", p, p -> talents.storm_bolt ),
    oh_attack( 0 )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    may_dodge = may_parry = may_block = false;
    // Assuming that our target is stun immune
    weapon_multiplier *= 4.00;

    if ( p -> specialization() == WARRIOR_FURY )
    {
      oh_attack = new storm_bolt_off_hand_t( p, "storm_bolt_offhand", data().effectN( 4 ).trigger() );
      add_child( oh_attack );
    }
  }

  void execute()
  {
    warrior_attack_t::execute(); // for fury, this is the MH attack

    if ( oh_attack && result_is_hit( execute_state -> result ) &&
         p() -> off_hand_weapon.type != WEAPON_NONE ) // If MH fails to land, OH does not execute.
         oh_attack -> execute();
  }

  bool ready()
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Thunder Clap =============================================================

struct thunder_clap_t: public warrior_attack_t
{
  thunder_clap_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "thunder_clap", p, p -> spec.thunder_clap )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    aoe = -1;
    may_dodge = may_parry = may_block = false;
    range = data().effectN( 2 ).radius_max();
    cooldown -> duration = data().cooldown();
    cooldown -> duration *= 1 + p -> glyphs.resonating_power -> effectN( 2 ).percent();
    attack_power_mod.direct *= 1.0 + p -> glyphs.resonating_power -> effectN( 1 ).percent();
  }

  void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( p() -> active.deep_wounds )
    {
      if ( result_is_hit( s -> result ) )
      {
        p() -> active.deep_wounds -> target = s -> target;
        p() -> active.deep_wounds -> execute();
      }
    }
  }

  double cost() const
  {
    double c = base_t::cost();

    if ( p() -> specialization() == WARRIOR_PROTECTION && p() -> active.stance == STANCE_DEFENSE )
      c = 0;

    return c;
  }
};

// Victory Rush =============================================================

struct victory_rush_heal_t: public warrior_heal_t
{
  victory_rush_heal_t( warrior_t* p ):
    warrior_heal_t( "victory_rush_heal", p, p -> find_spell( 118779 ) )
  {
    pct_heal = data().effectN( 1 ).percent() * ( 1 + p -> glyphs.victory_rush -> effectN( 1 ).percent() );
    background = true;
  }
  resource_e current_resource() const { return RESOURCE_NONE; }
};

struct victory_rush_t: public warrior_attack_t
{
  victory_rush_heal_t* victory_rush_heal;

  victory_rush_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "victory_rush", p, p -> find_class_spell( "Victory Rush" ) ),
    victory_rush_heal( new victory_rush_heal_t( p ) )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;

    cooldown -> duration = timespan_t::from_seconds( 1000.0 );
  }

  void execute()
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      victory_rush_heal -> execute();

    p() -> buff.tier15_2pc_tank -> decrement();
  }

  bool ready()
  {
    if ( p() -> buff.tier15_2pc_tank -> check() )
      return true;

    return warrior_attack_t::ready();
  }
};

// Whirlwind ================================================================

struct whirlwind_off_hand_t: public warrior_attack_t
{
  whirlwind_off_hand_t( warrior_t* p ):
    warrior_attack_t( "whirlwind_oh", p, p -> find_spell( 44949 ) )
  {
    dual = true;
    aoe = -1;
    range = p -> spec.whirlwind -> effectN( 2 ).radius_max(); // 8 yard range.
    range += p -> glyphs.wind_and_thunder -> effectN( 1 ).base_value(); // Increased by the glyph.
    weapon_multiplier *= 1.0 + p -> spec.crazed_berserker -> effectN( 4 ).percent();
    weapon = &( p -> off_hand_weapon );
  }

  double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.raging_wind ->  up() )
      am *= 1.0 + p() -> buff.raging_wind -> data().effectN( 1 ).percent();

    return am;
  }
};

struct whirlwind_t: public warrior_attack_t
{
  whirlwind_off_hand_t* oh_attack;
  whirlwind_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "whirlwind_mh", p, p -> spec.whirlwind ),
    oh_attack( 0 )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_DEFENSE;
    aoe = -1;

    range = p -> spec.whirlwind -> effectN( 2 ).radius_max(); // 8 yard range.
    range += p -> glyphs.wind_and_thunder -> effectN( 1 ).base_value(); // Increased by the glyph.
    if ( p -> specialization() == WARRIOR_FURY )
    {
      if ( p -> off_hand_weapon.type != WEAPON_NONE )
      {
        oh_attack = new whirlwind_off_hand_t( p );
        add_child( oh_attack );
      }
      weapon_multiplier *= 1.0 + p -> spec.crazed_berserker -> effectN( 4 ).percent();
      base_costs[RESOURCE_RAGE] += p -> spec.crazed_berserker -> effectN( 3 ).resource( RESOURCE_RAGE );
    }
    else
    {
      weapon_multiplier *= 2;
    }

    weapon = &( p -> main_hand_weapon );
  }

  double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.raging_wind ->  up() )
      am *= 1.0 + p() -> buff.raging_wind -> data().effectN( 1 ).percent();

    return am;
  }

  void execute()
  {
    warrior_attack_t::execute();

    if ( oh_attack )
      oh_attack -> execute();

    p() -> buff.meat_cleaver -> trigger();
    if ( p() -> perk.enhanced_whirlwind -> ok() )
      p() -> buff.meat_cleaver -> trigger();
    p() -> buff.raging_wind -> expire();
  }

  bool ready()
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Wild Strike ==============================================================

struct wild_strike_t: public warrior_attack_t
{
  wild_strike_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "wild_strike", p, p -> spec.wild_strike )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_DEFENSE;

    base_costs[RESOURCE_RAGE] += p -> talents.furious_strikes -> effectN( 1 ).resource( RESOURCE_RAGE );
    weapon = &( player -> off_hand_weapon );
    min_gcd = data().gcd();
  }

  timespan_t gcd() const
  { // Wild strike has an unwieldy gcd of 0.75, which results in some lost gcd time due to an average humans ability to
    // Spam the key quickly enough for ability queue to catch it. If the lag tolerance is set high enough so that it functions normally, it 
    // leads to accidentally double-triggering the ability when the player does not want to.
    // After looking at a bunch of logs and testing it, the mean "GCD" tends to be around 0.76, with a range of 0.75-0.79.
    timespan_t t = timespan_t::from_seconds( rng().gauss( 0.76, 0.03 ) );
    if ( t < min_gcd )
      return min_gcd;
    else
      return t;
  }

  double cost() const
  {
    double c = warrior_attack_t::cost();

    if ( p() -> buff.bloodsurge -> up() )
      c = 0; // No spell data at the moment.

    return c;
  }

  void execute()
  {
    warrior_attack_t::execute();

    p() -> buff.bloodsurge -> decrement();
  }

  bool ready()
  {
    if ( p() -> off_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// ==========================================================================
// Warrior Spells
// ==========================================================================

struct warrior_spell_t: public warrior_action_t < spell_t >
{
  warrior_spell_t( const std::string& n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ):
    base_t( n, p, s )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }
};

// Avatar ===================================================================

struct avatar_t: public warrior_spell_t
{
  avatar_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "avatar", p, p -> talents.avatar )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
  }

  void execute()
  {
    warrior_spell_t::execute();

    p() -> buff.avatar -> trigger();
  }
};

// Battle Shout =============================================================

struct battle_shout_t: public warrior_spell_t
{
  battle_shout_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "battle_shout", p, p -> find_class_spell( "Battle Shout" ) )
  {
    parse_options( options_str );
    range = -1;
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    callbacks = false;
    ignore_false_positive = true;
  }

  void execute()
  {
    warrior_spell_t::execute();

    if ( !sim -> overrides.attack_power_multiplier )
      sim -> auras.attack_power_multiplier -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
  }
};

// Berserker Rage ===========================================================

struct berserker_rage_t: public warrior_spell_t
{
  berserker_rage_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "berserker_rage", p, p -> find_class_spell( "Berserker Rage" ) )
  {
    parse_options( options_str );
    range = -1; // Just in case anyone wants to use berserker rage + enraged speed glyph to get somewhere a little faster... I guess.
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
  }

  void execute()
  {
    warrior_spell_t::execute();

    p() -> buff.berserker_rage -> trigger();
    if ( p() -> specialization() != WARRIOR_ARMS )
      p() -> enrage();
  }
};

// Bloodbath ================================================================

struct bloodbath_t: public warrior_spell_t
{
  bloodbath_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "bloodbath", p, p -> talents.bloodbath )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
  }

  void execute()
  {
    warrior_spell_t::execute();

    p() -> buff.bloodbath -> trigger();
  }
};

// Commanding Shout =========================================================

struct commanding_shout_t: public warrior_spell_t
{
  commanding_shout_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "commanding_shout", p, p -> find_class_spell( "Commanding Shout" ) )
  {
    parse_options( options_str );
    range = -1;
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    callbacks = false;
    ignore_false_positive = true;
  }

  void execute()
  {
    warrior_spell_t::execute();

    if ( !sim -> overrides.stamina )
      sim -> auras.stamina -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
  }
};

// Deep Wounds ==============================================================

struct deep_wounds_t: public warrior_spell_t
{
  deep_wounds_t( warrior_t* p ):
    warrior_spell_t( "deep_wounds", p, p -> spec.deep_wounds -> effectN( 2 ).trigger() )
  {
    background = tick_may_crit = true;
    hasted_ticks = false;
  }
};

// Die By the Sword  ==============================================================

struct die_by_the_sword_t: public warrior_spell_t
{
  die_by_the_sword_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "die_by_the_sword", p, p -> spec.die_by_the_sword )
  {
    parse_options( options_str );
    range = -1;
    stancemask = STANCE_DEFENSE | STANCE_BATTLE;
  }

  void execute()
  {
    warrior_spell_t::execute();
    p() -> buff.die_by_the_sword -> trigger();
  }

  bool ready()
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_spell_t::ready();
  }
};

// Enhanced Rend ==============================================================

struct enhanced_rend_t: public warrior_spell_t
{
  enhanced_rend_t( warrior_t* p ):
    warrior_spell_t( "enhanced_rend", p, p -> find_spell( 174736 ) )
  {
    may_crit = background = true;
  }

  double target_armor( player_t* ) const
  {
    return 0.0;
  }
};

// Last Stand ===============================================================

struct last_stand_t: public warrior_spell_t
{
  last_stand_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "last_stand", p, p -> spec.last_stand )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> sets.set( SET_TANK, T14, B2 ) -> effectN( 1 ).time_value();
  }

  void execute()
  {
    warrior_spell_t::execute();
    p() -> buff.last_stand -> trigger();
  }
};

// Rallying Cry ===============================================================

struct rallying_cry_heal_t: public warrior_heal_t
{
  double percent;
  rallying_cry_heal_t( warrior_t* p ):
    warrior_heal_t( "glyph_of_rallying_cry", p, p -> glyphs.rallying_cry )
  {
    percent = data().effectN( 1 ).percent();
    background = true;
  }

  void execute()
  {
    base_dd_max *= percent; //Heals for 20% of original damage
    base_dd_min *= percent;

    warrior_heal_t::execute();
  }
};

struct rallying_cry_t: public warrior_spell_t
{
  rallying_cry_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "rallying_cry", p, p -> spec.rallying_cry )
  {
    parse_options( options_str );
    range = -1;
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
  }

  void execute()
  {
    warrior_spell_t::execute();
    p() -> buff.rallying_cry -> trigger();
  }
};

// Ravager ==============================================================

struct ravager_tick_t: public warrior_spell_t
{
  ravager_tick_t( warrior_t* p, const std::string& name ):
    warrior_spell_t( name, p, p -> find_spell( 156287 ) )
  {
    aoe = -1;
    dual = may_crit = true;
  }
};

struct ravager_t: public warrior_spell_t
{
  spell_t* ravager;
  ravager_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "ravager", p, p -> talents.ravager ),
    ravager( new ravager_tick_t( p, "ravager_tick" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    hasted_ticks = callbacks = false;
    dot_duration = timespan_t::from_seconds( data().effectN( 4 ).base_value() );
    attack_power_mod.direct = attack_power_mod.tick = 0;
    add_child( ravager );
  }

  void execute()
  {
    if ( p() -> specialization() == WARRIOR_PROTECTION )
      p() -> buff.ravager_protection -> trigger();
    else
      p() -> buff.ravager -> trigger();

    warrior_spell_t::execute();
  }

  void tick( dot_t*d )
  {
    ravager -> execute();
    warrior_spell_t::tick( d );
  }
};

// Recklessness =============================================================

struct recklessness_t: public warrior_spell_t
{
  double bonus_crit;
  recklessness_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "recklessness", p, p -> spec.recklessness ),
    bonus_crit( 0.0 )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE;
    bonus_crit = data().effectN( 1 ).percent();
    bonus_crit += p -> perk.improved_recklessness -> effectN( 1 ).percent();
    bonus_crit *= 1 + p -> glyphs.recklessness -> effectN( 1 ).percent();
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> sets.set( SET_MELEE, T14, B4 ) -> effectN( 1 ).time_value();
    callbacks = false;
  }

  void execute()
  {
    warrior_spell_t::execute();

    p() -> buff.recklessness -> trigger( 1, bonus_crit );
    if ( p() -> sets.has_set_bonus( WARRIOR_FURY, T17, B4 ) )
      p() -> buff.tier17_4pc_fury_driver -> trigger();
  }
};

// Shield Barrier ===========================================================

struct shield_barrier_t: public warrior_action_t < absorb_t >
{
  shield_barrier_t( warrior_t* p, const std::string& options_str ):
    base_t( "shield_barrier", p, p -> specialization() == WARRIOR_PROTECTION ?
    p -> find_spell( 112048 ) : p -> spec.shield_barrier )
  {
    parse_options( options_str );
    stancemask = STANCE_GLADIATOR | STANCE_DEFENSE;
    use_off_gcd = true;
    may_crit = false;
    range = -1;
    target = player;
    attack_power_mod.direct = 1.4; // No spell data.
  }

  double cost() const
  {
    return std::min( 60.0, std::max( p() -> resources.current[RESOURCE_RAGE], 20.0 ) );
  }

  void impact( action_state_t* s )
  {
    //1) Buff does not stack with itself.
    //2) Will overwrite existing buff if new one is bigger.
    double amount;

    amount = s -> result_amount;
    amount *= cost() / 20;
    if ( !p() -> buff.shield_barrier -> check() ||
         ( p() -> buff.shield_barrier -> check() && p() -> buff.shield_barrier -> current_value < amount ) )
    {
      p() -> buff.shield_barrier -> trigger( 1, amount );
      stats -> add_result( 0.0, amount, ABSORB, s -> result, s -> block_result, p() );
    }
  }

  bool ready()
  {
    if ( !p() -> has_shield_equipped() && p() -> specialization() == WARRIOR_PROTECTION )
      return false;

    return base_t::ready();
  }
};

// Shield Block =============================================================

struct shield_block_t: public warrior_spell_t
{
  cooldown_t* block_cd;
  shield_block_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "shield_block", p, p -> find_class_spell( "Shield Block" ) ),
    block_cd( NULL )
  {
    parse_options( options_str );
    stancemask = STANCE_DEFENSE | STANCE_BATTLE;
    cooldown -> duration = data().charge_cooldown();
    cooldown -> charges = data().charges();
    use_off_gcd = true;
    block_cd = p -> get_cooldown( "block_cd" );
    block_cd -> duration = timespan_t::from_seconds( 1.5 );
  }

  double cost() const
  {
    double c = warrior_spell_t::cost();

    c += p() -> sets.set( SET_TANK, T14, B4 ) -> effectN( 1 ).resource( RESOURCE_RAGE );

    return c;
  }

  void execute()
  {
    warrior_spell_t::execute();
    block_cd -> start();

    if ( p() -> buff.shield_block -> check() )
      p() -> buff.shield_block -> extend_duration( p(), p() -> buff.shield_block -> data().duration() );
    else
      p() -> buff.shield_block -> trigger();
  }

  bool ready()
  {
    if ( !p() -> has_shield_equipped() )
      return false;

    if ( !block_cd -> up() )
      return false;

    return warrior_spell_t::ready();
  }
};

// Shield Charge ============================================================

struct shield_charge_t: public warrior_spell_t
{
  double movement_speed_increase;
  cooldown_t* shield_charge_cd;
  shield_charge_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "shield_charge", p, p -> find_spell( 156321 ) ),
    movement_speed_increase( 5.0 ), shield_charge_cd( NULL )
  {
    parse_options( options_str );
    stancemask = STANCE_GLADIATOR;
    cooldown -> duration = data().charge_cooldown();
    cooldown -> charges = data().charges();
    movement_directionality = MOVEMENT_OMNI;
    use_off_gcd = true;

    shield_charge_cd = p -> get_cooldown( "shield_charge_cd" );
    shield_charge_cd -> duration = timespan_t::from_seconds( 1.5 );
  }

  void execute()
  {
    warrior_spell_t::execute();

    if ( p() -> current.distance_to_move > 0 )
    {
      p() -> buff.shield_charge_movement -> trigger( 1, movement_speed_increase, 1,
                                                     timespan_t::from_seconds( p() -> current.distance_to_move / ( p() -> base_movement_speed * 
                                                     ( 1 + p() -> passive_movement_modifier() + movement_speed_increase ) ) ) );
      p() -> current.moving_away = 0;
    }

    if ( p() -> buff.shield_charge -> check() )
      p() -> buff.shield_charge -> extend_duration( p(), p() -> buff.shield_charge -> data().duration() );
    else
      p() -> buff.shield_charge -> trigger();

    shield_charge_cd -> start();
  }

  bool ready()
  {
    if ( !shield_charge_cd -> up() )
      return false;

    if ( p() -> buff.heroic_leap_movement -> check() || p() -> buff.charge_movement -> check() || p() -> buff.intervene_movement -> check() )
      return false;
  
    if ( !p() -> has_shield_equipped() )
      return false;

    return warrior_spell_t::ready();
  }
};

// Shield Wall ==============================================================

struct shield_wall_t: public warrior_spell_t
{
  shield_wall_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "shield_wall", p, p -> find_class_spell( "Shield Wall" ) )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
    harmful = false;
    range = -1;
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> spec.bastion_of_defense -> effectN( 2 ).time_value();
    cooldown -> duration += p -> glyphs.shield_wall -> effectN( 1 ).time_value();
  }

  void execute()
  {
    warrior_spell_t::execute();

    double value = p() -> buff.shield_wall -> data().effectN( 1 ).percent();
    value += p() -> glyphs.shield_wall -> effectN( 2 ).percent();

    p() -> buff.shield_wall -> trigger( 1, value );
  }
};

// Spell Reflection  ==============================================================

struct spell_reflection_t: public warrior_spell_t
{
  spell_reflection_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "spell_reflection", p, p -> find_class_spell( "Spell Reflection" ) )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
  }
};

// Mass Spell Reflection  ==============================================================

struct mass_spell_reflection_t: public warrior_spell_t
{
  mass_spell_reflection_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "spell_reflection", p, p -> talents.mass_spell_reflection )
  {
    parse_options( options_str );
    stancemask = STANCE_DEFENSE | STANCE_GLADIATOR | STANCE_BATTLE;
  }
};

// The swap/damage taken options are intended to make it easier for players to simulate possible gains/losses from
// swapping stances while in combat, without having to create a bunch of messy actions for it.
// (Instead, we have a bunch of messy code!)

// Stance ==============================================================

struct stance_t: public warrior_spell_t
{
  warrior_stance switch_to_stance, starting_stance, original_switch;
  std::string stance_str;
  double swap;
  stance_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "stance", p ),
    switch_to_stance( STANCE_BATTLE ), stance_str( "" ), swap( 0 )
  {
    add_option( opt_string( "choose", stance_str ) );
    add_option( opt_float( "swap", swap ) );
    parse_options( options_str );
    range = -1;
    stancemask = STANCE_DEFENSE | STANCE_GLADIATOR | STANCE_BATTLE;

    if ( p -> specialization() != WARRIOR_PROTECTION )
      starting_stance = STANCE_BATTLE;
    else if ( p -> gladiator )
      starting_stance = STANCE_GLADIATOR;
    else
      starting_stance = STANCE_DEFENSE;

    if ( !stance_str.empty() )
    {
      if ( stance_str == "battle" )
      {
        switch_to_stance = STANCE_BATTLE;
        original_switch = switch_to_stance;
      }
      else if ( stance_str == "def" || stance_str == "defensive" )
      {
        switch_to_stance = STANCE_DEFENSE;
        original_switch = switch_to_stance;
      }
      else if ( stance_str == "glad" || stance_str == "gladiator" )
      {
        switch_to_stance = STANCE_GLADIATOR;
        original_switch = switch_to_stance;
      }
    }

    if ( swap == 0 )
      cooldown = p -> cooldown.stance_swap;
    else
      cooldown -> duration = ( timespan_t::from_seconds( swap ) );

    ignore_false_positive = use_off_gcd = true;
    callbacks = harmful = false;
  }

  void execute()
  {
    switch ( p() -> active.stance )
    {
    case STANCE_BATTLE: p() -> buff.battle_stance -> expire(); break;
    case STANCE_DEFENSE:
    {
      p() -> buff.defensive_stance -> expire();
      p() -> recalculate_resource_max( RESOURCE_HEALTH );
      break;
    }
    case STANCE_GLADIATOR: p() -> buff.gladiator_stance -> expire(); break;
    }

    switch ( switch_to_stance )
    {
    case STANCE_BATTLE: p() -> buff.battle_stance -> trigger(); break;
    case STANCE_DEFENSE:
    {
      p() -> buff.defensive_stance -> trigger();
      p() -> recalculate_resource_max( RESOURCE_HEALTH ); // Force stamina modifier, otherwise it doesn't apply until stat_loss is called
      break;
    }
    case STANCE_GLADIATOR: p() -> buff.gladiator_stance -> trigger(); break;
    }
    if ( p() -> in_combat && !p() -> player_override_stance_dance && starting_stance != switch_to_stance )
    {
      p() -> stance_dance = true;
    }
    p() -> active.stance = switch_to_stance;
    p() -> cooldown.stance_swap -> start();
    p() -> cooldown.stance_swap -> adjust( -1 * p() -> cooldown.stance_swap -> duration * ( 1.0 - p() -> cache.attack_haste() ) ); // Yeah.... it's hasted by headlong rush.

    if ( swap > 0 )
    {
      timespan_t stance_gcd;
      stance_gcd = p() -> cooldown.stance_swap -> remains();
      if ( swap >= ( stance_gcd * 2 ).total_seconds() )
      {
        cooldown -> start();
        if ( p() -> active.stance != starting_stance )
          switch_to_stance = starting_stance;
        else
        {
          switch_to_stance = original_switch;
          cooldown -> adjust( -1 * stance_gcd );
        }
      }
      else if ( cooldown -> duration > stance_gcd ) // Don't worry about starting the cooldown if the limitation will be the stance global.
      {
        cooldown -> start();
      }
    }
  }

  bool ready()
  {
    if ( p() -> in_combat && p() -> talents.gladiators_resolve -> ok() )
      return false;
    if ( p() -> cooldown.stance_swap -> down() || cooldown -> down() || ( swap == 0 && p() -> active.stance == switch_to_stance ) )
      return false;

    return warrior_spell_t::ready();
  }
};

// Sweeping Strikes =========================================================

struct sweeping_strikes_t: public warrior_spell_t
{
  sweeping_strikes_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "sweeping_strikes", p, p -> spec.sweeping_strikes )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE;
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> perk.enhanced_sweeping_strikes -> effectN( 2 ).time_value();
    ignore_false_positive = true;
  }

  void execute()
  {
    warrior_spell_t::execute();
    p() -> buff.sweeping_strikes -> trigger();
  }
};

// Taunt =======================================================================

struct taunt_t: public warrior_spell_t
{
  taunt_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "taunt", p, p -> find_class_spell( "Taunt" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
    ignore_false_positive = true;
    stancemask = STANCE_DEFENSE | STANCE_GLADIATOR;
  }

  void impact( action_state_t* s )
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    warrior_spell_t::impact( s );
  }
};

// Vigilance =======================================================================

struct vigilance_t: public warrior_spell_t
{
  vigilance_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "vigilance", p, p -> talents.vigilance )
  {
    parse_options( options_str );
    stancemask = STANCE_BATTLE | STANCE_GLADIATOR | STANCE_DEFENSE;
  }
};

} // UNNAMED NAMESPACE

// warrior_t::create_action  ================================================

action_t* warrior_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "auto_attack"          ) return new auto_attack_t          ( this, options_str );
  if ( name == "avatar"               ) return new avatar_t               ( this, options_str );
  if ( name == "battle_shout"         ) return new battle_shout_t         ( this, options_str );
  if ( name == "berserker_rage"       ) return new berserker_rage_t       ( this, options_str );
  if ( name == "bladestorm"           ) return new bladestorm_t           ( this, options_str );
  if ( name == "bloodbath"            ) return new bloodbath_t            ( this, options_str );
  if ( name == "bloodthirst"          ) return new bloodthirst_t          ( this, options_str );
  if ( name == "charge"               ) return new charge_t               ( this, options_str );
  if ( name == "colossus_smash"       ) return new colossus_smash_t       ( this, options_str );
  if ( name == "commanding_shout"     ) return new commanding_shout_t     ( this, options_str );
  if ( name == "demoralizing_shout"   ) return new demoralizing_shout     ( this, options_str );
  if ( name == "devastate"            ) return new devastate_t            ( this, options_str );
  if ( name == "die_by_the_sword"     ) return new die_by_the_sword_t     ( this, options_str );
  if ( name == "dragon_roar"          ) return new dragon_roar_t          ( this, options_str );
  if ( name == "enraged_regeneration" ) return new enraged_regeneration_t ( this, options_str );
  if ( name == "execute"              ) return new execute_t              ( this, options_str );
  if ( name == "hamstring"            ) return new hamstring_t            ( this, options_str );
  if ( name == "heroic_charge"        ) return new heroic_charge_t        ( this, options_str );
  if ( name == "heroic_leap"          ) return new heroic_leap_t          ( this, options_str );
  if ( name == "heroic_strike"        ) return new heroic_strike_t        ( this, options_str );
  if ( name == "heroic_throw"         ) return new heroic_throw_t         ( this, options_str );
  if ( name == "impending_victory"    ) return new impending_victory_t    ( this, options_str );
  if ( name == "intervene" || name == "safeguard" ) return new intervene_t ( this, options_str );
  if ( name == "last_stand"           ) return new last_stand_t           ( this, options_str );
  if ( name == "mortal_strike"        ) return new mortal_strike_t        ( this, options_str );
  if ( name == "pummel"               ) return new pummel_t               ( this, options_str );
  if ( name == "raging_blow"          ) return new raging_blow_t          ( this, options_str );
  if ( name == "rallying_cry"         ) return new rallying_cry_t         ( this, options_str );
  if ( name == "ravager"              ) return new ravager_t              ( this, options_str );
  if ( name == "recklessness"         ) return new recklessness_t         ( this, options_str );
  if ( name == "rend"                 ) return new rend_t                 ( this, options_str );
  if ( name == "revenge"              ) return new revenge_t              ( this, options_str );
  if ( name == "siegebreaker"         ) return new siegebreaker_t         ( this, options_str );
  if ( name == "shield_barrier"       ) return new shield_barrier_t       ( this, options_str );
  if ( name == "shield_block"         ) return new shield_block_t         ( this, options_str );
  if ( name == "shield_charge"        ) return new shield_charge_t        ( this, options_str );
  if ( name == "shield_slam"          ) return new shield_slam_t          ( this, options_str );
  if ( name == "shield_wall"          ) return new shield_wall_t          ( this, options_str );
  if ( name == "shockwave"            ) return new shockwave_t            ( this, options_str );
  if ( name == "slam"                 ) return new slam_t                 ( this, options_str );
  if ( name == "spell_reflection" || name == "mass_spell_reflection" )
  {
    if ( talents.mass_spell_reflection-> ok() )
      return new mass_spell_reflection_t( this, options_str );
    else
      return new spell_reflection_t( this, options_str );
  }
  if ( name == "stance"               ) return new stance_t               ( this, options_str );
  if ( name == "storm_bolt"           ) return new storm_bolt_t           ( this, options_str );
  if ( name == "sweeping_strikes"     ) return new sweeping_strikes_t     ( this, options_str );
  if ( name == "taunt"                ) return new taunt_t                ( this, options_str );
  if ( name == "thunder_clap"         ) return new thunder_clap_t         ( this, options_str );
  if ( name == "victory_rush"         ) return new victory_rush_t         ( this, options_str );
  if ( name == "vigilance"            ) return new vigilance_t            ( this, options_str );
  if ( name == "whirlwind"            ) return new whirlwind_t            ( this, options_str );
  if ( name == "wild_strike"          ) return new wild_strike_t          ( this, options_str );

  return player_t::create_action( name, options_str );
}

// warrior_t::init_spells ===================================================

void warrior_t::init_spells()
{
  player_t::init_spells();

  // Mastery
  mastery.critical_block        = find_mastery_spell( WARRIOR_PROTECTION );
  mastery.weapons_master        = find_mastery_spell( WARRIOR_ARMS );
  mastery.unshackled_fury       = find_mastery_spell( WARRIOR_FURY );

  // Spec Passives
  spec.bastion_of_defense       = find_specialization_spell( "Bastion of Defense" );
  spec.bladed_armor             = find_specialization_spell( "Bladed Armor" );
  spec.blood_craze              = find_specialization_spell( "Blood Craze" );
  spec.bloodsurge               = find_specialization_spell( "Bloodsurge" );
  spec.bloodthirst              = find_specialization_spell( "Bloodthirst" );
  spec.colossus_smash           = find_specialization_spell( "Colossus Smash" );
  spec.crazed_berserker         = find_specialization_spell( "Crazed Berserker" );
  spec.cruelty                  = find_specialization_spell( "Cruelty" );
  spec.deep_wounds              = find_specialization_spell( "Deep Wounds" );
  spec.demoralizing_shout       = find_specialization_spell( "Demoralizing Shout" );
  spec.devastate                = find_specialization_spell( "Devastate" );
  spec.die_by_the_sword         = find_specialization_spell( "Die By the Sword" );
  spec.enrage                   = find_specialization_spell( "Enrage" );
  spec.execute                  = find_specialization_spell( "Execute" );
  spec.inspiring_presence       = find_specialization_spell( "Inspiring Presence" );
  spec.last_stand               = find_specialization_spell( "Last Stand" );
  spec.meat_cleaver             = find_specialization_spell( "Meat Cleaver" );
  spec.mocking_banner           = find_specialization_spell( "Mocking Banner" );
  spec.mortal_strike            = find_specialization_spell( "Mortal Strike" );
  spec.piercing_howl            = find_specialization_spell( "Piercing Howl" );
  spec.protection               = find_specialization_spell( "Protection" );
  spec.raging_blow              = find_specialization_spell( "Raging Blow" );
  spec.rallying_cry             = find_specialization_spell( "Rallying Cry" );
  spec.recklessness             = find_specialization_spell( "Recklessness" );
  spec.rend                     = find_specialization_spell( "Rend" );
  spec.resolve                  = find_specialization_spell( "Resolve" );
  spec.revenge                  = find_specialization_spell( "Revenge" );
  spec.riposte                  = find_specialization_spell( "Riposte" );
  spec.seasoned_soldier         = find_specialization_spell( "Seasoned Soldier" );
  spec.shield_barrier           = find_specialization_spell( "Shield Barrier" );
  spec.shield_block             = find_specialization_spell( "Shield Block" );
  spec.shield_mastery           = find_specialization_spell( "Shield Mastery" );
  spec.shield_slam              = find_specialization_spell( "Shield Slam" );
  spec.shield_wall              = find_specialization_spell( "Shield Wall" );
  spec.singleminded_fury        = find_specialization_spell( "Single-Minded Fury" );
  spec.sweeping_strikes         = find_specialization_spell( "Sweeping Strikes" );
  spec.sword_and_board          = find_specialization_spell( "Sword and Board" );
  spec.titans_grip              = find_specialization_spell( "Titan's Grip" );
  spec.thunder_clap             = find_specialization_spell( "Thunder Clap" );
  spec.ultimatum                = find_specialization_spell( "Ultimatum" );
  spec.unwavering_sentinel      = find_specialization_spell( "Unwavering Sentinel" );
  spec.weapon_mastery           = find_specialization_spell( "Weapon Mastery" );
  spec.whirlwind                = find_specialization_spell( "Whirlwind" );
  spec.wild_strike              = find_specialization_spell( "Wild Strike" );

  // Talents
  talents.juggernaut            = find_talent_spell( "Juggernaut" );
  talents.double_time           = find_talent_spell( "Double Time" );
  talents.warbringer            = find_talent_spell( "Warbringer" );

  talents.enraged_regeneration  = find_talent_spell( "Enraged Regeneration" );
  talents.second_wind           = find_talent_spell( "Second Wind" );
  talents.impending_victory     = find_talent_spell( "Impending Victory" );

  talents.heavy_repercussions   = find_talent_spell( "Heavy Repercussions" );
  talents.unyielding_strikes    = find_talent_spell( "Unyielding Strikes" );
  talents.sudden_death          = find_talent_spell( "Sudden Death" );
  talents.taste_for_blood       = find_talent_spell( "Taste for Blood" );
  talents.unquenchable_thirst   = find_talent_spell( "Unquenchable Thirst" );
  talents.slam                  = find_talent_spell( "Slam" );
  talents.furious_strikes       = find_talent_spell( "Furious Strikes" );

  talents.storm_bolt            = find_talent_spell( "Storm Bolt" );
  talents.shockwave             = find_talent_spell( "Shockwave" );
  talents.dragon_roar           = find_talent_spell( "Dragon Roar" );

  talents.mass_spell_reflection = find_talent_spell( "Mass Spell Reflection" );
  talents.safeguard             = find_talent_spell( "Safeguard" );
  talents.vigilance             = find_talent_spell( "Vigilance" );

  talents.avatar                = find_talent_spell( "Avatar" );
  talents.bloodbath             = find_talent_spell( "Bloodbath" );
  talents.bladestorm            = find_talent_spell( "Bladestorm" );

  talents.anger_management      = find_talent_spell( "Anger Management" );
  talents.ravager               = find_talent_spell( "Ravager" );
  talents.siegebreaker          = find_talent_spell( "Siegebreaker" );
  talents.gladiators_resolve    = find_talent_spell( "Gladiator's Resolve" );

  //Perks
  perk.improved_heroic_leap          = find_perk_spell( "Improved Heroic Leap" );
  perk.improved_recklessness         = find_perk_spell( "Improved Recklessness" );

  perk.enhanced_sweeping_strikes     = find_perk_spell( "Enhanced Sweeping Strikes" );
  perk.improved_die_by_the_sword     = find_perk_spell( "Improved Die by The Sword" );
  perk.enhanced_rend                 = find_perk_spell( "Enhanced Rend" );

  perk.enhanced_whirlwind            = find_perk_spell( "Enhanced Whirlwind" );
  perk.empowered_execute             = find_perk_spell( "Empowered Execute" );

  perk.improved_heroic_throw         = find_perk_spell( "Improved Heroic Throw" );
  perk.improved_defensive_stance     = find_perk_spell( "Improved Defensive Stance" );
  perk.improved_block                = find_perk_spell( "Improved Block" );

  // Glyphs
  glyphs.bloodthirst            = find_glyph_spell( "Glyph of Bloodthirst" );
  glyphs.bull_rush              = find_glyph_spell( "Glyph of Bull Rush" );
  glyphs.cleave                 = find_glyph_spell( "Glyph of Cleave" );
  glyphs.death_from_above       = find_glyph_spell( "Glyph of Death From Above" );
  glyphs.drawn_sword            = find_glyph_spell( "Glyph of the Drawn Sword" );
  glyphs.enraged_speed          = find_glyph_spell( "Glyph of Enraged Speed" );
  glyphs.hamstring              = find_glyph_spell( "Glyph of Hamstring" );
  glyphs.heroic_leap            = find_glyph_spell( "Glyph of Heroic Leap" );
  glyphs.long_charge            = find_glyph_spell( "Glyph of Long Charge" );
  glyphs.raging_blow            = find_glyph_spell( "Glyph of Raging Blow" );
  glyphs.raging_wind            = find_glyph_spell( "Glyph of Raging Wind" );
  glyphs.rallying_cry           = find_glyph_spell( "Glyph of Rallying Cry" );
  glyphs.recklessness           = find_glyph_spell( "Glyph of Recklessness" );
  glyphs.resonating_power       = find_glyph_spell( "Glyph of Resonating Power" );
  glyphs.rude_interruption      = find_glyph_spell( "Glyph of Rude Interruption" );
  glyphs.shield_wall            = find_glyph_spell( "Glyph of Shield Wall" );
  glyphs.sweeping_strikes       = find_glyph_spell( "Glyph of Sweeping Strikes" );
  glyphs.unending_rage          = find_glyph_spell( "Glyph of Unending Rage" );
  glyphs.victory_rush           = find_glyph_spell( "Glyph of Victory Rush" );
  glyphs.wind_and_thunder       = find_glyph_spell( "Glyph of Wind and Thunder" ); //So roleplay.

  // Generic spells
  spell.charge                  = find_class_spell( "Charge" );
  spell.intervene               = find_class_spell( "Intervene" );
  spell.headlong_rush           = find_spell( 158836 ); // Stop changing this, stupid. find_spell( "headlong rush" ) will never work.
  spell.heroic_leap             = find_class_spell( "Heroic Leap" );

  // Active spells
  if ( spec.blood_craze -> ok() ) active.blood_craze = new blood_craze_t( this );
  if ( talents.bloodbath -> ok() ) active.bloodbath_dot = new bloodbath_dot_t( this );
  if ( spec.deep_wounds -> ok() ) active.deep_wounds = new deep_wounds_t( this );
  if ( perk.enhanced_rend -> ok() ) active.enhanced_rend = new enhanced_rend_t( this );
  if ( glyphs.rallying_cry -> ok() ) active.rallying_cry_heal = new rallying_cry_heal_t( this );
  if ( talents.second_wind -> ok() ) active.second_wind = new second_wind_t( this );
  if ( sets.has_set_bonus( SET_TANK, T16, B2 ) ) active.t16_2pc = new tier16_2pc_tank_heal_t( this );
  if ( sets.has_set_bonus( WARRIOR_PROTECTION, T17, B4 ) )  spell.t17_prot_2p = find_spell( 169688 );

  if ( !talents.gladiators_resolve -> ok() )
    gladiator = false;
}

// warrior_t::init_base =====================================================

void warrior_t::init_base_stats()
{
  player_t::init_base_stats();

  resources.base[RESOURCE_RAGE] = 100;

  if ( glyphs.unending_rage -> ok() )
    resources.base[RESOURCE_RAGE] += glyphs.unending_rage -> effectN( 1 ).resource( RESOURCE_RAGE );

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility = 0.0;

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base.dodge += spec.bastion_of_defense -> effectN( 3 ).percent();

  base_gcd = timespan_t::from_seconds( 1.5 );

  // initialize resolve for prot
  if ( specialization() == WARRIOR_PROTECTION )
    resolve_manager.init();
}

//Pre-combat Action Priority List============================================

void warrior_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( level > 90 )
    {
      if ( primary_role() == ROLE_ATTACK || gladiator )
        flask_action += "greater_draenic_strength_flask";
      else if ( primary_role() == ROLE_TANK )
        flask_action += "greater_draenic_stamina_flask";
    }
    else
    {
      if ( primary_role() == ROLE_ATTACK )
        flask_action += "winters_bite";
      else if ( primary_role() == ROLE_TANK )
        flask_action += "earth";
    }
    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food )
  {
    std::string food_action = "food,type=";
    if ( specialization() == WARRIOR_FURY )
    {
      if ( level > 90 )
        food_action += "pickled_eel";
      else
        food_action += "black_pepper_ribs_and_shrimp";
    }
    else if ( specialization() == WARRIOR_ARMS )
    {
      if ( level > 90 )
        food_action += "sleeper_sushi";
      else
        food_action += "black_pepper_ribs_and_shrimp";
    }
    else
    {
      if ( level > 90 )
        food_action += "pickled_eel";
      else
        food_action += "chun_tian_spring_rolls";
    }
    precombat -> add_action( food_action );
  }

  if ( specialization() == WARRIOR_ARMS )
  {
    talent_overrides_str += "bladestorm,if=raid_event.adds.count>=1|enemies>1/"
      "dragon_roar,if=raid_event.adds.count>=1|enemies>1/"
      "taste_for_blood,if=raid_event.adds.count>=1|enemies>1/"
      "ravager,if=raid_event.adds.cooldown>=60&raid_event.adds.exists";
    precombat -> add_action( "stance,choose=battle" );
    precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done.\n"
      "# Generic on-use trinket line if needed when swapping trinkets out. \n"
      "# actions+=/use_item,slot=trinket1,if=active_enemies=1&(buff.bloodbath.up|(!talent.bloodbath.enabled&debuff.colossus_smash.up))|(active_enemies>=2&(prev_gcd.ravager|(!talent.ravager.enabled&!cooldown.bladestorm.remains&dot.rend.ticking)))" );
  }
  else if ( specialization() == WARRIOR_FURY )
  {
    talent_overrides_str += "bladestorm,if=raid_event.adds.count>=1|enemies>1/"
      "dragon_roar,if=raid_event.adds.count>=1|enemies>1/"
      "ravager,if=raid_event.adds.cooldown>=60&raid_event.adds.exists";
    precombat -> add_action( "stance,choose=battle" );
    precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done.\n"
      "# Generic on-use trinket line if needed when swapping trinkets out. \n"
      "#actions+=/use_item,slot=trinket1,if=active_enemies=1&(buff.bloodbath.up|(!talent.bloodbath.enabled&(buff.avatar.up|!talent.avatar.enabled)))|(active_enemies>=2&buff.ravager.up)" );
  }
  else if ( gladiator )
  {
    talent_overrides_str += "bladestorm,if=raid_event.adds.count>=1|enemies>1/"
      "dragon_roar,if=raid_event.adds.count>=1|enemies>1";
    precombat -> add_action( "stance,choose=gladiator" );
    precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done.\n"
      "# Generic on-use trinket line if needed when swapping trinkets out. \n"
      "#actions+=/use_item,slot=trinket1,if=buff.bloodbath.up|buff.avatar.up|buff.shield_charge.up|target.time_to_die<10" );
  }
  else
  {
    precombat -> add_action( "stance,choose=defensive" );
    precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done.\n"
      "# Generic on-use trinket line if needed when swapping trinkets out. \n"
      "#actions+=/use_item,slot=trinket1,if=buff.bloodbath.up|buff.avatar.up|target.time_to_die<10" );
    precombat -> add_action( this, "Shield Wall" );
  }

  //Pre-pot
  if ( sim -> allow_potions )
  {
    if ( level > 90 )
    {
      if ( specialization() != WARRIOR_PROTECTION )
        precombat -> add_action( "potion,name=draenic_strength" );
      else
        precombat -> add_action( "potion,name=draenic_armor" );
    }
    else if ( level >= 80 )
    {
      if ( primary_role() == ROLE_ATTACK )
        precombat -> add_action( "potion,name=mogu_power" );
      else if ( primary_role() == ROLE_TANK )
        precombat -> add_action( "potion,name=mountains" );
    }
  }
}

// Fury Warrior Action Priority List ========================================

void warrior_t::apl_fury()
{
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default" );
  action_priority_list_t* movement            = get_action_priority_list( "movement" );
  action_priority_list_t* single_target       = get_action_priority_list( "single_target" );
  action_priority_list_t* two_targets         = get_action_priority_list( "two_targets" );
  action_priority_list_t* three_targets       = get_action_priority_list( "three_targets" );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe" );
  action_priority_list_t* bladestorm          = get_action_priority_list( "bladestorm" );

  default_list -> add_action( this, "Charge", "if=debuff.charge.down" );
  default_list -> add_action( "auto_attack" );
  default_list -> add_action( "call_action_list,name=movement,if=movement.distance>5", "This is mostly to prevent cooldowns from being accidentally used during movement." );
  default_list -> add_action( this, "Berserker Rage", "if=buff.enrage.down|(prev_gcd.bloodthirst&buff.raging_blow.stack<2)" );
  default_list -> add_action( this, "Heroic Leap", "if=(raid_event.movement.distance>25&raid_event.movement.in>45)|!raid_event.movement.exists" );

  size_t num_items = items.size();
  for ( size_t i = 0; i < num_items; i++ )
  {
    if ( items[i].name_str == "scabbard_of_kyanos" )
      default_list -> add_action( "use_item,name=" + items[i].name_str + ",if=(active_enemies>1|!raid_event.adds.exists)&((talent.bladestorm.enabled&cooldown.bladestorm.remains=0)|buff.avatar.up|buff.bloodbath.up|target.time_to_die<25)" );
    else if ( items[i].name_str == "vial_of_convulsive_shadows" )
      default_list -> add_action( "use_item,name=" + items[i].name_str + ",if=(active_enemies>1|!raid_event.adds.exists)&((talent.bladestorm.enabled&cooldown.bladestorm.remains=0)|buff.recklessness.up|target.time_to_die<25|!talent.anger_management.enabled)" );
    else if ( items[i].name_str != "nitro_boosts" && items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      default_list -> add_action( "use_item,name=" + items[i].name_str + ",if=(active_enemies>1|!raid_event.adds.exists)&((talent.bladestorm.enabled&cooldown.bladestorm.remains=0)|buff.recklessness.up|buff.avatar.up|buff.bloodbath.up|target.time_to_die<25)" );
  }

  if ( sim -> allow_potions )
  {
    if ( level > 90 )
      default_list -> add_action( "potion,name=draenic_strength,if=(target.health.pct<20&buff.recklessness.up)|target.time_to_die<=25" );
    else if ( level >= 80 )
      default_list -> add_action( "potion,name=mogu_power,if=(target.health.pct<20&buff.recklessness.up)|target.time_to_die<=25" );
  }

  default_list -> add_action( "call_action_list,name=single_target,if=(raid_event.adds.cooldown<60&raid_event.adds.count>2&active_enemies=1)|raid_event.movement.cooldown<5", "Skip cooldown usage if we can line them up with bladestorm on a large set of adds, or if movement is coming soon." );
  default_list -> add_action( this, "Recklessness", "if=(((target.time_to_die>190|target.health.pct<20)&(buff.bloodbath.up|!talent.bloodbath.enabled))|target.time_to_die<=12|talent.anger_management.enabled)&((talent.bladestorm.enabled&(!raid_event.adds.exists|enemies=1))|!talent.bladestorm.enabled)",
                              "This incredibly long line (Due to differing talent choices) says 'Use recklessness on cooldown, unless the boss will die before the ability is usable again, and then use it with execute.'" );
  default_list -> add_talent( this, "Avatar", "if=buff.recklessness.up|cooldown.recklessness.remains>60|target.time_to_die<30" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
      default_list -> add_action( racial_actions[i] + ",if=rage<rage.max-40" );
    else
      default_list -> add_action( racial_actions[i] + ",if=buff.bloodbath.up|!talent.bloodbath.enabled|buff.recklessness.up" );
  }

  default_list -> add_action( "call_action_list,name=single_target,if=active_enemies=1" );
  default_list -> add_action( "call_action_list,name=two_targets,if=active_enemies=2" );
  default_list -> add_action( "call_action_list,name=three_targets,if=active_enemies=3" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>3" );

  movement -> add_action( this, "Heroic Leap" );
  movement -> add_action( this, "Charge", "cycle_targets=1,if=debuff.charge.down" );
  movement -> add_action( this, "Charge", "", "If possible, charge a target that will give rage. Otherwise, just charge to get back in range." );
  for ( size_t i = 0; i < num_items; i++ )
  {
    if ( items[i].parsed.encoded_addon == "nitro_boosts" )
      movement -> add_action( "use_item,name=" + items[i].name_str + ",if=movement.distance>90" );
  }
  movement -> add_talent( this, "Storm Bolt", "", "May as well throw storm bolt if we can." );
  movement -> add_action( this, "Heroic Throw" );

  single_target -> add_talent( this, "Bloodbath" );
  single_target -> add_action( this, "Recklessness", "if=target.health.pct<20&raid_event.adds.exists" );
  single_target -> add_action( this, "Wild Strike", "if=(rage>rage.max-20)&target.health.pct>20" );
  single_target -> add_action( this, "Bloodthirst", "if=(!talent.unquenchable_thirst.enabled&(rage<rage.max-40))|buff.enrage.down|buff.raging_blow.stack<2" );
  single_target -> add_talent( this, "Ravager", "if=buff.bloodbath.up|(!talent.bloodbath.enabled&(!raid_event.adds.exists|raid_event.adds.in>60|target.time_to_die<40))" );
  single_target -> add_talent( this, "Siegebreaker" );
  single_target -> add_action( this, "Execute", "if=buff.sudden_death.react" );
  single_target -> add_talent( this, "Storm Bolt" );
  single_target -> add_action( this, "Wild Strike", "if=buff.bloodsurge.up" );
  single_target -> add_action( this, "Execute", "if=buff.enrage.up|target.time_to_die<12" );
  single_target -> add_talent( this, "Dragon Roar", "if=buff.bloodbath.up|!talent.bloodbath.enabled" );
  single_target -> add_action( this, "Raging Blow" );
  single_target -> add_action( "wait,sec=cooldown.bloodthirst.remains,if=cooldown.bloodthirst.remains<0.5&rage<50" );
  single_target -> add_action( this, "Wild Strike", "if=buff.enrage.up&target.health.pct>20" );
  single_target -> add_talent( this, "Bladestorm", "if=!raid_event.adds.exists" );
  single_target -> add_talent( this, "Shockwave", "if=!talent.unquenchable_thirst.enabled" );
  single_target -> add_talent( this, "Impending Victory", "if=!talent.unquenchable_thirst.enabled&target.health.pct>20" );
  single_target -> add_action( this, "Bloodthirst" );

  two_targets -> add_talent( this, "Bloodbath" );
  two_targets -> add_talent( this, "Ravager", "if=buff.bloodbath.up|!talent.bloodbath.enabled" );
  two_targets -> add_talent( this, "Dragon Roar", "if=buff.bloodbath.up|!talent.bloodbath.enabled" );
  two_targets -> add_action( "call_action_list,name=bladestorm" );
  two_targets -> add_action( this, "Bloodthirst", "if=buff.enrage.down|rage<40|buff.raging_blow.down" );
  two_targets -> add_talent( this, "Siegebreaker" );
  two_targets -> add_action( this, "Execute", "cycle_targets=1" );
  two_targets -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.up|target.health.pct<20" );
  two_targets -> add_action( this, "Whirlwind", "if=!buff.meat_cleaver.up&target.health.pct>20" );
  two_targets -> add_action( this, "Wild Strike", "if=buff.bloodsurge.up" );
  two_targets -> add_action( this, "Bloodthirst" );
  two_targets -> add_action( this, "Whirlwind" );

  three_targets -> add_talent( this, "Bloodbath" );
  three_targets -> add_talent( this, "Ravager", "if=buff.bloodbath.up|!talent.bloodbath.enabled" );
  three_targets -> add_action( "call_action_list,name=bladestorm" );
  three_targets -> add_action( this, "Bloodthirst", "if=buff.enrage.down|rage<50|buff.raging_blow.down" );
  three_targets -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.stack>=2" );
  three_targets -> add_talent( this, "Siegebreaker" );
  three_targets -> add_action( this, "Execute", "cycle_targets=1" );
  three_targets -> add_talent( this, "Dragon Roar", "if=buff.bloodbath.up|!talent.bloodbath.enabled" );
  three_targets -> add_action( this, "Whirlwind", "if=target.health.pct>20" );
  three_targets -> add_action( this, "Bloodthirst" );
  three_targets -> add_action( this, "Wild Strike", "if=buff.bloodsurge.up" );
  three_targets -> add_action( this, "Raging Blow" );

  aoe -> add_talent( this, "Bloodbath" );
  aoe -> add_talent( this, "Ravager", "if=buff.bloodbath.up|!talent.bloodbath.enabled" );
  aoe -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.stack>=3&buff.enrage.up" );
  aoe -> add_action( this, "Bloodthirst", "if=buff.enrage.down|rage<50|buff.raging_blow.down" );
  aoe -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.stack>=3" );
  aoe -> add_action( "call_action_list,name=bladestorm" );
  aoe -> add_action( this, "Whirlwind" );
  aoe -> add_talent( this, "Siegebreaker" );
  aoe -> add_action( this, "Execute", "if=buff.sudden_death.react" );
  aoe -> add_talent( this, "Dragon Roar", "if=buff.bloodbath.up|!talent.bloodbath.enabled" );
  aoe -> add_action( this, "Bloodthirst" );
  aoe -> add_action( this, "Wild Strike", "if=buff.bloodsurge.up" );

  bladestorm -> add_action( this, "Recklessness", "sync=bladestorm,if=buff.enrage.remains>6&((talent.anger_management.enabled&raid_event.adds.in>45)|(!talent.anger_management.enabled&raid_event.adds.in>60)|!raid_event.adds.exists|active_enemies>desired_targets)", "oh god why" );
  bladestorm -> add_talent( this, "Bladestorm", "if=buff.enrage.remains>6&((talent.anger_management.enabled&raid_event.adds.in>45)|(!talent.anger_management.enabled&raid_event.adds.in>60)|!raid_event.adds.exists|active_enemies>desired_targets)" );
}

// Arms Warrior Action Priority List ========================================

void warrior_t::apl_arms()
{
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default" );
  action_priority_list_t* movement            = get_action_priority_list( "movement" );
  action_priority_list_t* single_target       = get_action_priority_list( "single" );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe" );

  default_list -> add_action( this, "Charge", "if=debuff.charge.down" );
  default_list -> add_action( "auto_attack" );
  default_list -> add_action( "call_action_list,name=movement,if=movement.distance>5", "This is mostly to prevent cooldowns from being accidentally used during movement." );

  size_t num_items = items.size();
  for ( size_t i = 0; i < num_items; i++ )
  {
    if ( items[i].name_str == "scabbard_of_kyanos" )
      default_list -> add_action( "use_item,name=" + items[i].name_str + ",if=debuff.colossus_smash.up" );
    else if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) && items[i].parsed.encoded_addon != "nitro_boosts" )
      default_list -> add_action( "use_item,name=" + items[i].name_str + ",if=(buff.bloodbath.up|(!talent.bloodbath.enabled&debuff.colossus_smash.up))" );
  }

  if ( sim -> allow_potions )
  {
    if ( level > 90 )
      default_list -> add_action( "potion,name=draenic_strength,if=(target.health.pct<20&buff.recklessness.up)|target.time_to_die<25" );
    else if ( level >= 80 )
      default_list -> add_action( "potion,name=mogu_power,if=(target.health.pct<20&buff.recklessness.up)|target.time_to_die<25" );
  }

  default_list -> add_action( this, "Recklessness", "if=(((target.time_to_die>190|target.health.pct<20)&(buff.bloodbath.up|!talent.bloodbath.enabled))|target.time_to_die<=12|talent.anger_management.enabled)&((desired_targets=1&!raid_event.adds.exists)|!talent.bladestorm.enabled)",
                              "This incredibly long line (Due to differing talent choices) says 'Use recklessness on cooldown with colossus smash, unless the boss will die before the ability is usable again, and then use it with execute.'" );
  default_list -> add_talent( this, "Bloodbath", "if=(dot.rend.ticking&cooldown.colossus_smash.remains<5&((talent.ravager.enabled&prev_gcd.ravager)|!talent.ravager.enabled))|target.time_to_die<20" );
  default_list -> add_talent( this, "Avatar", "if=buff.recklessness.up|target.time_to_die<25" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
      default_list -> add_action( racial_actions[i] + ",if=rage<rage.max-40" );
    else
      default_list -> add_action( racial_actions[i] + ",if=buff.bloodbath.up|(!talent.bloodbath.enabled&debuff.colossus_smash.up)|buff.recklessness.up" );
  }

  default_list -> add_action( this, "Heroic Leap", "if=(raid_event.movement.distance>25&raid_event.movement.in>45)|!raid_event.movement.exists" );
  default_list -> add_action( "call_action_list,name=single,if=active_enemies=1" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>1" );

  movement -> add_action( this, "Heroic Leap" );
  movement -> add_action( this, "Charge", "cycle_targets=1,if=debuff.charge.down" );
  movement -> add_action( this, "Charge", "", "If possible, charge a target that will give us rage. Otherwise, just charge to get back in range." );
  for ( size_t i = 0; i < num_items; i++ )
  {
    if ( items[i].parsed.encoded_addon == "nitro_boosts" )
      movement -> add_action( "use_item,name=" + items[i].name_str + ",if=movement.distance>90" );
  }
  movement -> add_talent( this, "Storm Bolt", "", "May as well throw storm bolt if we can." );
  movement -> add_action( this, "Heroic Throw" );

  single_target -> add_action( this, "Rend", "if=target.time_to_die>4&dot.rend.remains<5.4&(target.health.pct>20|!debuff.colossus_smash.up)" );
  single_target -> add_talent( this, "Ravager", "if=cooldown.colossus_smash.remains<4&(!raid_event.adds.exists|raid_event.adds.in>55)" );
  single_target -> add_action( this, "Colossus Smash" );
  single_target -> add_action( this, "Mortal Strike", "if=target.health.pct>20" );
  single_target -> add_talent( this, "Bladestorm", "if=(((debuff.colossus_smash.up|cooldown.colossus_smash.remains>3)&target.health.pct>20)|(target.health.pct<20&rage<30&cooldown.colossus_smash.remains>4))&(!raid_event.adds.exists|raid_event.adds.in>55|(talent.anger_management.enabled&raid_event.adds.in>40))" );
  single_target -> add_talent( this, "Storm Bolt", "if=target.health.pct>20|(target.health.pct<20&!debuff.colossus_smash.up)" );
  single_target -> add_talent( this, "Siegebreaker" );
  single_target -> add_talent( this, "Dragon Roar", "if=!debuff.colossus_smash.up&(!raid_event.adds.exists|raid_event.adds.in>55|(talent.anger_management.enabled&raid_event.adds.in>40))" );
  single_target -> add_action( this, "Execute", "if=buff.sudden_death.react" );
  single_target -> add_action( this, "Execute", "if=!buff.sudden_death.react&(rage>72&cooldown.colossus_smash.remains>gcd)|debuff.colossus_smash.up|target.time_to_die<5" );
  single_target -> add_talent( this, "Impending Victory", "if=rage<40&target.health.pct>20&cooldown.colossus_smash.remains>1" );
  single_target -> add_talent( this, "Slam", "if=(rage>20|cooldown.colossus_smash.remains>gcd)&target.health.pct>20&cooldown.colossus_smash.remains>1" );
  single_target -> add_action( this, "Thunder Clap", "if=!talent.slam.enabled&target.health.pct>20&(rage>=40|debuff.colossus_smash.up)&glyph.resonating_power.enabled&cooldown.colossus_smash.remains>gcd" );
  single_target -> add_action( this, "Whirlwind", "if=!talent.slam.enabled&target.health.pct>20&(rage>=40|debuff.colossus_smash.up)&cooldown.colossus_smash.remains>gcd" );
  single_target -> add_talent( this, "Shockwave" );

  aoe -> add_action( this, "Sweeping Strikes" );
  aoe -> add_action( this, "Rend", "if=ticks_remain<2&target.time_to_die>4&(target.health.pct>20|!debuff.colossus_smash.up)" );
  aoe -> add_action( this, "Rend", "cycle_targets=1,max_cycle_targets=2,if=ticks_remain<2&target.time_to_die>8&!buff.colossus_smash_up.up&talent.taste_for_blood.enabled" );
  aoe -> add_action( this, "Rend", "cycle_targets=1,if=ticks_remain<2&target.time_to_die-remains>18&!buff.colossus_smash_up.up&active_enemies<=8" );
  aoe -> add_talent( this, "Ravager", "if=buff.bloodbath.up|cooldown.colossus_smash.remains<4" );
  aoe -> add_talent( this, "Bladestorm", "if=((debuff.colossus_smash.up|cooldown.colossus_smash.remains>3)&target.health.pct>20)|(target.health.pct<20&rage<30&cooldown.colossus_smash.remains>4)" );
  aoe -> add_action( this, "Colossus Smash", "if=dot.rend.ticking" );
  aoe -> add_action( this, "Execute", "cycle_targets=1,if=!buff.sudden_death.react&active_enemies<=8&((rage>72&cooldown.colossus_smash.remains>gcd)|rage>80|target.time_to_die<5|debuff.colossus_smash.up)" );
  aoe -> add_action( "heroic_charge,cycle_targets=1,if=target.health.pct<20&rage<70&swing.mh.remains>2&debuff.charge.down" );
  aoe -> add_action( this, "Mortal Strike", "if=target.health.pct>20&active_enemies<=5", "Heroic Charge is an event that makes the warrior heroic leap out of melee range for an instant\n"
                                                                                         "#If heroic leap is not available, the warrior will simply run out of melee to charge range, and then charge back in.\n"
                                                                                         "#This can delay autoattacks, but typically the rage gained from charging (Especially with bull rush glyphed) is more than\n"
                                                                                         "#The amount lost from delayed autoattacks. Charge only grants rage from charging a different target than the last time.\n"
                                                                                         "#Which means this is only worth doing on AoE, and only when you cycle your charge target." );
  aoe -> add_talent( this, "Dragon Roar", "if=!debuff.colossus_smash.up" );
  aoe -> add_action( this, "Thunder Clap", "if=(target.health.pct>20|active_enemies>=9)&glyph.resonating_power.enabled" );
  aoe -> add_action( this, "Rend", "cycle_targets=1,if=ticks_remain<2&target.time_to_die>8&!buff.colossus_smash_up.up&active_enemies>=9&rage<50&!talent.taste_for_blood.enabled" );
  aoe -> add_action( this, "Whirlwind", "if=target.health.pct>20|active_enemies>=9" );
  aoe -> add_talent( this, "Siegebreaker" );
  aoe -> add_talent( this, "Storm Bolt", "if=cooldown.colossus_smash.remains>4|debuff.colossus_smash.up" );
  aoe -> add_talent( this, "Shockwave" );
  aoe -> add_action( this, "Execute", "if=buff.sudden_death.react" );
}

// Protection Warrior Action Priority List ========================================

void warrior_t::apl_prot()
{
  //threshold for defensive abilities
  std::string threshold = "incoming_damage_2500ms>health.max*0.1";

  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* prot = get_action_priority_list( "prot" );
  action_priority_list_t* prot_aoe = get_action_priority_list( "prot_aoe" );

  default_list -> add_action( this, "charge" );
  default_list -> add_action( "auto_attack" );

  size_t num_items = items.size();
  for ( size_t i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      default_list -> add_action( "use_item,name=" + items[i].name_str + ",if=active_enemies=1&(buff.bloodbath.up|!talent.bloodbath.enabled)|(active_enemies>=2&buff.ravager_protection.up)" );
  }
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  default_list -> add_action( racial_actions[i] + ",if=buff.bloodbath.up|buff.avatar.up" );
  default_list -> add_action( this, "Berserker Rage", "if=buff.enrage.down" );
  default_list -> add_action( "call_action_list,name=prot" );

  //defensive
  prot -> add_action( this, "Shield Block", "if=!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.enraged_regeneration.up|buff.shield_block.up)" );
  prot -> add_action( this, "Shield Barrier", "if=buff.shield_barrier.down&((buff.shield_block.down&action.shield_block.charges_fractional<0.75)|rage>=85)" );
  prot -> add_action( this, "Demoralizing Shout", "if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.enraged_regeneration.up|buff.shield_block.up|buff.potion.up)" );
  prot -> add_talent( this, "Enraged Regeneration", "if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.enraged_regeneration.up|buff.shield_block.up|buff.potion.up)" );
  prot -> add_action( this, "Shield Wall", "if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.enraged_regeneration.up|buff.shield_block.up|buff.potion.up)" );
  prot -> add_action( this, "Last Stand", "if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.enraged_regeneration.up|buff.shield_block.up|buff.potion.up)" );

  //potion
  if ( sim -> allow_potions )
  {
    if ( level > 90 )
      prot -> add_action( "potion,name=draenic_armor,if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.enraged_regeneration.up|buff.shield_block.up|buff.potion.up)|target.time_to_die<=25" );
    else if ( level >= 80 )
      prot -> add_action( "potion,name=mountains,if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.enraged_regeneration.up|buff.shield_block.up|buff.potion.up)|target.time_to_die<=25" );
  }

  //stoneform
  prot -> add_action( "stoneform,if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.enraged_regeneration.up|buff.shield_block.up|buff.potion.up)" );

  //dps-single-target
  prot -> add_action( "call_action_list,name=prot_aoe,if=active_enemies>3" );
  prot -> add_action( this, "Heroic Strike", "if=buff.ultimatum.up|(talent.unyielding_strikes.enabled&buff.unyielding_strikes.stack>=6)" );
  prot -> add_talent( this, "Bloodbath", "if=talent.bloodbath.enabled&((cooldown.dragon_roar.remains=0&talent.dragon_roar.enabled)|(cooldown.storm_bolt.remains=0&talent.storm_bolt.enabled)|talent.shockwave.enabled)" );
  prot -> add_talent( this, "Avatar", "if=talent.avatar.enabled&((cooldown.ravager.remains=0&talent.ravager.enabled)|(cooldown.dragon_roar.remains=0&talent.dragon_roar.enabled)|(talent.storm_bolt.enabled&cooldown.storm_bolt.remains=0)|(!(talent.dragon_roar.enabled|talent.ravager.enabled|talent.storm_bolt.enabled)))" );
  prot -> add_action( this, "Shield Slam" );
  prot -> add_action( this, "Revenge" );
  prot -> add_talent( this, "Ravager" );
  prot -> add_talent( this, "Storm Bolt" );
  prot -> add_talent( this, "Dragon Roar" );
  prot -> add_talent( this, "Impending Victory", "if=talent.impending_victory.enabled&cooldown.shield_slam.remains<=execute_time" );
  prot -> add_action( this, "Victory Rush", "if=!talent.impending_victory.enabled&cooldown.shield_slam.remains<=execute_time" );
  prot -> add_action( this, "Execute", "if=buff.sudden_death.react" );
  prot -> add_action( this, "Devastate" );

  //dps-aoe
  prot_aoe -> add_talent( this, "Bloodbath" );
  prot_aoe -> add_talent( this, "Avatar" );
  prot_aoe -> add_action( this, "Thunder Clap", "if=!dot.deep_wounds.ticking" );
  prot_aoe -> add_action( this, "Heroic Strike", "if=buff.ultimatum.up|rage>110|(talent.unyielding_strikes.enabled&buff.unyielding_strikes.stack>=6)" );
  prot_aoe -> add_action( this, "Heroic Leap", "if=(raid_event.movement.distance>25&raid_event.movement.in>45)|!raid_event.movement.exists" );
  prot_aoe -> add_action( this, "Shield Slam", "if=buff.shield_block.up" );
  prot_aoe -> add_talent( this, "Ravager", "if=(buff.avatar.up|cooldown.avatar.remains>10)|!talent.avatar.enabled" );
  prot_aoe -> add_talent( this, "Dragon Roar", "if=(buff.bloodbath.up|cooldown.bloodbath.remains>10)|!talent.bloodbath.enabled" );
  prot_aoe -> add_talent( this, "Shockwave" );
  prot_aoe -> add_action( this, "Revenge" );
  prot_aoe -> add_action( this, "Thunder Clap" );
  prot_aoe -> add_talent( this, "Bladestorm" );
  prot_aoe -> add_action( this, "Shield Slam" );
  prot_aoe -> add_talent( this, "Storm Bolt" );
  prot_aoe -> add_action( this, "Shield Slam" );
  prot_aoe -> add_action( this, "Execute", "if=buff.sudden_death.react" );
  prot_aoe -> add_action( this, "Devastate" );
}

// Gladiator Warrior Action Priority List ========================================

void warrior_t::apl_glad()
{
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* movement = get_action_priority_list( "movement" );
  action_priority_list_t* gladiator = get_action_priority_list( "single" );
  action_priority_list_t* gladiator_aoe = get_action_priority_list( "aoe" );

  default_list -> add_action( this, "Charge" );
  default_list -> add_action( "auto_attack" );
  default_list -> add_action( "call_action_list,name=movement,if=movement.distance>5", "This is mostly to prevent cooldowns from being accidentally used during movement." );
  default_list -> add_talent( this, "Avatar" );
  default_list -> add_talent( this, "Bloodbath" );

  size_t num_items = items.size();
  for ( size_t i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) && items[i].parsed.encoded_addon != "nitro_boosts")
      default_list -> add_action( "use_item,name=" + items[i].name_str + ",if=buff.bloodbath.up|buff.avatar.up|buff.shield_charge.up|target.time_to_die<15" );
  }
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
      default_list -> add_action( racial_actions[i] + ",if=rage<rage.max-40" );
    else
      default_list -> add_action( racial_actions[i] + ",if=buff.bloodbath.up|buff.avatar.up|buff.shield_charge.up|target.time_to_die<10" );
  }
  if ( sim -> allow_potions )
    default_list -> add_action( "potion,name=draenic_armor,if=buff.bloodbath.up|buff.avatar.up|buff.shield_charge.up" );

  default_list -> add_action( "shield_charge,if=(!buff.shield_charge.up&!cooldown.shield_slam.remains)|charges=2" );
  default_list -> add_action( this, "Berserker Rage", "if=buff.enrage.down" );
  default_list -> add_action( this, "Heroic Leap", "if=(raid_event.movement.distance>25&raid_event.movement.in>45)|!raid_event.movement.exists" );
  default_list -> add_action( this, "Heroic Strike", "if=(buff.shield_charge.up|(buff.unyielding_strikes.up&rage>=80-buff.unyielding_strikes.stack*10))&target.health.pct>20" );
  default_list -> add_action( this, "Heroic Strike", "if=buff.ultimatum.up|rage>=rage.max-20|buff.unyielding_strikes.stack>4|target.time_to_die<10" );
  default_list -> add_action( "call_action_list,name=single,if=active_enemies=1" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>=2" );

  movement -> add_action( this, "Heroic Leap" );
  movement -> add_action( "shield_charge" );
  for ( size_t i = 0; i < num_items; i++ )
  {
    if ( items[i].parsed.encoded_addon == "nitro_boosts" )
      movement -> add_action( "use_item,name=" + items[i].name_str + ",if=movement.distance>90" );
  }
  movement -> add_talent( this, "Storm Bolt", "", "May as well throw storm bolt if we can." );
  movement -> add_action( this, "Heroic Throw" );

  gladiator -> add_action( this, "Devastate", "if=buff.unyielding_strikes.stack>0&buff.unyielding_strikes.stack<6&buff.unyielding_strikes.remains<1.5" );
  gladiator -> add_action( this, "Shield Slam" );
  gladiator -> add_action( this, "Revenge" );
  gladiator -> add_action( this, "Execute", "if=buff.sudden_death.react" );
  gladiator -> add_talent( this, "Storm Bolt" );
  gladiator -> add_talent( this, "Dragon Roar", "if=buff.unyielding_strikes.stack>=4&buff.unyielding_strikes.stack<6" );
  gladiator -> add_action( this, "Execute", "if=rage>60&target.health.pct<20" );
  gladiator -> add_action( this, "Devastate" );

  gladiator_aoe -> add_action( this, "Revenge" );
  gladiator_aoe -> add_action( this, "Shield Slam" );
  gladiator_aoe -> add_talent( this, "Dragon Roar", "if=(buff.bloodbath.up|cooldown.bloodbath.remains>10)|!talent.bloodbath.enabled" );
  gladiator_aoe -> add_talent( this, "Storm Bolt", "if=(buff.bloodbath.up|cooldown.bloodbath.remains>7)|!talent.bloodbath.enabled" );
  gladiator_aoe -> add_action( "thunder_clap,cycle_targets=1,if=dot.deep_wounds.remains<3&active_enemies>4" );
  gladiator_aoe -> add_talent( this, "Bladestorm", "if=buff.shield_charge.down" );
  gladiator_aoe -> add_action( this, "Execute", "if=buff.sudden_death.react" );
  gladiator_aoe -> add_action( this, "Thunder Clap", "if=active_enemies>6" );
  gladiator_aoe -> add_action( "devastate,cycle_targets=1,if=dot.deep_wounds.remains<5&cooldown.shield_slam.remains>execute_time*0.4" );
  gladiator_aoe -> add_action( this, "Devastate", "if=cooldown.shield_slam.remains>execute_time*0.4" );
}

// NO Spec Combat Action Priority List

void warrior_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list -> add_action( this, "Heroic Throw" );
}

namespace rppm{

template <typename Base>
struct warrior_real_ppm_t: public Base
{
  public:
  typedef warrior_real_ppm_t base_t;

  warrior_real_ppm_t( warrior_t& p, const real_ppm_t& params ):
    Base( params ), warrior( p )
  {}

  protected:
  warrior_t& warrior;
};

struct sudden_death_t: public warrior_real_ppm_t < real_ppm_t >
{
  sudden_death_t( warrior_t& p ):
    base_t( p, real_ppm_t( p, 2.5, RPPM_HASTE ) )
  {}
};
};

// =========================================================================
// Buff Help Classes
// =========================================================================

// Defensive Stance Rage Gain ==============================================

static void defensive_stance( buff_t* buff, int, int )
{
  warrior_t* p = debug_cast<warrior_t*>( buff -> player );

  p -> resource_gain( RESOURCE_RAGE, 1, p -> gain.defensive_stance );
  buff -> refresh();
}

// Fury T17 4 piece ========================================================

static void tier17_4pc_fury( buff_t* buff, int, int )
{
  warrior_t* p = debug_cast<warrior_t*>( buff -> player );

  p -> buff.tier17_4pc_fury -> trigger( 1 );
}

// ==========================================================================
// Warrior Buffs
// ==========================================================================

namespace buffs
{
template <typename Base>
struct warrior_buff_t: public Base
{
public:
  typedef warrior_buff_t base_t;

  warrior_buff_t( warrior_td_t& p, const buff_creator_basics_t& params ):
    Base( params ), warrior( p.warrior )
  {}

  warrior_buff_t( warrior_t& p, const buff_creator_basics_t& params ):
    Base( params ), warrior( p )
  {}

  warrior_td_t& get_td( player_t* t ) const
  {
    return *( warrior.get_target_data( t ) );
  }

protected:
  warrior_t& warrior;
};

struct bloodsurge_t: public warrior_buff_t < buff_t >
{
  int wasted;
  bloodsurge_t( warrior_t& p, const std::string&n, const spell_data_t*s ):
    base_t( p, buff_creator_t( &p, n, s )
    .chance( p.spec.bloodsurge -> effectN( 1 ).percent() ) ), wasted( 0 )
  {}

  void execute( int a, double b, timespan_t t )
  {
    wasted = 2;
    base_t::execute( a, b, t );
  }

  void decrement( int a, double b )
  {
    wasted--;
    base_t::decrement( a, b );
  }

  void reset()
  {
    base_t::reset();
    wasted = 0;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration )
  {
    if ( wasted > 0 )
    {
      do
      {
        warrior.proc.bloodsurge_wasted -> occur();
        wasted--;
      }
      while ( wasted > 0 );
    }
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct defensive_stance_t: public warrior_buff_t < buff_t >
{
  defensive_stance_t( warrior_t& p, const std::string&n, const spell_data_t*s ):
    base_t( p, buff_creator_t( &p, n, s )
    .can_cancel( false )
    .activated( true )
    .tick_callback( defensive_stance )
    .tick_behavior( BUFF_TICK_REFRESH )
    .refresh_behavior( BUFF_REFRESH_TICK )
    .add_invalidate( CACHE_EXP )
    .add_invalidate( CACHE_CRIT_AVOIDANCE )
    .add_invalidate( CACHE_CRIT_BLOCK )
    .add_invalidate( CACHE_BLOCK )
    .add_invalidate( CACHE_STAMINA )
    .add_invalidate( CACHE_ARMOR )
    .add_invalidate( CACHE_BONUS_ARMOR )
    .duration( timespan_t::from_seconds( 3 ) )
    .period( timespan_t::from_seconds( 3 ) ) )
  {}

  void execute( int a, double b, timespan_t t )
  {
    warrior.active.stance = STANCE_DEFENSE;
    base_t::execute( a, b, t );
  }
};

struct battle_stance_t: public warrior_buff_t < buff_t >
{
  battle_stance_t( warrior_t& p, const std::string&n, const spell_data_t*s ):
    base_t( p, buff_creator_t( &p, n, s )
    .activated( true ) 
    .can_cancel( false ) )
  {}

  void execute( int a, double b, timespan_t t )
  {
    warrior.active.stance = STANCE_BATTLE;
    base_t::execute( a, b, t );
  }
};

struct gladiator_stance_t: public warrior_buff_t < buff_t >
{
  gladiator_stance_t( warrior_t& p, const std::string&n, const spell_data_t*s ):
    base_t( p, buff_creator_t( &p, n, s )
    .activated( true )
    .can_cancel( false )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  {}

  void execute( int a, double b, timespan_t t )
  {
    warrior.active.stance = STANCE_GLADIATOR;
    base_t::execute( a, b, t );
  }
};

struct rallying_cry_t: public warrior_buff_t < buff_t >
{
  int health_gain;
  rallying_cry_t( warrior_t& p, const std::string&n, const spell_data_t*s ):
    base_t( p, buff_creator_t( &p, n, s ) ), health_gain( 0 )
  {}

  bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    health_gain = static_cast<int>( util::floor( warrior.resources.max[RESOURCE_HEALTH] * data().effectN( 1 ).percent() ) );
    warrior.stat_gain( STAT_MAX_HEALTH, health_gain, (gain_t*)0, (action_t*)0, true );
    return base_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration )
  {
    warrior.stat_loss( STAT_MAX_HEALTH, health_gain, (gain_t*)0, (action_t*)0, true );
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct last_stand_t: public warrior_buff_t < buff_t >
{
  int health_gain;
  last_stand_t( warrior_t& p, const std::string&n, const spell_data_t*s ):
    base_t( p, buff_creator_t( &p, n, s ).cd( timespan_t::zero() ) ), health_gain( 0 )
  {}

  bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    health_gain = static_cast<int>( util::floor( warrior.resources.max[RESOURCE_HEALTH] * warrior.spec.last_stand -> effectN( 1 ).percent() ) );
    warrior.stat_gain( STAT_MAX_HEALTH, health_gain, (gain_t*)0, (action_t*)0, true );
    return base_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration )
  {
    warrior.stat_loss( STAT_MAX_HEALTH, health_gain, (gain_t*)0, (action_t*)0, true );
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct debuff_demo_shout_t: public warrior_buff_t < buff_t >
{
  debuff_demo_shout_t( warrior_td_t& p ):
    base_t( p, buff_creator_t( p, "demoralizing_shout", p.source -> find_specialization_spell( "Demoralizing Shout" ) ) )
  {
    default_value = data().effectN( 1 ).percent();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration )
  {
    if ( warrior.sets.has_set_bonus( SET_TANK, T16, B4 ) )
      warrior.buff.tier16_reckless_defense -> trigger();

    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};
} // end namespace buffs

// ==========================================================================
// Warrior Character Definition
// ==========================================================================

warrior_td_t::warrior_td_t( player_t* target, warrior_t& p ):
actor_pair_t( target, &p ), warrior( p )
{
  using namespace buffs;

  dots_bloodbath   = target -> get_dot( "bloodbath", &p );
  dots_deep_wounds = target -> get_dot( "deep_wounds", &p );
  dots_ravager     = target -> get_dot( "ravager", &p );
  dots_rend        = target -> get_dot( "rend", &p );

  debuffs_colossus_smash = buff_creator_t( *this, "colossus_smash" )
    .duration( p.spec.colossus_smash -> duration() )
    .cd( timespan_t::zero() );

  debuffs_charge = buff_creator_t( *this, "charge" )
    .duration( timespan_t::zero() );

  debuffs_demoralizing_shout = new buffs::debuff_demo_shout_t( *this );
  debuffs_taunt = buff_creator_t( *this, "taunt", p.find_class_spell( "Taunt" ) );
}

// warrior_t::init_buffs ====================================================

void warrior_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  buff.debuffs_slam = buff_creator_t( this, "slam", talents.slam )
    .can_cancel( false );

  buff.avatar = buff_creator_t( this, "avatar", talents.avatar )
    .cd( timespan_t::zero() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.battle_stance = new buffs::battle_stance_t( *this, "battle_stance", find_class_spell( "Battle Stance" ) );

  buff.berserker_rage = buff_creator_t( this, "berserker_rage", find_class_spell( "Berserker Rage" ) );

  buff.bladestorm = buff_creator_t( this, "bladestorm", talents.bladestorm )
    .period( timespan_t::zero() )
    .cd( timespan_t::zero() );

  buff.bloodbath = buff_creator_t( this, "bloodbath", talents.bloodbath )
    .cd( timespan_t::zero() );

  buff.blood_craze = buff_creator_t( this, "blood_craze", find_spell( 159363 ) );

  buff.bloodsurge = new buffs::bloodsurge_t( *this, "bloodsurge", spec.bloodsurge -> effectN( 1 ).trigger() );

  buff.colossus_smash = buff_creator_t( this, "colossus_smash_up", spec.colossus_smash )
    .duration( spec.colossus_smash -> duration() )
    .cd( timespan_t::zero() );

  buff.defensive_stance = new buffs::defensive_stance_t( *this, "defensive_stance", find_class_spell( "Defensive Stance" ) );

  buff.die_by_the_sword = buff_creator_t( this, "die_by_the_sword", spec.die_by_the_sword )
    .default_value( spec.die_by_the_sword -> effectN( 2 ).percent() + perk.improved_die_by_the_sword -> effectN( 1 ).percent() )
    .cd( timespan_t::zero() )
    .add_invalidate( CACHE_PARRY );

  buff.enrage = buff_creator_t( this, "enrage", spec.enrage -> effectN( 1 ).trigger() )
    .can_cancel( false )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.enraged_regeneration = buff_creator_t( this, "enraged_regeneration", talents.enraged_regeneration );

  buff.enraged_speed = buff_creator_t( this, "enraged_speed", glyphs.enraged_speed )
    .can_cancel( false )
    .duration( buff.enrage -> data().duration() );

  buff.gladiator_stance = new buffs::gladiator_stance_t( *this, "gladiator_stance", find_spell( talents.gladiators_resolve -> effectN( 1 ).base_value() ) );

  buff.hamstring = buff_creator_t( this, "hamstring", glyphs.hamstring -> effectN( 1 ).trigger() );

  buff.heroic_leap_glyph = buff_creator_t( this, "heroic_leap_glyph", find_spell( 133278 ) )
    .chance( glyphs.heroic_leap -> ok() ? 1.0 : 0 );

  buff.heroic_leap_movement = buff_creator_t( this, "heroic_leap_movement" );
  buff.charge_movement = buff_creator_t( this, "charge_movement" );
  buff.shield_charge_movement = buff_creator_t( this, "shield_charge_movement" );
  buff.intervene_movement = buff_creator_t( this, "intervene_movement" );

  buff.last_stand = new buffs::last_stand_t( *this, "last_stand", spec.last_stand );

  buff.meat_cleaver = buff_creator_t( this, "meat_cleaver", spec.meat_cleaver -> effectN( 1 ).trigger() )
    .max_stack( perk.enhanced_whirlwind -> ok() ? 4 : 3 );

  buff.raging_blow = buff_creator_t( this, "raging_blow", find_spell( 131116 ) )
    .cd( timespan_t::zero() );
  // The buff has a 0.5 second ICD in spell data, but from in game testing this doesn't
  // do anything. Also confirmed from sparkle dragon.

  buff.raging_blow_glyph = buff_creator_t( this, "raging_blow_glyph", glyphs.raging_blow -> effectN( 1 ).trigger() );

  buff.raging_wind = buff_creator_t( this, "raging_wind", glyphs.raging_wind -> effectN( 1 ).trigger() );

  buff.rallying_cry = new buffs::rallying_cry_t( *this, "rallying_cry", find_spell( 97463 ) );

  buff.ravager = buff_creator_t( this, "ravager", talents.ravager );

  buff.ravager_protection = buff_creator_t( this, "ravager_protection", talents.ravager )
    .add_invalidate( CACHE_PARRY );

  buff.recklessness = buff_creator_t( this, "recklessness", spec.recklessness )
    .duration( spec.recklessness -> duration() * ( 1.0 + glyphs.recklessness -> effectN( 2 ).percent() ) )
    .cd( timespan_t::zero() );

  buff.rude_interruption = buff_creator_t( this, "rude_interruption", glyphs.rude_interruption -> effectN( 1 ).trigger() )
    .default_value( glyphs.rude_interruption -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.shield_barrier = absorb_buff_creator_t( this, "shield_barrier", specialization() == WARRIOR_PROTECTION ? find_spell( 112048 ) : spec.shield_barrier )
    .source( get_stats( "shield_barrier" ) );

  buff.shield_block = buff_creator_t( this, "shield_block", find_spell( 132404 ) )
    .cd( timespan_t::zero() )
    .add_invalidate( CACHE_BLOCK );

  buff.shield_charge = buff_creator_t( this, "shield_charge", find_spell( 169667 ) )
    .default_value( find_spell( 169667 ) -> effectN( 1 ).percent() + sets.set( WARRIOR_PROTECTION, T17, B4 ) -> effectN( 2 ).percent() )
    .cd( timespan_t::zero() );

  buff.shield_wall = buff_creator_t( this, "shield_wall", spec.shield_wall )
    .default_value( spec.shield_wall -> effectN( 1 ).percent() )
    .cd( timespan_t::zero() );

  buff.sudden_death = buff_creator_t( this, "sudden_death", talents.sudden_death -> effectN( 1 ).trigger() );

  buff.sweeping_strikes = buff_creator_t( this, "sweeping_strikes", spec.sweeping_strikes )
    .duration( spec.sweeping_strikes -> duration() + perk.enhanced_sweeping_strikes -> effectN( 1 ).time_value() )
    .cd( timespan_t::zero() );

  buff.sword_and_board = buff_creator_t( this, "sword_and_board", find_spell( 50227 ) )
    .chance( spec.sword_and_board -> effectN( 1 ).percent() );

  buff.tier15_2pc_tank = buff_creator_t( this, "tier15_2pc_tank", sets.set( SET_TANK, T15, B2 ) -> effectN( 1 ).trigger() );

  buff.tier16_reckless_defense = buff_creator_t( this, "tier16_reckless_defense", find_spell( 144500 ) );

  buff.tier16_4pc_death_sentence = buff_creator_t( this, "death_sentence", find_spell( 144442 ) )
    .chance( sets.set( SET_MELEE, T16, B4 ) -> effectN( 1 ).percent() );

  buff.tier17_2pc_arms = buff_creator_t( this, "tier17_2pc_arms", sets.set( WARRIOR_ARMS, T17, B2 ) -> effectN( 1 ).trigger() )
    .chance( sets.set( WARRIOR_ARMS, T17, B2 ) -> proc_chance() );

  buff.tier17_4pc_fury = buff_creator_t( this, "rampage", sets.set( WARRIOR_FURY, T17, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
    .add_invalidate( CACHE_ATTACK_SPEED )
    .add_invalidate( CACHE_CRIT );

  buff.tier17_4pc_fury_driver = buff_creator_t( this, "rampage_driver", sets.set( WARRIOR_FURY, T17, B4 ) -> effectN( 1 ).trigger() )
    .tick_callback( tier17_4pc_fury );

  buff.pvp_2pc_arms = buff_creator_t( this, "pvp_2pc_arms", sets.set( WARRIOR_ARMS, PVP, B2 ) -> effectN( 1 ).trigger() )
    .default_value( sets.set( WARRIOR_ARMS, PVP, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );

  buff.pvp_2pc_fury = buff_creator_t( this, "pvp_2pc_fury", sets.set( WARRIOR_FURY, PVP, B2 ) -> effectN( 1 ).trigger() )
    .default_value( sets.set( WARRIOR_FURY, PVP, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );

  buff.pvp_2pc_prot = buff_creator_t( this, "pvp_2pc_prot", sets.set( WARRIOR_PROTECTION, PVP, B2 ) -> effectN( 1 ).trigger() )
    .default_value( sets.set( WARRIOR_PROTECTION, PVP, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );

  buff.unyielding_strikes = buff_creator_t( this, "unyielding_strikes", talents.unyielding_strikes -> effectN( 1 ).trigger() )
    .default_value( talents.unyielding_strikes -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE ) );

  buff.ultimatum = buff_creator_t( this, "ultimatum", spec.ultimatum -> effectN( 1 ).trigger() );
}

// warrior_t::init_scaling ==================================================

void warrior_t::init_scaling()
{
  player_t::init_scaling();

  if ( specialization() == WARRIOR_FURY )
    scales_with[STAT_WEAPON_OFFHAND_DPS] = true;

  if ( specialization() == WARRIOR_PROTECTION )
    scales_with[STAT_BONUS_ARMOR] = true;

  scales_with[STAT_AGILITY] = false;
}

// warrior_t::init_gains ====================================================

void warrior_t::init_gains()
{
  player_t::init_gains();

  gain.avoided_attacks        = get_gain( "avoided_attacks" );
  gain.bloodthirst            = get_gain( "bloodthirst" );
  gain.charge                 = get_gain( "charge" );
  gain.critical_block         = get_gain( "critical_block" );
  gain.defensive_stance       = get_gain( "defensive_stance" );
  gain.drawn_sword_glyph      = get_gain( "drawn_sword_glyph" );
  gain.enrage                 = get_gain( "enrage" );
  gain.melee_crit             = get_gain( "melee_crit" );
  gain.melee_main_hand        = get_gain( "melee_main_hand" );
  gain.melee_off_hand         = get_gain( "melee_off_hand" );
  gain.revenge                = get_gain( "revenge" );
  gain.shield_slam            = get_gain( "shield_slam" );
  gain.sweeping_strikes       = get_gain( "sweeping_strikes" );
  gain.sword_and_board        = get_gain( "sword_and_board" );
  gain.taste_for_blood        = get_gain( "taste_for_blood" );

  gain.tier15_4pc_tank        = get_gain( "tier15_4pc_tank" );
  gain.tier16_2pc_melee       = get_gain( "tier16_2pc_melee" );
  gain.tier16_4pc_tank        = get_gain( "tier16_4pc_tank" );
  gain.tier17_4pc_arms        = get_gain( "tier17_4pc_arms" );
}

// warrior_t::init_position ====================================================

void warrior_t::init_position()
{
  player_t::init_position();

  if ( specialization() == WARRIOR_PROTECTION && primary_role() == ROLE_ATTACK )
  {
    base.position = POSITION_BACK;
    position_str = util::position_type_string( base.position );
    if ( sim -> debug )
      sim -> out_debug.printf( "%s: Position adjusted to %s for Gladiator DPS", name(), position_str.c_str() );
  }
}

// warrior_t::init_procs ======================================================

void warrior_t::init_procs()
{
  player_t::init_procs();
  proc.bloodsurge_wasted       = get_proc( "bloodsurge_wasted" );
  proc.delayed_auto_attack     = get_proc( "delayed_auto_attack" );

  proc.t17_2pc_fury            = get_proc( "t17_2pc_fury" );
  proc.t17_2pc_arms            = get_proc( "t17_2pc_arms" );
}

// warrior_t::init_rng ========================================================

void warrior_t::init_rng()
{
  player_t::init_rng();

  rppm.sudden_death = std::shared_ptr<rppm::sudden_death_t>( new rppm::sudden_death_t( *this ) );
}

// warrior_t::init_resources ================================================

void warrior_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[RESOURCE_RAGE] = 0;
}

// warrior_t::init_actions ==================================================

void warrior_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );

    quiet = true;
    return;
  }

  clear_action_priority_lists();

  // This section is dedicated to "how in the hell do we find out if the person is a gladiator dps, or just a 
  // tank who likes the 5% reduced damage taken."
  // First up, why not check to see if the player has the talent?
  gladiator = talents.gladiators_resolve -> ok();

  if ( gladiator )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK ) // Allow players with tanking trinkets to simulate gladiator.
    {
      gladiator = true;
    }
    // Second, if the person specifically selects tank, then I guess we're rolling with a tank.
    else if ( primary_role() == ROLE_TANK )
      gladiator = false;
    // Next, "Mark of Blackrock" is a enchant that procs bonus armor, but will only proc if the character goes below 50% hp.
    // Thus, it seems like it will be the best enchant for tanks, but fairly awful for gladiator since dps-specs don't spend a lot of time under 50% hp.
    else if ( find_proc( "Mark of Blackrock" ) != nullptr )
    {
      gladiator = false;
      sim -> out_debug.printf( "%s: Has been imported as a Tank, as it has Mark of Blackrock enchanted.", name() );
    }
    // Next, check both trinkets for stamina. In the future I will add more items to check, for now this will work.
    // If the trinkets have stamina, then it's likely a tank.
    else
    {
      size_t num_items = items.size();
      for ( size_t i = 0; i < num_items; i++ )
      {
        if ( items[i].slot == SLOT_TRINKET_1 || items[i].slot == SLOT_TRINKET_2 )
        {
          if ( items[i].has_item_stat( STAT_STAMINA ) )
          {
            gladiator = false;
            sim -> out_debug.printf( "%s: Has been imported as a Tank, due to wearing a trinket with stamina.", name() );
            break;
          }
        }
      }
    }
    if ( gladiator && !( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK ) )
    {
      sim -> out_debug.printf( "%s: Has been imported as a Gladiator DPS, due to not wearing a stamina trinket and not having Mark of Blackrock enchanted. If you wish to override this, set role to tank, and re-import.", name() );
    }
  }

  apl_precombat();

  switch ( specialization() )
  {
  case WARRIOR_FURY:
    apl_fury();
    break;
  case WARRIOR_ARMS:
    apl_arms();
    break;
  case WARRIOR_PROTECTION:
    if ( !gladiator )
      apl_prot();
    else
      apl_glad();
    break;
  default:
    apl_default(); // DEFAULT
    break;
  }

  // Default
  use_default_action_list = true;
  player_t::init_action_list();
}

// warrior_t::arise() ======================================================

void warrior_t::arise()
{
  player_t::arise();

  if ( active.stance == STANCE_BATTLE )
    buff.battle_stance -> trigger();
  else if ( active.stance == STANCE_GLADIATOR )
    buff.gladiator_stance -> trigger();
  else if ( active.stance == STANCE_DEFENSE )
    buff.defensive_stance -> trigger();

  if ( specialization() != WARRIOR_PROTECTION  && !sim -> overrides.versatility && level >= 80 ) // Currently it is impossible to remove this.
    sim -> auras.versatility -> trigger();
}

// warrior_t::combat_begin ==================================================

void warrior_t::combat_begin()
{
  if ( !sim -> fixed_time )
  {
    if ( warrior_fixed_time )
    {
      for ( size_t i = 0; i < sim -> player_list.size(); ++i )
      {
        player_t* p = sim -> player_list[i];
        if ( p -> specialization() != WARRIOR_FURY && p -> specialization() != WARRIOR_ARMS )
        {
          warrior_fixed_time = false;
          break;
        }
      }
      if ( warrior_fixed_time )
      {
        sim -> fixed_time = true;
        sim -> errorf( "To fix issues with the target exploding <20% range due to execute, fixed_time=1 has been enabled. This gives similar results" );
        sim -> errorf( "to execute's usage in a raid sim, without taking an eternity to simulate. To disable this option, add warrior_fixed_time=0 to your sim." );
      }
    }
  }
  player_t::combat_begin();

  if ( initial_rage > 0 )
    resources.current[RESOURCE_RAGE] = initial_rage; // User specified rage.

  if ( specialization() == WARRIOR_PROTECTION )
    resolve_manager.start();
}

// warrior_t::reset =========================================================

void warrior_t::reset()
{
  player_t::reset();

  active.stance = STANCE_BATTLE;
  if ( gladiator )
  {
    active.stance = STANCE_GLADIATOR;
  }

  heroic_charge = 0;
  last_target_charged = 0;
  if ( rppm.sudden_death )
  {
    rppm.sudden_death -> reset();
  }
}

// Movement related overrides. =============================================

void warrior_t::moving()
{
  return;
}

void warrior_t::interrupt()
{
  buff.charge_movement -> expire();
  buff.heroic_leap_movement -> expire();
  buff.shield_charge_movement -> expire();
  buff.intervene_movement -> expire();
  if ( heroic_charge )
  {
    event_t::cancel( heroic_charge );
  }
  player_t::interrupt();
}

void warrior_t::teleport( double, timespan_t )
{
  return; // All movement "teleports" are modeled.
}

void warrior_t::trigger_movement( double distance, movement_direction_e direction )
{
  if ( heroic_charge )
  {
    event_t::cancel( heroic_charge ); // Cancel heroic leap if it's running to make sure nothing weird happens when movement from another source is attempted.
    player_t::trigger_movement( distance, direction );
  }
  else
  {
    player_t::trigger_movement( distance, direction );
  }
}

// warrior_t::composite_player_multiplier ===================================

double warrior_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buff.avatar -> up() )
    m *= 1.0 + buff.avatar -> data().effectN( 1 ).percent();

  if ( main_hand_weapon.group() == WEAPON_2H && spec.seasoned_soldier -> ok() )
    m *= 1.0 + spec.seasoned_soldier -> effectN( 1 ).percent();

  // --- Enrages ---
  if ( buff.enrage -> up() )
  {
    m *= 1.0 + buff.enrage -> data().effectN( 2 ).percent();

    if ( mastery.unshackled_fury -> ok() )
      m *= 1.0 + cache.mastery_value();
  }

  if ( main_hand_weapon.group() == WEAPON_1H &&
       off_hand_weapon.group() == WEAPON_1H )
    m *= 1.0 + spec.singleminded_fury -> effectN( 1 ).percent();

  // --- Buffs / Procs ---
  if ( buff.rude_interruption -> up() )
    m *= 1.0 + buff.rude_interruption -> value();

  if ( active.stance == STANCE_GLADIATOR )
  {
    if ( dbc::is_school( school, SCHOOL_PHYSICAL ) )
    {
      m *= 1.0 + buff.gladiator_stance -> data().effectN( 1 ).percent();
    }
  }

  return m;
}

// warrior_t::composite_attribute =============================================

double warrior_t::composite_attribute( attribute_e attr ) const
{
  double a = player_t::composite_attribute( attr );

  switch ( attr )
  {
  case ATTR_STAMINA:
    if ( active.stance == STANCE_DEFENSE )
      a += spec.unwavering_sentinel -> effectN( 1 ).percent() * player_t::composite_attribute( ATTR_STAMINA );
    break;
  default:
    break;
  }
  return a;
}



double warrior_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( active.stance == STANCE_DEFENSE )
  {
    a *= 1.0 + perk.improved_defensive_stance -> effectN( 1 ).percent();
  }

  return a;
}

// warrior_t::composite_melee_expertise =====================================

double warrior_t::composite_melee_expertise( weapon_t* ) const
{
  double e = player_t::composite_melee_expertise();

  if ( active.stance == STANCE_DEFENSE )
    e += spec.unwavering_sentinel -> effectN( 5 ).percent();

  return e;
}

// warrior_t::composite_rating_multiplier =====================================

double warrior_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
  case RATING_MELEE_CRIT:
    return m *= 1.0 + spec.cruelty -> effectN( 1 ).percent();
  case RATING_SPELL_CRIT:
    return m *= 1.0 + spec.cruelty -> effectN( 1 ).percent();
  case RATING_MASTERY:
    m *= 1.0 + spec.weapon_mastery -> effectN( 1 ).percent();
    m *= 1.0 + spec.shield_mastery -> effectN( 1 ).percent();
    return m;
    break;
  default:
    break;
  }
  return m;
}

// warrior_t::matching_gear_multiplier ======================================

double warrior_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( ( attr == ATTR_STRENGTH ) && ( specialization() != WARRIOR_PROTECTION ) )
    return 0.05;

  if ( ( attr == ATTR_STAMINA ) && ( specialization() == WARRIOR_PROTECTION ) )
    return 0.05;

  return 0.0;
}

// warrior_t::composite_block ================================================

double warrior_t::composite_block() const
{
  // this handles base block and and all block subject to diminishing returns
  double block_subject_to_dr = cache.mastery() * mastery.critical_block -> effectN( 2 ).mastery_value();
  double b = player_t::composite_block_dr( block_subject_to_dr );

  // add in spec- and perk-specific block bonuses not subject to DR
  b += spec.bastion_of_defense -> effectN( 1 ).percent();
  b += perk.improved_block -> effectN( 1 ).percent();

  // shield block adds 100% block chance
  if ( buff.shield_block -> up() )
    b += buff.shield_block -> data().effectN( 1 ).percent();

  return b;
}

// warrior_t::composite_block_reduction ======================================

double warrior_t::composite_block_reduction() const
{
  double br = player_t::composite_block_reduction();

  // Prot T17 4-pc increases block value by 5% while shield block is active (additive)
  if ( buff.shield_block -> up() )
  {
    if ( sets.has_set_bonus( WARRIOR_PROTECTION, T17, B4 ) )
      br += spell.t17_prot_2p -> effectN( 1 ).percent();
  }

  return br;
}

// warrior_t::composite_melee_attack_power ==================================

double warrior_t::composite_melee_attack_power() const
{
  double ap = player_t::composite_melee_attack_power();

  ap += spec.bladed_armor -> effectN( 1 ).percent() * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
}

// warrior_t::composite_parry_rating() ========================================

double warrior_t::composite_parry_rating() const
{
  double p = player_t::composite_parry_rating();

  // add Riposte
  if ( spec.riposte -> ok() )
    p += composite_melee_crit_rating();

  return p;
}

// warrior_t::composite_parry =================================================

double warrior_t::composite_parry() const
{
  double parry = player_t::composite_parry();

  if ( buff.ravager_protection -> up() )
    parry += talents.ravager -> effectN( 1 ).percent();

  if ( buff.die_by_the_sword -> up() )
    parry += spec.die_by_the_sword -> effectN( 1 ).percent();

  return parry;
}

// warrior_t::composite_attack_power_multiplier ==============================

double warrior_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  if ( mastery.critical_block -> ok() )
    ap *= 1.0 + cache.mastery() * mastery.critical_block -> effectN( 5 ).mastery_value();

  return ap;
}

// warrior_t::composite_crit_block =====================================

double warrior_t::composite_crit_block() const
{
  double b = player_t::composite_crit_block();

  if ( mastery.critical_block -> ok() )
    b += cache.mastery() * mastery.critical_block -> effectN( 1 ).mastery_value();

  return b;
}

// warrior_t::composite_crit_avoidance ===========================================

double warrior_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  if ( active.stance == STANCE_DEFENSE )
    c += spec.unwavering_sentinel -> effectN( 4 ).percent();

  return c;
}

// warrior_t::composite_melee_speed ========================================

double warrior_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();

  if ( buff.tier17_4pc_fury -> up() )
  {
    s /= 1.0 + buff.tier17_4pc_fury -> current_stack *
      sets.set( WARRIOR_FURY, T17, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() -> effectN( 1 ).percent();
  }

  return s;
}

// warrior_t::composite_melee_crit =========================================

double warrior_t::composite_melee_crit() const
{
  double c = player_t::composite_melee_crit();

  if ( buff.tier17_4pc_fury -> up() )
  {
    c += buff.tier17_4pc_fury -> current_stack *
      sets.set( WARRIOR_FURY, T17, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() -> effectN( 1 ).percent();
  }

  return c;
}

// warrior_t::composite_player_critical_damage_multiplier ==================

double warrior_t::composite_player_critical_damage_multiplier() const
{
  double cdm = player_t::composite_player_critical_damage_multiplier();

  if ( buff.recklessness -> check() )
    cdm += ( buff.recklessness -> data().effectN( 2 ).percent() * ( 1.0 + glyphs.recklessness -> effectN( 1 ).percent() ) );

  return cdm;
}

// warrior_t::composite_spell_crit =========================================

double warrior_t::composite_spell_crit() const
{
  return composite_melee_crit();
}

// warrior_t::temporary_movement_modifier ==================================

double warrior_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  // These are ordered in the highest speed movement increase to the lowest, there's no reason to check the rest as they will just be overridden.
  // Also gives correct benefit numbers.
  if ( buff.heroic_leap_movement -> up() )
    temporary = std::max( buff.heroic_leap_movement -> value(), temporary );
  else if ( buff.charge_movement -> up() )
    temporary = std::max( buff.charge_movement -> value(), temporary );
  else if ( buff.intervene_movement -> up() )
    temporary = std::max( buff.intervene_movement -> value(), temporary );
  else if ( buff.shield_charge_movement -> up() )
    temporary = std::max( buff.shield_charge_movement -> value(), temporary );
  else if ( buff.heroic_leap_glyph -> up() )
    temporary = std::max( buff.heroic_leap_glyph -> data().effectN( 1 ).percent(), temporary );
  else if ( buff.enraged_speed -> up() )
    temporary = std::max( buff.enraged_speed -> data().effectN( 1 ).percent(), temporary );

  return temporary;
}

// warrior_t::invalidate_cache ==============================================

void warrior_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( c == CACHE_ATTACK_CRIT && mastery.critical_block -> ok() )
    player_t::invalidate_cache( CACHE_PARRY );

  if ( c == CACHE_MASTERY && mastery.critical_block -> ok() )
  {
    player_t::invalidate_cache( CACHE_BLOCK );
    player_t::invalidate_cache( CACHE_CRIT_BLOCK );
    player_t::invalidate_cache( CACHE_ATTACK_POWER );
  }
  if ( c == CACHE_MASTERY && mastery.unshackled_fury -> ok() )
    player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  if ( c == CACHE_BONUS_ARMOR && spec.bladed_armor -> ok() )
    player_t::invalidate_cache( CACHE_ATTACK_POWER );
}

// warrior_t::primary_role() ================================================

role_e warrior_t::primary_role() const
{
  // For now, assume "Default role"/ROLE_NONE wants to be a gladiator dps.
  if ( specialization() == WARRIOR_PROTECTION && player_t::primary_role() == ROLE_TANK )
  {
    return ROLE_TANK;
  }
  return ROLE_ATTACK;
}

// warrior_t::convert_hybrid_stat ==============================================

stat_e warrior_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_AGI_INT:
    return STAT_NONE;
  case STAT_STR_AGI_INT:
  case STAT_STR_AGI:
  case STAT_STR_INT:
    return STAT_STRENGTH;
  case STAT_SPIRIT:
    return STAT_NONE;
  case STAT_BONUS_ARMOR:
    if ( specialization() == WARRIOR_PROTECTION )
      return s;
    else
      return STAT_NONE;
  default: return s;
  }
}

// warrior_t::assess_damage =================================================

void warrior_t::assess_damage( school_e school,
                               dmg_e    dtype,
                               action_state_t* s )
{
  if ( s -> result == RESULT_HIT ||
       s -> result == RESULT_CRIT ||
       s -> result == RESULT_GLANCE )
  {
    if ( active.stance == STANCE_DEFENSE )
      s -> result_amount *= 1.0 + ( buff.defensive_stance -> data().effectN( 1 ).percent() +
      talents.gladiators_resolve -> effectN( 2 ).percent() );

    warrior_td_t* td = get_target_data( s -> action -> player );

    if ( td -> debuffs_demoralizing_shout -> up() )
      s -> result_amount *= 1.0 + td -> debuffs_demoralizing_shout -> value();

    //take care of dmg reduction CDs
    if ( buff.shield_wall -> up() )
      s -> result_amount *= 1.0 + buff.shield_wall -> value();

    if ( buff.die_by_the_sword -> up() )
      s -> result_amount *= 1.0 + buff.die_by_the_sword -> default_value;

    if ( s -> block_result == BLOCK_RESULT_CRIT_BLOCKED )
    {
      if ( cooldown.rage_from_crit_block -> up() )
      {
        cooldown.rage_from_crit_block -> start();
        resource_gain( RESOURCE_RAGE,
                       buff.enrage -> data().effectN( 1 ).resource( RESOURCE_RAGE ),
                       gain.critical_block );
        buff.enrage -> trigger();
        buff.enraged_speed -> trigger();
      }
    }
  }

  if ( ( s -> result == RESULT_DODGE || s -> result == RESULT_PARRY ) && !s -> action -> is_aoe() ) // AoE attacks do not reset revenge.
    cooldown.revenge -> reset( true );

  if ( s -> result == RESULT_PARRY && buff.die_by_the_sword -> up() && glyphs.drawn_sword -> ok() )
  {
    player_t::resource_gain( RESOURCE_RAGE,
                             glyphs.drawn_sword -> effectN( 1 ).resource( RESOURCE_RAGE ),
                             gain.drawn_sword_glyph );
  }

  player_t::assess_damage( school, dtype, s );

  if ( ( s -> result == RESULT_HIT || s -> result == RESULT_CRIT || s -> result == RESULT_GLANCE ) && buff.tier16_reckless_defense -> up() )
  {
    player_t::resource_gain( RESOURCE_RAGE,
                             floor( s -> result_amount / resources.max[RESOURCE_HEALTH] * 100 ),
                             gain.tier16_4pc_tank );
  }

  if ( active.t16_2pc )
  {
    if ( s -> block_result != BLOCK_RESULT_UNBLOCKED ) //heal if blocked
    {
      double heal_amount = floor( s -> blocked_amount * sets.set( SET_TANK, T16, B2 ) -> effectN( 1 ).percent() );
      active.t16_2pc -> base_dd_min = active.t16_2pc -> base_dd_max = heal_amount;
      active.t16_2pc -> execute();
    }

    if ( s -> self_absorb_amount > 0 ) //always heal if shield_barrier absorbed it. This assumes that shield_barrier is our only own absorb spell.
    {
      double heal_amount = floor( s -> self_absorb_amount * sets.set( SET_TANK, T16, B2 ) -> effectN( 2 ).percent() );
      active.t16_2pc -> base_dd_min = active.t16_2pc -> base_dd_max = heal_amount;
      active.t16_2pc -> execute();
    }
  }
}

// warrior_t::create_options ================================================

void warrior_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "initial_rage", initial_rage ) );
  add_option( opt_bool( "warrior_fixed_time", warrior_fixed_time ) );
  add_option( opt_bool( "control_stance_swapping", player_override_stance_dance ) );
}

// Mark of the Shattered Hand ===============================================

struct warrior_shattered_bleed_t: public warrior_attack_t
{
  warrior_shattered_bleed_t( warrior_t* p ):
    warrior_attack_t( "shattered_bleed", p, p -> find_spell( 159238 ) )
  {
    dot_behavior = DOT_REFRESH;
    may_miss = may_block = may_dodge = may_parry = callbacks = hasted_ticks = false;
    may_crit = background = special = true;
    tick_may_crit = false;
  }

  double target_armor( player_t* ) const
  {
    return 0.0;
  }

  timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const
  {
    timespan_t new_duration = std::min( triggered_duration * 0.3, dot -> remains() ) + triggered_duration;
    timespan_t period_mod = new_duration % base_tick_time;
    new_duration += (base_tick_time - period_mod);

    return new_duration;
  }
};

// warrior_t::create_proc_action =============================================

action_t* warrior_t::create_proc_action( const std::string& name )
{
  if ( name == "shattered_bleed" ) return new warrior_shattered_bleed_t(this);

  return 0;
}

// warrior_t::create_profile ================================================

bool warrior_t::create_profile( std::string& profile_str, save_e type, bool save_html )
{
  if ( specialization() == WARRIOR_PROTECTION && primary_role() == ROLE_TANK )
    position_str = "front";

  return player_t::create_profile( profile_str, type, save_html );
}

// warrior_t::copy_from =====================================================

void warrior_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warrior_t* p = debug_cast<warrior_t*>( source );

  initial_rage = p -> initial_rage;
  warrior_fixed_time = p -> warrior_fixed_time;
  player_override_stance_dance = p -> player_override_stance_dance;
}

// warrior_t::stance_swap ==================================================

void warrior_t::stance_swap()
{
  // Blizzard has automated stance swapping with defensive and battle stance. This function will swap to the stance automatically if
  // The ability that we are trying to use is not usable in the current stance.
  if ( talents.gladiators_resolve -> ok() && in_combat ) // Cannot swap stances with gladiator's resolve in combat.
    return;

  warrior_stance swap;

  switch ( active.stance )
  {
  case STANCE_BATTLE:
  {
    buff.battle_stance -> expire();
    swap = STANCE_DEFENSE;
    break;
  }
  case STANCE_DEFENSE:
  {
    buff.defensive_stance -> expire();
    swap = STANCE_BATTLE;
    recalculate_resource_max( RESOURCE_HEALTH );
    break;
  }
  case STANCE_GLADIATOR:
  {
    buff.gladiator_stance -> expire();
    swap = STANCE_GLADIATOR;
    break;
  }
  default: swap = active.stance; break;
  }

  switch ( swap )
  {
  case STANCE_BATTLE: buff.battle_stance -> trigger(); break;
  case STANCE_DEFENSE:
  {
    buff.defensive_stance -> trigger();
    recalculate_resource_max( RESOURCE_HEALTH );
    break;
  }
  case STANCE_GLADIATOR: buff.gladiator_stance -> trigger(); break;
  }
  if ( in_combat )
  {
    cooldown.stance_swap -> start();
    cooldown.stance_swap -> adjust( -1 * cooldown.stance_swap -> duration * ( 1.0 - cache.attack_haste() ) ); // Yeah.... it's hasted by headlong rush.
    if ( !player_override_stance_dance )
    {
      stance_dance = true; // Invalidate stance dancing, so that the simulation will try and swap back.
    } // Only if the user doesn't override it. If overridden, the player will have to force it back to the original stance.
  }
}

// warrior_t::enrage ========================================================

void warrior_t::enrage()
{
  // Crit BT/Devastate/Block give rage, and refresh enrage
  // Additionally, BT crits grant 1 charge of Raging Blow

  if ( specialization() == WARRIOR_FURY )
    buff.raging_blow -> trigger();

  resource_gain( RESOURCE_RAGE,
                 buff.enrage -> data().effectN( 1 ).resource( RESOURCE_RAGE ),
                 gain.enrage );
  buff.enrage -> trigger();
  buff.enraged_speed -> trigger();
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warrior_report_t: public player_report_extension_t
{
public:
  warrior_report_t( warrior_t& player ):
    p( player )
  {}

  virtual void html_customsection( report::sc_html_stream& /*os*/ ) override
  {
    (void)p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  warrior_t& p;
};

// WARRIOR MODULE INTERFACE =================================================

struct warrior_module_t: public module_t
{
  warrior_module_t(): module_t( WARRIOR ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    warrior_t* p = new warrior_t( sim, name, r );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new warrior_report_t( *p ) );
    return p;
  }

  virtual bool valid() const { return true; }

  virtual void init( sim_t* /* sim */ ) const
  {
    /*
    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
    player_t* p = sim -> actor_list[i];
    }*/
  }

  virtual void combat_begin( sim_t* ) const {}

  virtual void combat_end( sim_t* ) const {}
};
} // UNNAMED NAMESPACE

const module_t* module_t::warrior()
{
  static warrior_module_t m;
  return &m;
}
