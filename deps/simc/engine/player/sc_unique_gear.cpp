// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

using namespace unique_gear;

#define maintenance_check( ilvl ) static_assert( ilvl >= 372, "unique item below min level, should be deprecated." )

namespace { // UNNAMED NAMESPACE

/**
 * Forward declarations so we can reorganize the file a bit more sanely.
 */

namespace enchant
{
  void mark_of_bleeding_hollow( special_effect_t& );
  void megawatt_filament( special_effect_t& );
  void oglethorpes_missile_splitter( special_effect_t& );
  void hemets_heartseeker( special_effect_t& );
  void mark_of_the_thunderlord( special_effect_t& );
  void mark_of_the_shattered_hand( special_effect_t& );
  void mark_of_the_frostwolf( special_effect_t& );
  void mark_of_shadowmoon( special_effect_t& );
  void mark_of_blackrock( special_effect_t& );
  void mark_of_warsong( special_effect_t& );
  void dancing_steel( special_effect_t& );
  void jade_spirit( special_effect_t& );
  void windsong( special_effect_t& );
  void rivers_song( special_effect_t& );
  void colossus( special_effect_t& );
  void executioner( special_effect_t& );
  void hurricane_spell( special_effect_t& );
}

namespace profession
{
  void nitro_boosts( special_effect_t& );
  void zen_alchemist_stone( special_effect_t& );
  void draenor_philosophers_stone( special_effect_t& );
}

namespace item
{
  void flurry_of_xuen( special_effect_t& );
  void essence_of_yulon( special_effect_t& );
  void endurance_of_niuzao( special_effect_t& );
  void skeers_bloodsoaked_talisman( special_effect_t& );
  void black_blood_of_yshaarj( special_effect_t& );
  void rune_of_reorigination( special_effect_t& );
  void spark_of_zandalar( special_effect_t& );
  void unerring_vision_of_leishen( special_effect_t& );
  void readiness( special_effect_t& );
  void amplification( special_effect_t& );
  void prismatic_prison_of_pride( special_effect_t& );
  void purified_bindings_of_immerseus( special_effect_t& );
  void thoks_tail_tip( special_effect_t& );
  void cleave( special_effect_t& );
  void heartpierce( special_effect_t& );

  /* Warlards of Draenor 6.0 */
  void blackiron_micro_crucible( special_effect_t& );
  void humming_blackiron_trigger( special_effect_t& );
  void battering_talisman_trigger( special_effect_t& );
  void forgemasters_insignia( special_effect_t& );
  void autorepairing_autoclave( special_effect_t& );
  void spellbound_runic_band( special_effect_t& );
  void spellbound_solium_band( special_effect_t& );
}

namespace gem
{
  void sinister_primal( special_effect_t& );
  void indomitable_primal( special_effect_t& );
  void capacitive_primal( special_effect_t& );
  void courageous_primal( special_effect_t& );
}

namespace set_bonus
{
  void t17_lfr_passive_stat( special_effect_t& );
  void t17_lfr_4pc_agimelee( special_effect_t& );
  void t17_lfr_4pc_leamelee( special_effect_t& );
  void t17_lfr_4pc_leacaster( special_effect_t& );
  void t17_lfr_4pc_mailcaster( special_effect_t& );
  void t17_lfr_4pc_platemelee( special_effect_t& );
  void t17_lfr_4pc_clothcaster( special_effect_t& );
}

/**
 * Select attribute operator for buffs. Selects the attribute based on the
 * comparator given (std::greater for example), based on all defined attributes
 * that the stat buff is using. Note that this is for _ATTRIBUTES_ only. Using
 * it for any kind of stats will not work (for now).
 *
 * TODO: Generic way to get "composite" stat_e, so we can extend this class to
 * work on all Blizzard stats.
 */
template<template<typename> class CMP>
struct select_attr
{
  CMP<double> comparator;

  bool operator()( const stat_buff_t& buff ) const
  {
    // Comparing to 0 isn't exactly "correct", however the odds of an actor
    // having zero for all primary attributes is slim to none. If for some
    // reason all checked attributes are 0, the last checked attribute will be
    // the one selected. The order of stats checked is determined by the
    // stat_buff_creator add_stats() calls.
    double compare_to = 0;
    stat_e compare_stat = STAT_NONE, my_stat = STAT_NONE;

    for ( size_t i = 0, end = buff.stats.size(); i < end; i++ )
    {
      if ( this == buff.stats[ i ].check_func.target<select_attr<CMP> >() )
        my_stat = buff.stats[ i ].stat;

      attribute_e stat = static_cast<attribute_e>( buff.stats[ i ].stat );
      double val = buff.player -> get_attribute( stat );
      if ( ! compare_to || comparator( val, compare_to ) )
      {
        compare_to = val;
        compare_stat = buff.stats[ i ].stat;
      }
    }

    return compare_stat == my_stat;
  }
};

std::string suffix( const item_t* item )
{
  assert( item );
  if ( item -> slot == SLOT_OFF_HAND )
    return "_oh";
  return "";
}

std::string tokenized_name( const spell_data_t* data )
{
  std::string s = data -> name_cstr();
  util::tokenize( s );
  return s;
}

/**
 * Master list of special effects in Simulationcraft.
 *
 * This list currently contains custom procs and procs where game client data
 * is either incorrect (so we can override values), or incomplete (so we can
 * help the automatic creation process on the simc side).
 *
 * Each line in the array corresponds to a specific spell (a proc driver spell,
 * or an "on use" spell) in World of Warcraft. There are several sources for
 * special effects:
 * 1) Items (Use, Equip, Chance on hit)
 * 2) Enchants, and profession specific enchants
 * 3) Engineering special effects (tinkers, ranged enchants)
 * 4) Gems
 * 
 * Blizzard does not discriminate between the different types, nor do we
 * anymore. Each spell can be mapped to a special effect in the simc client.
 * Each special effect is fed to a new proc callback object
 * (dbc_proc_callback_t) that handles the initialization of the proc, and in
 * generic proc cases, the initialization of the buffs/actions.
 *
 * Each entry contains three fields:
 * 1) The spell ID of the effect. You can find these from third party websites
 *    by clicking on the generated link in item tooltip.
 * 2) Currently a c-string of "additional options" given for a special effect.
 *    This includes the forementioned fixes of incorrect values, and "help" to
 *    drive the automatic special effect generation process. Case insensitive.
 * 3) A callback to a custom initialization function. The function is of the
 *    form: void custom_function_of_awesome( special_effect_t& effect,
 *                                           const item_t& item,
 *                                           const special_effect_db_item_t& dbitem )
 *    Where 'effect' is the effect being created, 'item' is the item that has 
 *    the special effect, and 'dbitem' is the database entry itself.
 *
 * Now, special effect creation in this new system is currently a two phase
 * process. First, the special_effect_t struct for the special effect is filled
 * with enough information to initialize the proc (for generic procs, a driver
 * spell id is sufficient), and any options given in this list (through the
 * additional options). For custom special effects, the first phase simply
 * creates a stub special_effect_t object, and no game client data is processed
 * at this time.
 *
 * The second phase of the creation process is responsible for instantiating
 * the necessary action_callback_t object (simc procs), and whatever buffs, or
 * actions are required for the proc. This is also when custom callbacks get
 * called.
 *
 * Note: The special effect initialization process is now unified for all types
 * of special effects, we no longer discriminate between item, enchant, tinker,
 * or gem based special effects.
 *
 * Note2: Enchants, addons, and possibly gems will have a separate translation
 * table in sc_enchant.cpp that maps "user given" names of enchants
 * (enchant=dancing_steel), to in game data, so we can properly initialize the
 * correct spells here. Most of the enchants etc., are automatically
 * identified.  The table will only have the "non standard" user strings we
 * currently use, and whatever else we will use in the future.
 */
static const special_effect_db_item_t __special_effect_db[] = {
  /**
   * TRINKET-PROC-TYPE-TODO:
   * - Alacrity of Xuen (procs on damage or landing an ability?)
   * - Evil Eye of Galakras (damage/landing?)
   * - Kadris Toxic Totem (damage/landing?)
   * - Frenzied Crystal of Rage (damage/landing?)
   * - Fusion-Fire Core (damage/landing?)
   * - Talisman of Bloodlust (damage/landing?)
   * - Primordius' Talisman of Rage (damage/landing?)
   * - Fabled Feather of Ji-Kun (damage/landing?)
   */

  /**
   * Items
   */

  /* Warlords of Draenor 6.0 */
  { 177085, 0,                     item::blackiron_micro_crucible }, /* Blackiron Micro Crucible */
  { 177071, 0,                    item::humming_blackiron_trigger }, /* Humming Blackiron Trigger */
  { 177104, 0,                   item::battering_talisman_trigger }, /* Battering Talisman Trigger */
  { 177098, 0,                        item::forgemasters_insignia }, /* Forgemaster's Insignia */
  { 177090, 0,                      item::autorepairing_autoclave }, /* Forgemaster's Insignia */
  { 177171, 0,                        item::spellbound_runic_band }, /* 700 ilevel proc-ring */
  { 177163, 0,                       item::spellbound_solium_band }, /* 690 ilevel proc-ring */

  /* Mists of Pandaria: 5.4 */
  { 146195, 0,                               item::flurry_of_xuen }, /* Melee legendary cloak */
  { 146197, 0,                             item::essence_of_yulon }, /* Caster legendary cloak */
  { 146193, 0,                          item::endurance_of_niuzao }, /* Tank legendary cloak */

  { 146219, "ProcOn/Hit",                                       0 }, /* Yu'lon's Bite */
  { 146251, "ProcOn/Hit",                                       0 }, /* Thok's Tail Tip (Str proc) */

  { 145955, 0,                                    item::readiness },
  { 146019, 0,                                    item::readiness },
  { 146025, 0,                                    item::readiness },
  { 146051, 0,                                item::amplification },
  { 146136, 0,                                       item::cleave },

  { 146183, 0,                       item::black_blood_of_yshaarj }, /* Black Blood of Y'Shaarj */
  { 146286, 0,                  item::skeers_bloodsoaked_talisman }, /* Skeer's Bloodsoaked Talisman */
  { 146315, 0,                    item::prismatic_prison_of_pride }, /* Prismatic Prison needs role-specific disable */
  { 146047, 0,               item::purified_bindings_of_immerseus }, /* Purified Bindings needs role-specific disable */
  { 146251, 0,                               item::thoks_tail_tip }, /* Thok's Tail Tip needs role-specific disable */

  /* Mists of Pandaria: 5.2 */
  { 139116, 0,                        item::rune_of_reorigination }, /* Rune of Reorigination */
  { 138957, 0,                            item::spark_of_zandalar }, /* Spark of Zandalar */
  { 138964, 0,                   item::unerring_vision_of_leishen }, /* Unerring Vision of Lei Shen */

  { 138728, "Reverse",                                          0 }, /* Steadfast Talisman of the Shado-Pan Assault */
  { 139171, "ProcOn/Crit_RPPMAttackCrit",                       0 }, /* Gaze of the Twins */
  { 138757, "1Tick_138737Trigger",                              0 }, /* Renataki's Soul Charm */
  { 138790, "ProcOn/Hit_1Tick_138788Trigger",                   0 }, /* Wushoolay's Final Choice */
  { 138758, "1Tick_138760Trigger",                              0 }, /* Fabled Feather of Ji-Kun */
  { 139134, "ProcOn/Crit_RPPMSpellCrit",                        0 }, /* Cha-Ye's Essence of Brilliance */

  { 138865, "ProcOn/Dodge",                                     0 }, /* Delicate Vial of the Sanguinaire */
  // TODO: Ji-Kun's rising winds (heal action support)
  // TODO: Soul barrier (absorb with individual absorb event cap)
  // TODO: Rook's unlucky Talisman (aoe damage reduction)

  /* Mists of Pandaria: 5.0 */
  { 126650, "ProcOn/Hit",                                       0 }, /* Terror in the Mists */
  { 126658, "ProcOn/Hit",                                       0 }, /* Darkmist Vortex */

  /* Mists of Pandaria: Dungeon */
  { 126473, "ProcOn/Hit",                                       0 }, /* Vision of the Predator */
  { 126516, "ProcOn/Hit",                                       0 }, /* Carbonic Carbuncle */
  { 126482, "ProcOn/Hit",                                       0 }, /* Windswept Pages */
  { 126490, "ProcOn/Crit",                                      0 }, /* Searing Words */

  /* Mists of Pandaria: Player versus Player */
  { 138701, "ProcOn/Hit",                                       0 }, /* Brutal Talisman of the Shado-Pan Assault */
  { 138700, "ProcOn/Hit",                                       0 }, /* Vicious Talisman of the Shado-Pan Assault */

  { 126706, "ProcOn/Hit",                                       0 }, /* Gladiator's Insignia of Dominance */

  /* Mists of Pandaria: Darkmoon Faire */
  { 128990, "ProcOn/Hit",                                       0 }, /* Relic of Yu'lon */
  { 128445, "ProcOn/Crit",                                      0 }, /* Relic of Xuen (agi) */

  // Misc
  { 71892,  0,                                  item::heartpierce },
  { 71880,  0,                                  item::heartpierce },

  /**
   * Enchants
   */

  /* Warlords of Draenor */
  { 159239, 0,                enchant::mark_of_the_shattered_hand },
  { 159243, 0,                   enchant::mark_of_the_thunderlord },
  { 159682, 0,                           enchant::mark_of_warsong },
  { 159683, 0,                     enchant::mark_of_the_frostwolf },
  { 159684, 0,                        enchant::mark_of_shadowmoon },
  { 159685, 0,                         enchant::mark_of_blackrock },
  { 156059, 0,                         enchant::megawatt_filament },
  { 156052, 0,              enchant::oglethorpes_missile_splitter },
  { 173286, 0,                        enchant::hemets_heartseeker },
  { 173321, 0,                   enchant::mark_of_bleeding_hollow },

  /* Mists of Pandaria */
  { 118333, 0,                             enchant::dancing_steel },
  { 142531, 0,                             enchant::dancing_steel }, /* Bloody Dancing Steel */
  { 120033, 0,                               enchant::jade_spirit },
  { 141178, 0,                               enchant::jade_spirit },
  { 104561, 0,                                  enchant::windsong },
  { 104428, "rppmhaste",                                        0 }, /* Elemental Force */
  { 104441, 0,                               enchant::rivers_song },
  { 118314, 0,                                  enchant::colossus },

  /* Cataclysm */
  {  94747, 0,                           enchant::hurricane_spell },
  {  74221, "1PPM",                                             0 }, /* Hurricane Weapon */
  {  74245, "1PPM",                                             0 }, /* Landslide */
  {  94746, 0,                                                  0 }, /* Power Torrent */

  /* Wrath of the Lich King */
  {  59620, "1PPM",                                             0 }, /* Berserking */
  {  42976, 0,                               enchant::executioner },

  /* The Burning Crusade */
  {  28093, "1PPM",                                             0 }, /* Mongoose */

  /* Engineering enchants */
  { 177708, "1PPM_109092Trigger",                               0 }, /* Mirror Scope */
  { 177707, "1PPM_109085Trigger",                               0 }, /* Lord Blastingtons Scope of Doom */
  {  95713, "1PPM_95712Trigger",                                0 }, /* Gnomish XRay */
  {  99622, "1PPM_99621Trigger",                                0 }, /* Flintlocks Woodchucker */

  /* Profession perks */
  { 105574, 0,                    profession::zen_alchemist_stone }, /* Zen Alchemist Stone (stat proc) */
  { 157136, 0,             profession::draenor_philosophers_stone }, /* Draenor Philosopher's Stone (stat proc) */
  {  55004, 0,                           profession::nitro_boosts },

  /**
   * Gems
   */

  {  39958, "0.2PPM", 0 }, /* Thundering Skyfire */ // TODO: check why 20% PPM and not 100% from spell data?
  {  55380, "0.2PPM", 0 }, /* Thundering Skyfire */ /* Can use same callback for both */ // TODO: check why 20% PPM and not 100% from spell data?
  { 137592, 0,                               gem::sinister_primal }, /* Caster Legendary Gem */
  { 137594, 0,                            gem::indomitable_primal }, /* Tank Legendary Gem */
  { 137595, 0,                             gem::capacitive_primal }, /* Melee Legendary Gem */
  { 137248, 0,                             gem::courageous_primal }, /* Healer Legendary Gem */

  /* Generic special effects begin here                                      */

  /* T17 LFR set bonuses */
  { 179108, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Cloth DPS */
  { 179107, 0,                 set_bonus::t17_lfr_4pc_clothcaster },
  { 179110, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Cloth Healer */
  { 179114, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Leather Melee */
  { 179115, 0,                    set_bonus::t17_lfr_4pc_leamelee },
  { 179117, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Leather Caster */
  { 179118, 0,                   set_bonus::t17_lfr_4pc_leacaster },
  { 179121, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Leather Healer */
  { 179127, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Leather Tank */
  { 179130, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Mail Agility */
  { 179131, 0,                    set_bonus::t17_lfr_4pc_agimelee },
  { 179133, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Mail Caster */
  { 179134, 0,                  set_bonus::t17_lfr_4pc_mailcaster },
  { 179137, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Mail Healer */
  { 179139, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Plate Melee */
  { 179140, 0,                  set_bonus::t17_lfr_4pc_platemelee },
  { 179142, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Plate Tank */
  { 179145, 0,                    set_bonus::t17_lfr_passive_stat }, /* 2P Plate Healer */

  /* Always NULL terminate, last entry will be used as a "not found" value */
  {      0, 0,                                                  0 }
};


// Enchants ================================================================

void enchant::mark_of_bleeding_hollow( special_effect_t& effect )
{
  // Custom callback to help the special effect initialization, we can use
  // generic initialization for the enchant, but the game client data does not
  // link driver to the procced spell, so we do it here.

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.trigger_spell_id = 173322;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchant::megawatt_filament( special_effect_t& effect )
{
  // Custom callback to help the special effect initialization, we can use
  // generic initialization for the enchant, but the game client data does not
  // link driver to the procced spell, so we do it here.

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.trigger_spell_id = 156060;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchant::oglethorpes_missile_splitter( special_effect_t& effect )
{
  // Custom callback to help the special effect initialization, we can use
  // generic initialization for the enchant, but the game client data does not
  // link driver to the procced spell, so we do it here.

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.trigger_spell_id = 156055;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchant::hemets_heartseeker( special_effect_t& effect )
{
  // Custom callback to help the special effect initialization, we can use
  // generic initialization for the enchant, but the game client data does not
  // link driver to the procced spell, so we do it here.

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.trigger_spell_id = 173288;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchant::mark_of_shadowmoon( special_effect_t& effect )
{
  effect.type = SPECIAL_EFFECT_EQUIP;

  struct mos_proc_callback_t : public dbc_proc_callback_t
  {
    mos_proc_callback_t( const item_t* i, const special_effect_t& effect ) :
      dbc_proc_callback_t( i, effect )
    { }

    void trigger( action_t* a, void* call_data )
    {
      if ( listener -> resources.pct( RESOURCE_MANA ) > 0.5 )
        return;

      dbc_proc_callback_t::trigger( a, call_data );
    }
  };

  new mos_proc_callback_t( effect.item, effect );
}

void enchant::mark_of_blackrock( special_effect_t& effect )
{
  effect.type = SPECIAL_EFFECT_EQUIP;

  struct mob_proc_callback_t : public dbc_proc_callback_t
  {
    mob_proc_callback_t( const item_t* i, const special_effect_t& effect ) :
      dbc_proc_callback_t( i, effect )
    { }

    void trigger( action_t* a, void* call_data )
    {
      if ( listener -> resources.pct( RESOURCE_HEALTH ) >= 0.6 )
        return;

      dbc_proc_callback_t::trigger( a, call_data );
    }
  };

  new mob_proc_callback_t( effect.item, effect );
}

void enchant::mark_of_warsong( special_effect_t& effect )
{
  // Custom callback to help the special effect initialization, we can use
  // generic initialization for the enchant, but the game client data does not
  // link driver to the procced spell, so we do it here.

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.trigger_spell_id = 159675;
  effect.reverse = true;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchant::mark_of_the_thunderlord( special_effect_t& effect )
{
  struct mott_buff_t : public stat_buff_t
  {
    unsigned extensions;
    unsigned max_extensions;

    mott_buff_t( const item_t* item, const std::string& name, unsigned max_ext ) :
      stat_buff_t( stat_buff_creator_t( item -> player, name, item -> player -> find_spell( 159234 ) ) ),
      extensions( 0 ), max_extensions( max_ext )
    { }

    void extend_duration( player_t* p, timespan_t extend_duration )
    {
      if ( extensions < max_extensions )
      {
        stat_buff_t::extend_duration( p, extend_duration );
        extensions++;
      }
    }

    void execute( int stacks, double value, timespan_t duration )
    { stat_buff_t::execute( stacks, value, duration ); extensions = 0; }

    void reset()
    { stat_buff_t::reset(); extensions = 0; }

    void expire_override( int expiration_stacks, timespan_t remaining_duration )
    { stat_buff_t::expire_override( expiration_stacks, remaining_duration ); extensions = 0; }
  };

  // Max extensions is hardcoded, no spell data to fetch it
  effect.custom_buff = new mott_buff_t( effect.item, effect.name(), 3 );

  // Setup another proc callback, that uses the same driver as the proc that
  // triggers the buff, however it only procs on crits. This callback will
  // extend the duration of the buff, only if the buff is up. The extension
  // capping is handled in the buff itself.
  special_effect_t* effect2 = new special_effect_t( effect.item );
  effect2 -> name_str = effect.name() + "_crit_driver";
  effect2 -> proc_chance_ = 1;
  effect2 -> ppm_ = 0;
  effect2 -> spell_id = effect.spell_id;
  effect2 -> custom_buff = effect.custom_buff;
  effect2 -> cooldown_ = timespan_t::zero();
  effect2 -> proc_flags2_ = PF2_CRIT;

  effect.item -> player -> special_effects.push_back( effect2 );

  struct mott_crit_callback_t : public dbc_proc_callback_t
  {
    mott_crit_callback_t( const item_t* item, const special_effect_t& effect ) :
      dbc_proc_callback_t( item, effect )
    { }

    void execute( action_t*, action_state_t* )
    {
      if ( proc_buff -> check() )
        proc_buff -> extend_duration( listener, timespan_t::from_seconds( 2 ) );
    }
  };

  new dbc_proc_callback_t( effect.item, effect );
  new mott_crit_callback_t( effect.item, *effect.item -> player -> special_effects.back() );
}

void enchant::mark_of_the_frostwolf( special_effect_t& effect )
{
  // Custom callback to help the special effect initialization, we can use
  // generic initialization for the enchant, but the game client data does not
  // link driver to the procced spell, so we do it here.

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.trigger_spell_id = 159676;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchant::mark_of_the_shattered_hand( special_effect_t& effect )
{
  effect.rppm_scale = RPPM_HASTE;
  effect.trigger_spell_id = 159238;
  effect.name_str = effect.item -> player -> find_spell( 159238 ) -> name_cstr();

  struct bleed_attack_t : public attack_t
  {
    bleed_attack_t( player_t* p, const special_effect_t& effect ) :
      attack_t( effect.name(), p, p -> find_spell( effect.trigger_spell_id ) )
    {
      dot_behavior = DOT_REFRESH;
      hasted_ticks = false; background = true; callbacks = false; special = true;
      may_miss = may_block = may_dodge = may_parry = false; may_crit = true;
      tick_may_crit = false;
    }

    double target_armor( player_t* ) const
    { return 0.0; }
  };

  action_t* bleed = effect.item -> player -> create_proc_action( "shattered_bleed" );
  if ( ! bleed )
    bleed = new bleed_attack_t( effect.item -> player, effect );

  effect.execute_action = bleed;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchant::colossus( special_effect_t& effect )
{
  const spell_data_t* spell = effect.item -> player-> find_spell( 116631 );

  absorb_buff_t* buff =
      absorb_buff_creator_t( effect.item -> player,
                             tokenized_name( spell ) + suffix( effect.item ) )
      .spell( spell )
      .source( effect.item -> player -> get_stats( tokenized_name( spell ) + suffix( effect.item ) ) )
      .activated( false );

  effect.rppm_scale = RPPM_HASTE;
  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchant::rivers_song( special_effect_t& effect )
{
  const spell_data_t* spell = effect.item -> player -> find_spell( 116660 );

  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( effect.item -> player, tokenized_name( spell ) ) );

  if ( ! buff )
    buff = stat_buff_creator_t( effect.item -> player, tokenized_name( spell ), spell )
           .activated( false );

  effect.rppm_scale = RPPM_HASTE;
  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchant::dancing_steel( special_effect_t& effect )
{
  // Account for Bloody Dancing Steel and Dancing Steel buffs
  const spell_data_t* spell = effect.item -> player -> find_spell( effect.spell_id == 142531 ? 142530 : 120032 );

  double value = spell -> effectN( 1 ).average( effect.item -> player );

  stat_buff_t* buff  = stat_buff_creator_t( effect.item -> player, tokenized_name( spell ) + suffix( effect.item ), spell )
                       .activated( false )
                       .add_stat( STAT_STRENGTH, value, select_attr<std::greater>() )
                       .add_stat( STAT_AGILITY,  value, select_attr<std::greater>() );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.item, effect );
}

struct jade_spirit_check_func
{
  bool operator()( const stat_buff_t& buff )
  {
    if ( buff.player -> resources.max[ RESOURCE_MANA ] <= 0.0 )
      return false;

    return buff.player -> resources.pct( RESOURCE_MANA ) < 0.25;
  }
};

void enchant::jade_spirit( special_effect_t& effect )
{
  const spell_data_t* spell = effect.item -> player -> find_spell( 104993 );

  double int_value = spell -> effectN( 1 ).average( effect.item -> player );
  double spi_value = spell -> effectN( 2 ).average( effect.item -> player );

  // Set trigger spell here, so special_effect_t::name() returns a pretty name
  // for the custom buff.
  effect.trigger_spell_id = 104993;

  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( effect.item -> player, effect.name() ) );
  if ( ! buff )
    buff = stat_buff_creator_t( effect.item -> player, effect.name(), spell )
           .activated( false )
           .add_stat( STAT_INTELLECT, int_value )
           .add_stat( STAT_SPIRIT, spi_value, jade_spirit_check_func() );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

struct windsong_callback_t : public dbc_proc_callback_t
{
  stat_buff_t* haste, *crit, *mastery;

  windsong_callback_t( const item_t* i,
                       const special_effect_t& effect,
                       stat_buff_t* hb,
                       stat_buff_t* cb,
                       stat_buff_t* mb ) :
                 dbc_proc_callback_t( i, effect ),
    haste( hb ), crit( cb ), mastery( mb )
  { }

  void execute( action_t* /* a */, action_state_t* /* call_data */ )
  {
    stat_buff_t* buff;

    int p_type = ( int ) ( listener -> sim -> rng().real() * 3.0 );
    switch ( p_type )
    {
      case 0: buff = haste; break;
      case 1: buff = crit; break;
      case 2:
      default:
        buff = mastery; break;
    }

    buff -> trigger();
  }
};

void enchant::windsong( special_effect_t& effect )
{
  const spell_data_t* mastery = effect.item -> player -> find_spell( 104510 );
  const spell_data_t* haste = effect.item -> player -> find_spell( 104423 );
  const spell_data_t* crit = effect.item -> player -> find_spell( 104509 );

  stat_buff_t* mastery_buff = stat_buff_creator_t( effect.item -> player, "windsong_mastery" + suffix( effect.item ), mastery )
                              .activated( false );
  stat_buff_t* haste_buff   = stat_buff_creator_t( effect.item -> player, "windsong_haste" + suffix( effect.item ), haste )
                              .activated( false );
  stat_buff_t* crit_buff    = stat_buff_creator_t( effect.item -> player, "windsong_crit" + suffix( effect.item ), crit )
                              .activated( false );

  //effect.name_str = tokenized_name( mastery ) + suffix( item );

  new windsong_callback_t( effect.item, effect, haste_buff, crit_buff, mastery_buff );
}

struct hurricane_spell_proc_t : public dbc_proc_callback_t
{
  buff_t *mh_buff, *oh_buff, *s_buff;

  hurricane_spell_proc_t( const special_effect_t& effect, buff_t* mhb, buff_t* ohb, buff_t* sb ) :
    dbc_proc_callback_t( effect.item -> player, effect ),
    mh_buff( mhb ), oh_buff( ohb ), s_buff( sb )
  { }

  void execute( action_t* /* a */, action_state_t* /* call_data */ )
  {
    if ( mh_buff && mh_buff -> check() )
      mh_buff -> trigger();
    else if ( oh_buff && oh_buff -> check() )
      oh_buff -> trigger();
    else
      s_buff -> trigger();
  }
};

void enchant::hurricane_spell( special_effect_t& effect )
{
  int n_hurricane_enchants = 0;

  if ( effect.item -> player -> items[ SLOT_MAIN_HAND ].parsed.enchant_id == 4083 ||
       util::str_compare_ci( effect.item -> player -> items[ SLOT_MAIN_HAND ].option_enchant_str, "hurricane" ) )
    n_hurricane_enchants++;

  if ( effect.item -> player -> items[ SLOT_OFF_HAND ].parsed.enchant_id == 4083 ||
       util::str_compare_ci( effect.item -> player -> items[ SLOT_OFF_HAND ].option_enchant_str, "hurricane" ) )
    n_hurricane_enchants++;

  buff_t* mh_buff = buff_t::find( effect.item -> player, "hurricane" );
  buff_t* oh_buff = buff_t::find( effect.item -> player, "hurricane_oh" );

  // If we have 2 hurricane enchants, and we're creating the first one
  // (opposite hand weapon buff has not been created), bail out early.  Note
  // that this presumes that the spell item enchant has the procs spell ids in
  // correct order (which they are, at the moment).
  if ( n_hurricane_enchants == 2 && ( ! mh_buff || ! oh_buff ) )
    return;

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();
  stat_buff_t* spell_buff = stat_buff_creator_t( effect.item -> player, "hurricane_spell", spell )
                            .activated( false );

  new hurricane_spell_proc_t( effect, mh_buff, oh_buff, spell_buff );

}

void enchant::executioner( special_effect_t& effect )
{
  const spell_data_t* spell = effect.item -> player -> find_spell( effect.spell_id );
  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( effect.item -> player, tokenized_name( spell ) ) );

  if ( ! buff )
    buff = stat_buff_creator_t( effect.item -> player, tokenized_name( spell ), spell )
           .activated( false );

  effect.name_str = tokenized_name( spell );
  effect.ppm_ = 1.0;

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.item, effect );
}

// Profession perks =========================================================

struct nitro_boosts_action_t : public action_t
{
  buff_t* buff;

  nitro_boosts_action_t( player_t* p ) :
    action_t( ACTION_USE, "nitro_boosts", p )
  {
    background = true;
    cooldown = p -> get_cooldown( "potion" );
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    player -> buffs.nitro_boosts-> trigger();
  }

  bool ready()
  {
    if ( ! player -> buffs.raid_movement -> check() )
      return false;

    return action_t::ready();
  }
};

void profession::nitro_boosts( special_effect_t& effect )
{
  effect.type = SPECIAL_EFFECT_USE;
  effect.execute_action = new nitro_boosts_action_t( effect.item -> player );
};

void profession::zen_alchemist_stone( special_effect_t& effect )
{
  struct zen_alchemist_stone_callback : public dbc_proc_callback_t
  {
    stat_buff_t* buff_str;
    stat_buff_t* buff_agi;
    stat_buff_t* buff_int;

    zen_alchemist_stone_callback( const item_t* i, const special_effect_t& data ) :
      dbc_proc_callback_t( i -> player, data )
    {
      const spell_data_t* spell = listener -> find_spell( 105574 );

      struct common_buff_creator : public stat_buff_creator_t
      {
        common_buff_creator( player_t* p, const std::string& n, const spell_data_t* spell ) :
          stat_buff_creator_t ( p, "zen_alchemist_stone_" + n, spell  )
        {
          duration( p -> find_spell( 60229 ) -> duration() );
          chance( 1.0 );
          activated( false );
        }
      };

      double value = spell -> effectN( 1 ).average( *i );

      buff_str = common_buff_creator( listener, "str", spell )
                 .add_stat( STAT_STRENGTH, value );
      buff_agi = common_buff_creator( listener, "agi", spell )
                 .add_stat( STAT_AGILITY, value );
      buff_int = common_buff_creator( listener, "int", spell )
                 .add_stat( STAT_INTELLECT, value );
    }

    virtual void execute( action_t* a, action_state_t* /* state */ ) override
    {
      player_t* p = a -> player;

      if ( p -> strength() > p -> agility() )
      {
        if ( p -> strength() > p -> intellect() )
          buff_str -> trigger();
        else
          buff_int -> trigger();
      }
      else if ( p -> agility() > p -> intellect() )
        buff_agi -> trigger();
      else
        buff_int -> trigger();
    }
  };

  maintenance_check( 450 );

  new zen_alchemist_stone_callback( effect.item, effect );
}

void profession::draenor_philosophers_stone( special_effect_t& effect )
{
  struct draenor_philosophers_stone_callback : public dbc_proc_callback_t
  {
    stat_buff_t* buff_str;
    stat_buff_t* buff_agi;
    stat_buff_t* buff_int;

    draenor_philosophers_stone_callback( const item_t* i, const special_effect_t& data ) :
      dbc_proc_callback_t( i -> player, data )
    {
      const spell_data_t* spell = listener -> find_spell( 157136 );

      struct common_buff_creator : public stat_buff_creator_t
      {
        common_buff_creator( player_t* p, const std::string& n, const spell_data_t* spell ) :
          stat_buff_creator_t ( p, "draenor_philosophers_stone_" + n, spell  )
        {
          duration( p -> find_spell( 60229 ) -> duration() );
          chance( 1.0 );
          activated( false );
        }
      };

      double value = spell -> effectN( 1 ).average( *i );

      buff_str = common_buff_creator( listener, "str", spell )
                 .add_stat( STAT_STRENGTH, value );
      buff_agi = common_buff_creator( listener, "agi", spell )
                 .add_stat( STAT_AGILITY, value );
      buff_int = common_buff_creator( listener, "int", spell )
                 .add_stat( STAT_INTELLECT, value );
    }

    virtual void execute( action_t* a, action_state_t* /* state */ ) override
    {
      player_t* p = a -> player;

      if ( p -> strength() > p -> agility() )
      {
        if ( p -> strength() > p -> intellect() )
          buff_str -> trigger();
        else
          buff_int -> trigger();
      }
      else if ( p -> agility() > p -> intellect() )
        buff_agi -> trigger();
      else
        buff_int -> trigger();
    }
  };

  maintenance_check( 620 );

  new draenor_philosophers_stone_callback( effect.item, effect );
}

void gem::sinister_primal( special_effect_t& effect )
{
  if ( effect.item -> sim -> challenge_mode )
    return;
  if ( effect.item -> player -> level == 100 )
    return;

  effect.custom_buff = effect.item -> player -> buffs.tempus_repit;

  new dbc_proc_callback_t( effect.item, effect );
}

void gem::indomitable_primal( special_effect_t& effect )
{
  if ( effect.item -> sim -> challenge_mode )
    return;
  if ( effect.item -> player -> level == 100 )
    return;

  effect.custom_buff = effect.item -> player -> buffs.fortitude;

  new dbc_proc_callback_t( effect.item, effect );
}

void gem::capacitive_primal( special_effect_t& effect )
{
  if ( effect.item -> sim -> challenge_mode )
    return;
  if ( effect.item -> player -> level == 100 )
    return;

  struct lightning_strike_t : public attack_t
  {
    lightning_strike_t( player_t* p ) :
      attack_t( "lightning_strike", p, p -> find_spell( 137597 ) )
    {
      may_crit = special = background = true;
      may_parry = may_dodge = false;
      proc = false;
    }
  };

  struct capacitive_primal_proc_t : public dbc_proc_callback_t
  {
    capacitive_primal_proc_t( const item_t* i, const special_effect_t& data ) :
      dbc_proc_callback_t( i, data )
    { }

    virtual void initialize() override
    {
      dbc_proc_callback_t::initialize();
      // Unfortunately the weapon-based RPPM modifiers have to be hardcoded,
      // as they will not show on the client tooltip data.
      if ( listener -> main_hand_weapon.group() != WEAPON_2H )
      {
        if ( listener -> specialization() == WARRIOR_FURY )
          rppm.set_modifier( 1.152 );
        else if ( listener -> specialization() == DEATH_KNIGHT_FROST )
          rppm.set_modifier( 1.134 );
      }
    }

    void trigger( action_t* action, void* call_data )
    {
      // Flurry of Xuen and Capacitance cannot proc Capacitance
      if ( action -> id == 147891 || action -> id == 146194 || action -> id == 137597 )
        return;

      dbc_proc_callback_t::trigger( action, call_data );
    }
  };

  player_t* p = effect.item -> player;

  // Stacking Buff
  effect.custom_buff = buff_creator_t( p, "capacitance", p -> find_spell( 137596 ) );
  effect.rppm_scale = RPPM_HASTE;

  // Execute Action
  action_t* ls = p -> create_proc_action( "lightning_strike" );
  if ( ! ls )
    ls = new lightning_strike_t( p );
  effect.execute_action = ls;

  new capacitive_primal_proc_t( effect.item, effect );
}

void gem::courageous_primal( special_effect_t& effect )
{
  if ( effect.item -> sim -> challenge_mode )
    return;

  if ( effect.item -> player -> level == 100 )
    return;

  struct courageous_primal_proc_t : public dbc_proc_callback_t
  {
    courageous_primal_proc_t( const special_effect_t& data ) :
      dbc_proc_callback_t( effect.item -> player, data )
    { }

    virtual void trigger( action_t* action, void* call_data ) override
    {
      spell_base_t* spell = debug_cast<spell_base_t*>( action );
      if ( ! spell -> procs_courageous_primal_diamond )
        return;

      dbc_proc_callback_t::trigger( action, call_data );
    }
  };

  effect.custom_buff = effect.item -> player -> buffs.courageous_primal_diamond_lucidity;

  new courageous_primal_proc_t( effect );
}

void set_bonus::t17_lfr_passive_stat( special_effect_t& effect )
{
  const spell_data_t* spell = effect.player -> find_spell( effect.spell_id );
  stat_e stat = STAT_NONE;
  // Sanity check for stat-giving aura
  if ( spell -> effectN( 1 ).subtype() != A_MOD_STAT || spell -> effectN( 1 ).subtype() == A_465 )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  if ( spell -> effectN( 1 ).subtype() == A_MOD_STAT )
  {
    if ( spell -> effectN( 1 ).misc_value1() >= 0 )
    {
      stat = static_cast< stat_e >( spell -> effectN( 1 ).misc_value1() + 1 );
    }
    else if ( spell -> effectN( 1 ).misc_value1() == -1 )
    {
      stat = STAT_ALL;
    }
  }
  else
  {
    stat = STAT_BONUS_ARMOR;
  }

  double amount = util::round( spell -> effectN( 1 ).average( effect.player, std::min( MAX_LEVEL, effect.player -> level ) ) );

  effect.player -> initial.stats.add_stat( stat, amount );
}

void set_bonus::t17_lfr_4pc_agimelee( special_effect_t& effect )
{
  struct t17_lfr_4pc_agi_melee_nuke_t : public melee_attack_t
  {
    t17_lfr_4pc_agi_melee_nuke_t( player_t* p ) :
      melee_attack_t( "converging_spikes", p, p -> find_spell( 179132 ) )
    {
      background = true;
      callbacks = false;
    }
  };

  action_t* a = effect.player -> create_proc_action( "converging_spikes" );
  if ( ! a )
  {
    a = new t17_lfr_4pc_agi_melee_nuke_t( effect.player );
  }

  effect.execute_action = a;

  new dbc_proc_callback_t( effect.player, effect );
}

void set_bonus::t17_lfr_4pc_leamelee( special_effect_t& effect )
{
  effect.player -> buffs.surge_of_energy = buff_creator_t( effect.player, "surge_of_energy", effect.player -> find_spell( 179116 ) )
                                           .affects_regen( true );
  effect.custom_buff = effect.player -> buffs.surge_of_energy;

  new dbc_proc_callback_t( effect.player, effect );
}

void set_bonus::t17_lfr_4pc_leacaster( special_effect_t& effect )
{
  struct natures_fury_t : public residual_action::residual_periodic_action_t< spell_t >
  {
    natures_fury_t( player_t* p ) :
      residual_action_t( "natures_fury", p, p -> find_spell( 179120 ) )
    { background = true; callbacks = false; }
  };

  struct natures_fury_cb_t : public dbc_proc_callback_t
  {
    const spell_data_t* cbdata;
    natures_fury_t* dot;

    natures_fury_cb_t( const special_effect_t& data ) :
      dbc_proc_callback_t( data.player, data ),
      cbdata( data.player -> find_spell( 179119 ) ),
      dot( new natures_fury_t( data.player ) )
    { }

    void reset()
    {
      dbc_proc_callback_t::reset();

      deactivate();
    }

    void execute( action_t* /* a */, action_state_t* state )
    {
      if ( state -> result_amount == 0 )
      {
        return;
      }

      double value = state -> result_amount * cbdata -> effectN( 1 ).percent();
      residual_action::trigger( dot, state -> target, value );
    }
  };

  struct natures_fury_buff_t : public buff_t
  {
    natures_fury_cb_t* callback;

    natures_fury_buff_t( player_t* player, natures_fury_cb_t* cb ) :
      buff_t( buff_creator_t( player, "natures_fury", player -> find_spell( 179119 ) ) ),
      callback( cb )
    { }

    void start( int stacks, double value, timespan_t duration )
    {
      buff_t::start( stacks, value, duration );

      callback -> activate();
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration )
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );

      callback -> deactivate();
    }
  };

  special_effect_t* damage_effect = new special_effect_t( effect.player );
  damage_effect -> spell_id = 179119;
  damage_effect -> name_str = "natures_fury_damage_driver";
  damage_effect -> proc_flags2_ = PF2_ALL_HIT; // Shift proccing to impact, so we can trigger with aoe spells properly
  effect.player -> special_effects.push_back( damage_effect );

  natures_fury_cb_t* cb = new natures_fury_cb_t( *damage_effect );

  effect.player -> buffs.natures_fury = new natures_fury_buff_t( effect.player, cb );
  effect.custom_buff = effect.player -> buffs.natures_fury;

  new dbc_proc_callback_t( effect.player, effect );
}

void set_bonus::t17_lfr_4pc_mailcaster( special_effect_t& effect )
{
  struct electric_orb_aoe_t : public spell_t
  {
    electric_orb_aoe_t( player_t* player ) :
      spell_t( "electric_orb", player, player -> find_spell( 179136 ) )
    {
      proc = background = may_crit = true;
      callbacks = false;

      aoe = -1;
    }
  };

  struct electric_orb_event_t : public event_t
  {
    electric_orb_aoe_t* aoe;
    player_t* target;
    unsigned pulse_id;

    electric_orb_event_t( sim_t& sim, electric_orb_aoe_t* a, player_t* t, unsigned pulse ) :
      event_t( sim ),
      aoe( a ), target( t ), pulse_id( pulse )
    {
      add_event( timespan_t::from_seconds( 2.0 ) );
    }
    virtual const char* name() const override
    { return "electric_orb_event"; }
    void execute()
    {
      aoe -> target = target;
      aoe -> execute();

      if ( ++pulse_id < 5 )
      {
        new ( sim() ) electric_orb_event_t( sim(), aoe, target, pulse_id );
      }
    }
  };

  struct electric_orb_cb_t : public dbc_proc_callback_t
  {
    electric_orb_aoe_t* aoe;

    electric_orb_cb_t( const special_effect_t& data ) :
      dbc_proc_callback_t( data.player, data ),
      aoe( new electric_orb_aoe_t( data.player ) )
    { }

    void execute( action_t* /* a */, action_state_t* state )
    {
      new ( *listener -> sim ) electric_orb_event_t(*listener -> sim, aoe, state -> target, 0 );
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT; // Proc on impact so we can proc on multiple targets

  new electric_orb_cb_t( effect );
}

void set_bonus::t17_lfr_4pc_platemelee( special_effect_t& effect )
{
  const spell_data_t* driver = effect.player -> find_spell( effect.spell_id );
  effect.player -> buffs.brute_strength = buff_creator_t( effect.player, "brute_strength", driver -> effectN( 1 ).trigger() )
                                          .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  effect.custom_buff = effect.player -> buffs.brute_strength;

  new dbc_proc_callback_t( effect.player, effect );
}

void set_bonus::t17_lfr_4pc_clothcaster( special_effect_t& effect )
{
  const spell_data_t* spell = spell_data_t::nil();

  switch ( effect.player -> specialization() )
  {
    case MAGE_ARCANE:
      spell = effect.player -> find_spell( 179157 );
      break;
    case MAGE_FIRE:
      spell = effect.player -> find_spell( 179154 );
      break;
    case MAGE_FROST:
      spell = effect.player -> find_spell( 179156 );
      break;
    case PRIEST_SHADOW:
    case WARLOCK_AFFLICTION:
    case WARLOCK_DEMONOLOGY:
    case WARLOCK_DESTRUCTION:
      spell = effect.player -> find_spell( 179155 );
      break;
    default:
      break;
  }

  if ( spell -> id() == 0 )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  effect.proc_flags_ = PF_ALL_DAMAGE;
  effect.proc_flags2_ = PF2_ALL_HIT;

  std::string spell_name = spell -> name_cstr();
  util::tokenize( spell_name );

  struct eruption_t : public spell_t
  {
    eruption_t( const std::string& name, player_t* player, const spell_data_t* s ) :
      spell_t( name, player, s )
    {
      background = proc = may_crit = true;
      callbacks = false;
    }
  };

  effect.execute_action = new eruption_t( spell_name, effect.player, spell );

  new dbc_proc_callback_t( effect.player, effect );
}

// Items ====================================================================

void item::rune_of_reorigination( special_effect_t& effect )
{
  struct rune_of_reorigination_callback_t : public dbc_proc_callback_t
  {
    enum
    {
      BUFF_CRIT = 0,
      BUFF_HASTE,
      BUFF_MASTERY
    };

    stat_buff_t* buff;

    rune_of_reorigination_callback_t( const special_effect_t& data ) :
      dbc_proc_callback_t( data.item -> player, data )
    {
      buff = static_cast< stat_buff_t* >( effect.custom_buff );
    }

    virtual void execute( action_t* action, action_state_t* /* state */ )
    {
      // We can never allow this trinket to refresh, so force the trinket to 
      // always expire, before we proc a new one.
      buff -> expire();

      player_t* p = action -> player;

      // Determine highest stat based on rating multipliered stats
      double chr = p -> composite_melee_haste_rating();
      if ( p -> sim -> scaling -> scale_stat == STAT_HASTE_RATING )
        chr -= p -> sim -> scaling -> scale_value * p -> composite_rating_multiplier( RATING_MELEE_HASTE );

      double ccr = p -> composite_melee_crit_rating();
      if ( p -> sim -> scaling -> scale_stat == STAT_CRIT_RATING )
        ccr -= p -> sim -> scaling -> scale_value * p -> composite_rating_multiplier( RATING_MELEE_CRIT );

      double cmr = p -> composite_mastery_rating();
      if ( p -> sim -> scaling -> scale_stat == STAT_MASTERY_RATING )
        cmr -= p -> sim -> scaling -> scale_value * p -> composite_rating_multiplier( RATING_MASTERY );

      // Give un-multipliered stats so we don't double dip anywhere.
      chr /= p -> composite_rating_multiplier( RATING_MELEE_HASTE );
      ccr /= p -> composite_rating_multiplier( RATING_MELEE_CRIT );
      cmr /= p -> composite_rating_multiplier( RATING_MASTERY );

      if ( p -> sim -> debug )
        p -> sim -> out_debug.printf( "%s rune_of_reorigination procs crit=%.0f haste=%.0f mastery=%.0f",
                            p -> name(), ccr, chr, cmr );

      if ( ccr >= chr )
      {
        // I choose you, crit
        if ( ccr >= cmr )
        {
          buff -> stats[ BUFF_CRIT    ].amount = 2 * ( chr + cmr );
          buff -> stats[ BUFF_HASTE   ].amount = -chr;
          buff -> stats[ BUFF_MASTERY ].amount = -cmr;
        }
        // I choose you, mastery
        else
        {
          buff -> stats[ BUFF_CRIT    ].amount = -ccr;
          buff -> stats[ BUFF_HASTE   ].amount = -chr;
          buff -> stats[ BUFF_MASTERY ].amount = 2 * ( ccr + chr );
        }
      }
      // I choose you, haste
      else if ( chr >= cmr )
      {
        buff -> stats[ BUFF_CRIT    ].amount = -ccr;
        buff -> stats[ BUFF_HASTE   ].amount = 2 * ( ccr + cmr );
        buff -> stats[ BUFF_MASTERY ].amount = -cmr;
      }
      // I choose you, mastery
      else
      {
        buff -> stats[ BUFF_CRIT    ].amount = -ccr;
        buff -> stats[ BUFF_HASTE   ].amount = -chr;
        buff -> stats[ BUFF_MASTERY ].amount = 2 * ( ccr + chr );
      }

      buff -> trigger();
    }
  };

  maintenance_check( 502 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = effect.item -> player -> find_spell( 139120 );

  std::string buff_name = spell -> name_cstr();
  util::tokenize( buff_name );

  stat_buff_t* buff = stat_buff_creator_t( effect.item -> player, buff_name, spell )
                      .activated( false )
                      .add_stat( STAT_CRIT_RATING, 0 )
                      .add_stat( STAT_HASTE_RATING, 0 )
                      .add_stat( STAT_MASTERY_RATING, 0 );

  effect.custom_buff  = buff;
  effect.ppm_         = -1.0 * driver -> real_ppm();
  effect.ppm_        *= item_database::approx_scale_coefficient( 528, effect.item -> item_level() );

  new rune_of_reorigination_callback_t( effect );
}

void item::spark_of_zandalar( special_effect_t& effect )
{
  maintenance_check( 502 );

  const spell_data_t* buff = effect.item -> player -> find_spell( 138960 );

  std::string buff_name = buff -> name_cstr();
  util::tokenize( buff_name );

  stat_buff_t* b = stat_buff_creator_t( effect.item -> player, buff_name, buff, effect.item );

  effect.custom_buff = b;

  struct spark_of_zandalar_callback_t : public dbc_proc_callback_t
  {
    buff_t*      sparks;
    stat_buff_t* buff;

    spark_of_zandalar_callback_t( const special_effect_t& data ) :
      dbc_proc_callback_t( *data.item, data )
    {
      const spell_data_t* spell = listener -> find_spell( 138958 );
      sparks = buff_creator_t( listener, "zandalari_spark_driver", spell )
               .quiet( true );
    }

    void execute( action_t* /* action */, action_state_t* /* state */ )
    {
      sparks -> trigger();

      if ( sparks -> stack() == sparks -> max_stack() )
      {
        sparks -> expire();
        proc_buff -> trigger();
      }
    }
  };

  new spark_of_zandalar_callback_t( effect );
};

void item::unerring_vision_of_leishen( special_effect_t& effect )
{
  struct perfect_aim_buff_t : public buff_t
  {
    perfect_aim_buff_t( player_t* p ) :
      buff_t( buff_creator_t( p, "perfect_aim", p -> find_spell( 138963 ) ).activated( false ) )
    { }

    void execute( int stacks, double value, timespan_t duration )
    {
      if ( current_stack == 0 )
      {
        player -> current.spell_crit  += data().effectN( 1 ).percent();
        player -> current.attack_crit += data().effectN( 1 ).percent();
        player -> invalidate_cache( CACHE_CRIT );
      }

      buff_t::execute( stacks, value, duration );
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration )
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );

      player -> current.spell_crit  -= data().effectN( 1 ).percent();
      player -> current.attack_crit -= data().effectN( 1 ).percent();
      player -> invalidate_cache( CACHE_CRIT );
    }
  };

  struct unerring_vision_of_leishen_callback_t : public dbc_proc_callback_t
  {
    unerring_vision_of_leishen_callback_t( const special_effect_t& data ) :
      dbc_proc_callback_t( data.item -> player, data )
    { }

    void initialize()
    {
      dbc_proc_callback_t::initialize();

      // Warlocks have a (hidden?) 0.6 modifier, not showing in DBCs.
      if ( listener -> type == WARLOCK )
        rppm.set_modifier( 0.6 );
    }
  };

  maintenance_check( 502 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );

  effect.ppm_         = -1.0 * driver -> real_ppm();
  effect.ppm_        *= item_database::approx_scale_coefficient( 528, effect.item -> item_level() );
  effect.proc_flags2_ = PF2_ALL_HIT;
  effect.custom_buff  = new perfect_aim_buff_t( effect.item -> player );

  new unerring_vision_of_leishen_callback_t( effect );
}

void item::skeers_bloodsoaked_talisman( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();
  // Aura is hidden, thre's no linkage in spell data actual
  const spell_data_t* buff = effect.item -> player -> find_spell( 146293 );

  std::string buff_name = buff -> name_cstr();
  util::tokenize( buff_name );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = stat_buff_creator_t( effect.item -> player, buff_name, buff )
                   .add_stat( STAT_CRIT_RATING, spell -> effectN( 1 ).average( effect.item ) )
                   .tick_behavior( BUFF_TICK_CLIP )
                   .period( spell -> effectN( 1 ).period() )
                   .duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::blackiron_micro_crucible( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  std::string buff_name = spell -> name_cstr();
  util::tokenize( buff_name );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = stat_buff_creator_t( effect.item -> player, buff_name, spell )
                   .add_stat( STAT_MULTISTRIKE_RATING, spell -> effectN( 1 ).average( effect.item ) )
                   .max_stack( 20 ) // Hardcoded for now - spell->max_stacks() returns 0
                   .tick_behavior( BUFF_TICK_CLIP )
                   .period( spell -> effectN( 1 ).period() )
                   .duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::humming_blackiron_trigger( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  std::string buff_name = spell -> name_cstr();
  util::tokenize( buff_name );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = stat_buff_creator_t( effect.item -> player, buff_name, spell )
                   .add_stat( STAT_CRIT_RATING, spell -> effectN( 1 ).average( effect.item ) )
                   .max_stack( 20 ) // Hardcoded for now - spell->max_stacks() returns 0
                   .tick_behavior( BUFF_TICK_CLIP )
                   .period( spell -> effectN( 1 ).period() )
                   .duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::battering_talisman_trigger( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();
  const spell_data_t* stacks = effect.item -> player -> find_spell( 146293 );

  std::string buff_name = spell -> name_cstr();
  util::tokenize( buff_name );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = stat_buff_creator_t( effect.item -> player, buff_name, spell )
    .add_stat( STAT_HASTE_RATING, spell -> effectN( 1 ).average( effect.item ) )
    .max_stack( stacks -> max_stacks() )
    .tick_behavior( BUFF_TICK_CLIP )
    .period( spell -> effectN( 1 ).period() )
    .duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::forgemasters_insignia( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  std::string buff_name = spell -> name_cstr();
  util::tokenize( buff_name );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = stat_buff_creator_t( effect.item -> player, buff_name, spell )
                   .add_stat( STAT_MULTISTRIKE_RATING, spell -> effectN( 1 ).average( effect.item ) )
                   .max_stack( 20 ) // Hardcoded for now - spell->max_stacks() returns 0
                   .tick_behavior( BUFF_TICK_CLIP )
                   .period( spell -> effectN( 1 ).period() )
                   .duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::autorepairing_autoclave( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  std::string buff_name = spell -> name_cstr();
  util::tokenize( buff_name );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = stat_buff_creator_t( effect.item -> player, buff_name, spell )
                   .add_stat( STAT_HASTE_RATING, spell -> effectN( 1 ).average( effect.item ) )
                   .max_stack( 20 ) // Hardcoded for now - spell->max_stacks() returns 0
                   .tick_behavior( BUFF_TICK_CLIP )
                   .period( spell -> effectN( 1 ).period() )
                   .duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::spellbound_runic_band( special_effect_t& effect )
{
  maintenance_check( 528 );

  player_t* p = effect.item -> player;
  const spell_data_t* driver = p -> find_spell( effect.spell_id );
  buff_t* buff = 0;

  // Need to test which procs on off-spec rings, assume correct proc for now.
  switch( p -> convert_hybrid_stat( STAT_STR_AGI_INT ) )
  {
    case STAT_STRENGTH:
      buff = buff_t::find( p, "archmages_greater_incandescence_str" );
      break;
    case STAT_AGILITY:
      buff = buff_t::find( p, "archmages_greater_incandescence_agi" );
      break;
    case STAT_INTELLECT:
      buff = buff_t::find( p, "archmages_greater_incandescence_int" );
      break;
    default:
      break;
  }

  effect.ppm_ = -1.0 * driver -> real_ppm();
  effect.custom_buff = buff;
  effect.type = SPECIAL_EFFECT_EQUIP;

  new dbc_proc_callback_t( p, effect );
}

void item::spellbound_solium_band( special_effect_t& effect )
{
  maintenance_check( 528 );

  player_t* p = effect.item -> player;
  const spell_data_t* driver = p -> find_spell( effect.spell_id );
  buff_t* buff = 0;

  //Need to test which procs on off-spec rings, assume correct proc for now.
  switch( p -> convert_hybrid_stat( STAT_STR_AGI_INT ) )
  {
    case STAT_STRENGTH:
      buff = buff_t::find( p, "archmages_incandescence_str" );
      break;
    case STAT_AGILITY:
      buff = buff_t::find( p, "archmages_incandescence_agi" );
      break;
    case STAT_INTELLECT:
      buff = buff_t::find( p, "archmages_incandescence_int" );
      break;
    default:
      break;
  }

  effect.ppm_ = -1.0 * driver -> real_ppm();
  effect.custom_buff = buff;
  effect.type = SPECIAL_EFFECT_EQUIP;

  new dbc_proc_callback_t( p, effect );
}

void item::black_blood_of_yshaarj( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* ticker = driver -> effectN( 1 ).trigger();
  const spell_data_t* buff = effect.item -> player -> find_spell( 146202 );

  std::string buff_name = buff -> name_cstr();
  util::tokenize( buff_name );

  stat_buff_t* b = stat_buff_creator_t( effect.item -> player, buff_name, buff )
                   .add_stat( STAT_INTELLECT, ticker -> effectN( 1 ).average( effect.item ) )
                   .tick_behavior( BUFF_TICK_CLIP )
                   .period( ticker -> effectN( 1 ).period() )
                   .duration( ticker -> duration () );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

struct flurry_of_xuen_melee_t : public melee_attack_t
{
  flurry_of_xuen_melee_t( player_t* player ) :
    melee_attack_t( "flurry_of_xuen", player, player -> find_spell( 147891 ) )
  {
    background = true;
    proc = false;
    aoe = 5;
    special = may_miss = may_parry = may_block = may_dodge = may_crit = true;
  }
};

struct flurry_of_xuen_ranged_t : public ranged_attack_t
{
  flurry_of_xuen_ranged_t( player_t* player ) :
    ranged_attack_t( "flurry_of_xuen", player, player -> find_spell( 147891 ) )
  {
    // TODO: check attack_power_mod.direct = data().extra_coeff();
    background = true;
    proc = false;
    aoe = 5;
    special = may_miss = may_parry = may_block = may_dodge = may_crit = true;
  }
};

struct flurry_of_xuen_driver_t : public attack_t
{
  action_t* ac;

  flurry_of_xuen_driver_t( player_t* player, action_t* action = 0 ) :
    attack_t( "flurry_of_xuen_driver", player, player -> find_spell( 146194 ) ),
    ac( 0 )
  {
    hasted_ticks = may_crit = may_miss = may_dodge = may_parry = callbacks = false;
    proc = background = dual = true;

    if ( ! action )
    {
      if ( player -> type == HUNTER )
        ac = new flurry_of_xuen_ranged_t( player );
      else
        ac = new flurry_of_xuen_melee_t( player );
    }
    else
      ac = action;
  }

  // Don't use tick action here, so we can get class specific snapshotting, if
  // there is a custom proc action crated. Hack and workaround and ugly.
  void tick( dot_t* )
  {
    if ( ac )
      ac -> schedule_execute();

    player -> trigger_ready();
  }
};

struct flurry_of_xuen_cb_t : public dbc_proc_callback_t
{
  flurry_of_xuen_cb_t( player_t* p, const special_effect_t& effect ) :
    dbc_proc_callback_t( p, effect )
  { }

  void trigger( action_t* action, void* call_data )
  {
    // Flurry of Xuen, and Lightning Strike cannot proc Flurry of Xuen
    if ( action -> id == 147891 || action -> id == 146194 || action -> id == 137597 )
      return;

    dbc_proc_callback_t::trigger( action, call_data );
  }
};

void item::flurry_of_xuen( special_effect_t& effect )
{
  maintenance_check( 600 );

  if ( effect.item -> sim -> challenge_mode )
    return;

  if ( effect.item -> player -> level == 100 )
    return;

  player_t* p = effect.item -> player;
  const spell_data_t* driver = p -> find_spell( effect.spell_id );

  effect.ppm_        = -1.0 * driver -> real_ppm();
  effect.ppm_       *= item_database::approx_scale_coefficient( effect.item -> parsed.data.level, effect.item -> item_level() );
  effect.rppm_scale = RPPM_HASTE;
  effect.execute_action = new flurry_of_xuen_driver_t( p, p -> create_proc_action( effect.name() ) );

  new flurry_of_xuen_cb_t( p, effect );
}

struct essence_of_yulon_t : public spell_t
{
  essence_of_yulon_t( player_t* p, const spell_data_t& driver ) :
    spell_t( "essence_of_yulon", p, p -> find_spell( 148008 ) )
  {
    background = may_crit = true;
    proc = false;
    aoe = 5;
    spell_power_mod.direct /= driver.duration().total_seconds() + 1;
  }
};

struct essence_of_yulon_driver_t : public spell_t
{
  essence_of_yulon_driver_t( player_t* player ) :
    spell_t( "essence_of_yulon", player, player -> find_spell( 146198 ) )
  {
    hasted_ticks = may_miss = may_dodge = may_parry = may_block = callbacks = may_crit = false;
    tick_zero = proc = background = dual = true;
    travel_speed = 0;

    tick_action = new essence_of_yulon_t( player, data() );
    dynamic_tick_action = true;
  }
};

struct essence_of_yulon_cb_t : public dbc_proc_callback_t
{

  essence_of_yulon_cb_t( player_t* p, const special_effect_t& effect ) :
    dbc_proc_callback_t( p, effect )
  { }

  void trigger( action_t* action, void* call_data )
  {
    if ( action -> id == 148008 ) // dot direct damage ticks can't proc itself
      return;

    dbc_proc_callback_t::trigger( action, call_data );
  }
};

void item::essence_of_yulon( special_effect_t& effect )
{
  maintenance_check( 600 );

  if ( effect.item -> sim -> challenge_mode )
    return;

  if ( effect.item -> player -> level == 100 )
    return;

  player_t* p = effect.item -> player;
  const spell_data_t* driver = p -> find_spell( effect.spell_id );

  effect.ppm_         = -1.0 * driver -> real_ppm();
  effect.ppm_        *= item_database::approx_scale_coefficient( effect.item -> parsed.data.level, effect.item -> item_level() );
  effect.rppm_scale  = RPPM_HASTE;
  effect.execute_action = new essence_of_yulon_driver_t( p );

  new essence_of_yulon_cb_t( p, effect );
}

void item::endurance_of_niuzao( special_effect_t& effect )
{
  maintenance_check( 600 );

  if ( effect.item -> sim -> challenge_mode )
    return;
  if ( effect.item -> player -> level == 100 )
    return;

  const spell_data_t* cd = effect.item -> player -> find_spell( 148010 );

  effect.item -> player -> legendary_tank_cloak_cd = effect.item -> player -> get_cooldown( "endurance_of_niuzao" );
  effect.item -> player -> legendary_tank_cloak_cd -> duration = cd -> duration();
}

void item::readiness( special_effect_t& effect )
{
  maintenance_check( 528 );

  struct cooldowns_t
  {
    specialization_e spec;
    const char*      cooldowns[8];
  };

  static const cooldowns_t __cd[] =
  {
    // NOTE: Spells that trigger buffs must have the cooldown of their buffs removed if they have one, or this trinket may cause undesirable results.
    { ROGUE_ASSASSINATION, { "evasion", "vanish", "cloak_of_shadows", "vendetta", 0, 0 } },
    { ROGUE_COMBAT,        { "evasion", "adrenaline_rush", "cloak_of_shadows", "killing_spree", 0, 0 } },
    { ROGUE_SUBTLETY,      { "evasion", "vanish", "cloak_of_shadows", "shadow_dance", 0, 0 } },
    { SHAMAN_ENHANCEMENT,  { "earth_elemental_totem", "fire_elemental_totem", "shamanistic_rage", "ascendance", "feral_spirit", 0 } },
    { DRUID_FERAL,         { "tigers_fury", "berserk", "barkskin", "survival_instincts", 0, 0, 0 } },
    { DRUID_GUARDIAN,      { "might_of_ursoc", "berserk", "barkskin", "survival_instincts", 0, 0, 0 } },
    { WARRIOR_FURY,        { "dragon_roar", "bladestorm", "shockwave", "avatar", "bloodbath", "recklessness", "storm_bolt", "heroic_leap" } },
    { WARRIOR_ARMS,        { "dragon_roar", "bladestorm", "shockwave", "avatar", "bloodbath", "recklessness", "storm_bolt", "heroic_leap" } },
    { WARRIOR_PROTECTION,  { "shield_wall", "demoralizing_shout", "last_stand", "recklessness", "heroic_leap", 0, 0 } },
    { DEATH_KNIGHT_BLOOD,  { "antimagic_shell", "dancing_rune_weapon", "icebound_fortitude", "outbreak", "vampiric_blood", "bone_shield", 0 } },
    { DEATH_KNIGHT_FROST,  { "antimagic_shell", "army_of_the_dead", "icebound_fortitude", "empower_rune_weapon", "outbreak", "pillar_of_frost", 0  } },
    { DEATH_KNIGHT_UNHOLY, { "antimagic_shell", "army_of_the_dead", "icebound_fortitude", "outbreak", "summon_gargoyle", 0 } },
    { MONK_BREWMASTER,	   { "fortifying_brew", "guard", "zen_meditation", 0, 0, 0, 0 } },
    { MONK_WINDWALKER,     { "energizing_brew", "fists_of_fury", "fortifying_brew", "zen_meditation", 0, 0, 0 } },
    { PALADIN_PROTECTION,  { "ardent_defender", "avenging_wrath", "divine_protection", "divine_shield", "guardian_of_ancient_kings", 0 } },
    { PALADIN_RETRIBUTION, { "avenging_wrath", "divine_protection", "divine_shield", "guardian_of_ancient_kings", 0, 0 } },
    { HUNTER_BEAST_MASTERY,{ "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", "bestial_wrath", 0 } },
    { HUNTER_MARKSMANSHIP, { "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", 0, 0 } },
    { HUNTER_SURVIVAL,     { "black_arrow", "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", 0 } },
    { SPEC_NONE,           { 0 } }
  };

  player_t* p = effect.item -> player;

  const spell_data_t* cdr_spell = p -> find_spell( effect.spell_id );
  const random_prop_data_t& budget = p -> dbc.random_property( effect.item -> item_level() );
  double cdr = 1.0 / ( 1.0 + budget.p_epic[ 0 ] * cdr_spell -> effectN( 1 ).m_average() / 100.0 );

  if ( p -> level > 90 )
  { // We have no clue how the trinket actually scales down with level. This will linearly decrease CDR until it hits .90 at level 100.
    double level_nerf = ( static_cast<double>( p -> level - 90 ) / 10.0 );
    level_nerf = ( 1 - cdr ) * level_nerf;
    cdr += level_nerf;
    cdr = std::min( 0.90, cdr ); // The amount of CDR doesn't go above 90%, even at level 100.
  }

  p -> buffs.cooldown_reduction -> s_data = cdr_spell;
  p -> buffs.cooldown_reduction -> default_value = cdr;
  p -> buffs.cooldown_reduction -> default_chance = 1;

  const cooldowns_t* cd = &( __cd[ 0 ] );
  do
  {
    if ( p -> specialization() != cd -> spec )
    {
      cd++;
      continue;
    }

    for ( size_t i = 0; i < 7; i++ )
    {
      if ( cd -> cooldowns[ i ] == 0 )
        break;

      cooldown_t* ability_cd = p -> get_cooldown( cd -> cooldowns[ i ] );
      ability_cd -> set_recharge_multiplier( cdr );
    }

    break;
  } while ( cd -> spec != SPEC_NONE );
}

void item::amplification( special_effect_t& effect )
{
  maintenance_check( 528 );

  player_t* p = effect.item -> player;
  const spell_data_t* amplify_spell = p -> find_spell( effect.spell_id );

  buff_t* first_amp = buff_t::find( p, "amplification" );
  buff_t* second_amp = buff_t::find( p, "amplification_2" );
  buff_t* amp_buff = 0;
  double* amp_value = 0;
  if ( first_amp -> default_chance == 0 )
  {
    amp_buff = first_amp;
    amp_value = &( p -> passive_values.amplification_1 );
  }
  else
  {
    amp_buff = second_amp;
    amp_value = &( p -> passive_values.amplification_2 );
  }

  const random_prop_data_t& budget = p -> dbc.random_property( effect.item -> item_level() );
  *amp_value = budget.p_epic[ 0 ] * amplify_spell -> effectN( 2 ).m_average() / 100.0;
  if ( p -> level > 90 )
  { // We have no clue how the trinket actually scales down with level. This will linearly decrease amplification until it hits 0 at level 100.
    double level_nerf = ( static_cast<double>( p -> level ) - 90 ) / 10.0;
    *amp_value *= 1 - level_nerf;
    *amp_value = std::max( 0.01, *amp_value ); // Cap it at 1%
  }
  amp_buff -> default_value = *amp_value;
  amp_buff -> default_chance = 1.0;
}

void item::prismatic_prison_of_pride( special_effect_t& effect )
{
  // Disable Prismatic Prison of Pride stat proc (Int) for all roles but HEAL
  if ( effect.item -> player -> role != ROLE_HEAL )
    return;

  effect.type = SPECIAL_EFFECT_EQUIP;
  new dbc_proc_callback_t( effect.item, effect );
}

void item::purified_bindings_of_immerseus( special_effect_t& effect )
{
  // Disable Purified Bindings stat proc (Int) on healing roles
  if ( effect.item -> player -> role == ROLE_HEAL )
    return;

  effect.type = SPECIAL_EFFECT_EQUIP;
  new dbc_proc_callback_t( effect.item, effect );
}

void item::thoks_tail_tip( special_effect_t& effect )
{
  // Disable Thok's Tail Tip stat proc (Str) on healing roles
  if ( effect.item -> player -> role == ROLE_HEAL )
    return;

  effect.type = SPECIAL_EFFECT_EQUIP;
  new dbc_proc_callback_t( effect.item, effect );
}

template <typename T>
struct cleave_t : public T
{
  cleave_t( const item_t* item, const std::string& name, school_e s ) :
    T( name, item -> player )
  {
    this -> callbacks = false;
    this -> may_crit = false;
    this -> may_glance = false;
    this -> may_miss = true;
    this -> special = true;
    this -> proc = true;
    this -> background = true;
    this -> school = s;
    this -> aoe = 5;
    if ( this -> type == ACTION_ATTACK )
    {
      this -> may_dodge = true;
      this -> may_parry = true;
      this -> may_block = true;
    }
  }

  void init()
  {
    T::init();

    this -> snapshot_flags = 0;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const
  {
    tl.clear();

    for ( size_t i = 0, actors = this -> sim -> target_non_sleeping_list.size(); i < actors; i++ )
    {
      player_t* t = this -> sim -> target_non_sleeping_list[ i ];

      if ( t -> is_enemy() && ( t != this -> target ) )
        tl.push_back( t );
    }

    return tl.size();
  }

  double target_armor( player_t* ) const
  { return 0.0; }
};

void item::cleave( special_effect_t& effect )
{
  maintenance_check( 528 );

  struct cleave_callback_t : public dbc_proc_callback_t
  {
    cleave_t<spell_t>* cleave_spell;
    cleave_t<attack_t>* cleave_attack;

    cleave_callback_t( const special_effect_t& data ) :
      dbc_proc_callback_t( *data.item, data )
    {
      cleave_spell = new cleave_t<spell_t>( data.item, "cleave_spell", SCHOOL_NATURE );
      cleave_attack = new cleave_t<attack_t>( data.item, "cleave_attack", SCHOOL_PHYSICAL );

    }

    void execute( action_t* action, action_state_t* state )
    {
      action_t* a = 0;

      if ( action -> type == ACTION_ATTACK )
        a = cleave_attack;
      else if ( action -> type == ACTION_SPELL )
        a = cleave_spell;
      // TODO: Heal

      if ( a )
      {
        a -> base_dd_min = a -> base_dd_max = state -> result_amount;
        // Invalidate target cache if target changes
        if ( a -> target != state -> target )
          a -> target_cache.is_valid = false;
        a -> target = state -> target;
        a -> schedule_execute();
      }
    }
  };

  player_t* p = effect.item -> player;
  const random_prop_data_t& budget = p -> dbc.random_property( effect.item -> item_level() );
  const spell_data_t* cleave_driver_spell = p -> find_spell( effect.spell_id );

  // Needs a damaging result
  effect.proc_flags2_ = PF2_ALL_HIT;
  effect.proc_chance_ = budget.p_epic[ 0 ] * cleave_driver_spell -> effectN( 1 ).m_average() / 10000.0;

  if ( p -> level > 90 )
  { // We have no clue how the trinket actually scales down with level. This will linearly decrease amplification until it hits 0 at level 100.
    double level_nerf = ( static_cast<double>( p -> level ) - 90 ) / 10.0;
     effect.proc_chance_ *= 1 - level_nerf;
     effect.proc_chance_ = std::max( 0.01, effect.proc_chance_ ); // Cap it at 1%
  }

  new cleave_callback_t( effect );
}

void item::heartpierce( special_effect_t& effect )
{
  struct invigorate_proc_t : public spell_t
  {
    gain_t* g;

    invigorate_proc_t( player_t* player, const spell_data_t* d ) :
      spell_t( "invigoration", player, d ),
      g( player -> get_gain( "invigoration" ) )
    {
      may_miss = may_crit = harmful = may_dodge = may_parry = callbacks = false;
      tick_may_crit = hasted_ticks = false;
      dual = quiet = background = true;
      target = player;
    }

    void tick( dot_t* d )
    {
      spell_t::tick( d );

      player -> resource_gain( player -> primary_resource(),
                               data().effectN( 1 ).resource( player -> primary_resource() ),
                               g,
                               this );
    }
  };

  unsigned spell_id = 0;
  switch ( effect.item -> player -> primary_resource() )
  {
    case RESOURCE_MANA:
      spell_id = 71881;
      break;
    case RESOURCE_ENERGY:
      spell_id = 71882;
      break;
    case RESOURCE_RAGE:
      spell_id = 71883;
      break;
    default:
      break;
  }

  if ( spell_id == 0 )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  effect.ppm_ = 1.0;
  effect.execute_action = new invigorate_proc_t( effect.item -> player, effect.item -> player -> find_spell( spell_id ) );

  new dbc_proc_callback_t( effect.item -> player, effect );
}

} // UNNAMED NAMESPACE

/*
 * Initialize a special effect, based on a spell id. Returns true if the first
 * phase initialization succeeded, false otherwise. If the spell id points to a
 * spell that our system cannot support, also sets the special effect type to
 * SPECIAL_EFFECT_NONE.
 *
 * Note that the first phase initialization simply fills up special_effect_t
 * with relevant information for non-custom special effects. Second phase of
 * the initialization (performed by unique_gear::init) will instantiate the
 * proc callback, and relevant actions/buffs, or call a custom function to
 * perform the initialization.
 */
bool unique_gear::initialize_special_effect( special_effect_t& effect,
                                             unsigned          spell_id )
{
  bool ret = true;
  player_t* p = effect.player;

  // Always check class specific (first phase) special effect initialization
  // first. If that returns false (nothing done), move to generic init
  if ( ! p -> init_special_effect( effect, spell_id ) )
  {
    const special_effect_db_item_t& dbitem = find_special_effect_db_item( __special_effect_db, (int)sizeof_array( __special_effect_db ), spell_id );

    // Figure out first phase options from our special effect database
    if ( dbitem.spell_id == spell_id )
    {
      // Custom special effect initialization is deferred, and no parsing from
      // spell data is done automatically.
      if ( dbitem.custom_cb != 0 )
      {
        effect.type = SPECIAL_EFFECT_CUSTOM;
        effect.custom_init = dbitem.custom_cb;
      }

      // Parse auxilary effect options before doing spell data based parsing
      if ( dbitem.encoded_options != 0 )
      {
        std::string encoded_options = dbitem.encoded_options;
        for ( size_t i = 0; i < encoded_options.length(); i++ )
          encoded_options[ i ] = std::tolower( encoded_options[ i ] );
        // Note, if the encoding parse fails (this should never ever happen),
        // we don't parse game client data either.
        if ( ! special_effect::parse_special_effect_encoding( effect, encoded_options ) )
          return false;
      }
    }
  }

  // Setup the driver always, though honoring any parsing performed in the 
  // first phase options.
  if ( effect.spell_id == 0 )
    effect.spell_id = spell_id;

  if ( effect.spell_id > 0 && p -> find_spell( effect.spell_id ) -> id() != effect.spell_id )
  {
    if ( p -> sim -> debug )
      p -> sim -> out_debug.printf( "Player %s unable to initialize special effect in item %s, spell identifier %u not found.",
          p -> name(), effect.item ? effect.item -> name() : "unknown", effect.spell_id );
    effect.type = SPECIAL_EFFECT_NONE;
    return ret;
  }

  // For generic procs, make sure we have a PPM, RPPM or Proc Chance available,
  // otherwise there's no point in trying to proc anything
  if ( effect.type == SPECIAL_EFFECT_EQUIP && ! special_effect::usable_proc( effect ) )
    effect.type = SPECIAL_EFFECT_NONE;
  else if ( effect.type == SPECIAL_EFFECT_USE &&
            effect.buff_type() == SPECIAL_EFFECT_BUFF_NONE &&
            effect.action_type() == SPECIAL_EFFECT_ACTION_NONE )
    effect.type = SPECIAL_EFFECT_NONE;

  return ret;
}

const special_effect_db_item_t& unique_gear::find_special_effect_db_item( const special_effect_db_item_t* start, unsigned n, unsigned spell_id )
{
  for ( size_t i = 0; i < n; i++ )
  {
    const special_effect_db_item_t* dbitem = start + i;
    if ( dbitem -> spell_id == spell_id )
      return *dbitem;
  }

  return *( start + ( n - 1 ) );
}

// ==========================================================================
// unique_gear::init
// ==========================================================================

void unique_gear::init( player_t* p )
{
  if ( p -> is_pet() || p -> is_enemy() ) return;

  for ( size_t i = 0; i < p -> items.size(); i++ )
  {
    item_t& item = p -> items[ i ];

    for ( size_t j = 0; j < item.parsed.special_effects.size(); j++ )
    {
      special_effect_t* effect = item.parsed.special_effects[ j ];
      if ( p -> sim -> debug )
        p -> sim -> out_debug.printf( "Initializing item-based special effect %s", effect -> to_string().c_str() );

      if ( effect -> type == SPECIAL_EFFECT_CUSTOM )
      {
        assert( effect -> custom_init );
        effect -> custom_init( *effect );
      }
      else if ( effect -> type == SPECIAL_EFFECT_EQUIP )
        new dbc_proc_callback_t( item, *effect );
    }
  }

  // Generic special effects, bound to no specific item
  for ( size_t i = 0; i < p -> special_effects.size(); i++ )
  {
    special_effect_t* effect = p -> special_effects[ i ];
    if ( p -> sim -> debug )
      p -> sim -> out_debug.printf( "Initializing generic special effect %s", effect -> to_string().c_str() );

    if ( effect -> type == SPECIAL_EFFECT_CUSTOM )
    {
      assert( effect -> custom_init );
      effect -> custom_init( *effect );
    }
    else if ( effect -> type == SPECIAL_EFFECT_EQUIP )
      new dbc_proc_callback_t( p, *effect );
  }
}

// Figure out if a given generic buff (associated with a trinket/item) is a
// stat buff of the correct type
static bool buff_has_stat( const buff_t* buff, stat_e stat )
{
  if ( ! buff )
    return false;

  // Not a stat buff
  const stat_buff_t* stat_buff = dynamic_cast< const stat_buff_t* >( buff );
  if ( ! stat_buff )
    return false;

  // At this point, if "any" was specificed, we're satisfied
  if ( stat == STAT_ALL )
    return true;

  for ( size_t i = 0, end = stat_buff -> stats.size(); i < end; i++ )
  {
    if ( stat_buff -> stats[ i ].stat == stat )
      return true;
  }

  return false;
}

// Base class for item effect expressions, finds all the special effects in the
// listed slots
struct item_effect_base_expr_t : public expr_t
{
  std::vector<const special_effect_t*> effects;

  item_effect_base_expr_t( action_t* a, const std::vector<slot_e> slots ) :
    expr_t( "item_effect_base_expr" )
  {
    const special_effect_t* e = 0;

    for ( size_t i = 0; i < slots.size(); i++ )
    {
      e = &( a -> player -> items[ slots[ i ] ].special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_EQUIP ) );
      if ( e -> source != SPECIAL_EFFECT_SOURCE_NONE )
        effects.push_back( e );

      e = &( a -> player -> items[ slots[ i ] ].special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) );
      if ( e -> source != SPECIAL_EFFECT_SOURCE_NONE )
        effects.push_back( e );

      e = &( a -> player -> items[ slots[ i ] ].special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_CUSTOM ) );
      if ( e -> source != SPECIAL_EFFECT_SOURCE_NONE )
        effects.push_back( e );
    }
  }
};

// Base class for expression-based item expressions (such as buff, or cooldown
// expressions). Implements the behavior of expression evaluation.
struct item_effect_expr_t : public item_effect_base_expr_t
{
  std::vector<expr_t*> exprs;

  item_effect_expr_t( action_t* a, const std::vector<slot_e> slots ) :
    item_effect_base_expr_t( a, slots )
  { }

  // Evaluates automatically to the maximum value out of all expressions, may
  // not be wanted in all situations. Best case? We should allow internal
  // operators here somehow
  double evaluate()
  {
    double result = 0;
    for ( size_t i = 0, end = exprs.size(); i < end; i++ )
    {
      double r = exprs[ i ] -> eval();
      if ( r > result )
        result = r;
    }

    return result;
  }

  virtual ~item_effect_expr_t()
  { range::dispose( exprs ); }
};

// Buff based item expressions, creates buff expressions for the items from
// user input
struct item_buff_expr_t : public item_effect_expr_t
{
  item_buff_expr_t( action_t* a, const std::vector<slot_e> slots, stat_e s, bool stacking, const std::string& expr_str ) :
    item_effect_expr_t( a, slots )
  {
    for ( size_t i = 0, end = effects.size(); i < end; i++ )
    {
      const special_effect_t* e = effects[ i ];

      buff_t* b = buff_t::find( a -> player, e -> name() );
      if ( buff_has_stat( b, s ) && ( ( ! stacking && b -> max_stack() <= 1 ) || ( stacking && b -> max_stack() > 1 ) ) )
      {
        if ( expr_t* expr_obj = buff_t::create_expression( b -> name(), a, expr_str, b ) )
          exprs.push_back( expr_obj );
      }
    }
  }
};

struct item_buff_exists_expr_t : public item_effect_expr_t
{
  double v;

  item_buff_exists_expr_t( action_t* a, const std::vector<slot_e>& slots, stat_e s ) :
    item_effect_expr_t( a, slots ), v( 0 )
  {
    for ( size_t i = 0, end = effects.size(); i < end; i++ )
    {
      const special_effect_t* e = effects[ i ];

      buff_t* b = buff_t::find( a -> player, e -> name() );
      if ( buff_has_stat( b, s ) )
      {
        v = 1;
        break;
      }
    }
  }

  double evaluate()
  { return v; }
};

// Cooldown based item expressions, creates cooldown expressions for the items
// from user input
struct item_cooldown_expr_t : public item_effect_expr_t
{
  item_cooldown_expr_t( action_t* a, const std::vector<slot_e> slots, const std::string& expr ) :
    item_effect_expr_t( a, slots )
  {
    for ( size_t i = 0, end = effects.size(); i < end; i++ )
    {
      const special_effect_t* e = effects[ i ];
      if ( e -> cooldown() != timespan_t::zero() )
      {
        cooldown_t* cd = a -> player -> get_cooldown( e -> cooldown_name() );
        if ( expr_t* expr_obj = cd -> create_expression( a, expr ) )
          exprs.push_back( expr_obj );
      }
    }
  }
};

struct item_cooldown_exists_expr_t : public item_effect_expr_t
{
  double v;

  item_cooldown_exists_expr_t( action_t* a, const std::vector<slot_e>& slots ) :
    item_effect_expr_t( a, slots ), v( 0 )
  {
    for ( size_t i = 0, end = effects.size(); i < end; i++ )
    {
      const special_effect_t* e = effects[ i ];
      if ( e -> cooldown() != timespan_t::zero() )
      {
        v = 1;
        break;
      }
    }
  }

  double evaluate()
  { return v; }
};

/**
 * Create "trinket" expressions, or anything relating to special effects.
 *
 * Note that this method returns zero (nullptr) when it cannot create an
 * expression.  The callee (player_t::create_expression) will handle unknown
 * expression processing.
 *
 * Trinket expressions are of the form:
 * trinket[.12].(has_|)(stacking_|)proc.<stat>.<buff_expr> OR
 * trinket[.12].(has_|)cooldown.<cooldown_expr>
 */
expr_t* unique_gear::create_expression( action_t* a, const std::string& name_str )
{
  enum proc_expr_e
  {
    PROC_EXISTS,
    PROC_ENABLED
  };

  enum proc_type_e
  {
    PROC_STAT,
    PROC_STACKING_STAT,
    PROC_COOLDOWN,
  };

  int ptype_idx = 1, stat_idx = 2, expr_idx = 3;
  enum proc_expr_e pexprtype = PROC_ENABLED;
  enum proc_type_e ptype = PROC_STAT;
  stat_e stat = STAT_NONE;
  std::vector<slot_e> slots;

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( util::is_number( splits[ 1 ] ) )
  {
    if ( splits[ 1 ] == "1" )
    {
      slots.push_back( SLOT_TRINKET_1 );
    }
    else if ( splits[ 1 ] == "2" )
    {
      slots.push_back( SLOT_TRINKET_2 );
    }
    else
      return 0;
    ptype_idx++;

    stat_idx++;
    expr_idx++;
  }
  // No positional parameter given so check both trinkets
  else
  {
    slots.push_back( SLOT_TRINKET_1 );
    slots.push_back( SLOT_TRINKET_2 );
  }

  if ( util::str_prefix_ci( splits[ ptype_idx ], "has_" ) )
    pexprtype = PROC_EXISTS;

  if ( util::str_in_str_ci( splits[ ptype_idx ], "cooldown" ) )
  {
    ptype = PROC_COOLDOWN;
    // Cooldowns dont have stat type for now
    expr_idx--;
  }

  if ( util::str_in_str_ci( splits[ ptype_idx ], "stacking_" ) )
    ptype = PROC_STACKING_STAT;

  if ( ptype != PROC_COOLDOWN )
  {
    // Use "all stat" to indicate "any" ..
    if ( util::str_compare_ci( splits[ stat_idx ], "any" ) )
      stat = STAT_ALL;
    else
    {
      stat = util::parse_stat_type( splits[ stat_idx ] );
      if ( stat == STAT_NONE )
        return 0;
    }
  }

  if ( pexprtype == PROC_ENABLED && ptype != PROC_COOLDOWN && splits.size() >= 4 )
  {
    return new item_buff_expr_t( a, slots, stat, ptype == PROC_STACKING_STAT, splits[ expr_idx ] );
  }
  else if ( pexprtype == PROC_ENABLED && ptype == PROC_COOLDOWN && splits.size() >= 3 )
  {
    return new item_cooldown_expr_t( a, slots, splits[ expr_idx ] );
  }
  else if ( pexprtype == PROC_EXISTS )
  {
    if ( ptype != PROC_COOLDOWN )
    {
      return new item_buff_exists_expr_t( a, slots, stat );
    }
    else
    {
      return new item_cooldown_exists_expr_t( a, slots );
    }
  }

  return 0;
}

static std::string::size_type match_prefix( const std::string& str, const char* prefixes[] )
{
  std::string::size_type offset = std::string::npos;

  while ( *prefixes != 0 && offset == std::string::npos )
  {
    offset = str.find( *prefixes );
    if ( offset != std::string::npos )
      offset += strlen( *prefixes );

    prefixes++;
  }

  return offset;
}

static std::string::size_type match_suffix( const std::string& str, const char* suffixes[] )
{
  std::string::size_type offset = std::string::npos;

  while ( *suffixes != 0 && offset == std::string::npos )
  {
    offset = str.rfind( *suffixes );
    suffixes++;
  }

  return offset;
}

// Find a consumable of a given subtype, see data_enum.hh for type values.
// Returns 0 if not found.
const item_data_t* unique_gear::find_consumable( const dbc_t& dbc,
                                                 const std::string& name,
                                                 item_subclass_consumable type )
{
  // Poor man's longest matching prefix!
  static const char* potion_prefixes[] = { "potion_of_the_", "potion_of_", "potion_", 0 };
  static const char* potion_suffixes[] = { "_potion", 0 };

  const item_data_t* item = 0;
  std::string consumable_name;

  for ( item = dbc::items( maybe_ptr( dbc.ptr ) ); item -> id != 0; item++ )
  {
    if ( item -> item_class != 0 )
      continue;

    if ( item -> item_subclass != type )
      continue;

    consumable_name = item -> name;

    util::tokenize( consumable_name );

    if ( util::str_compare_ci( consumable_name, name ) )
      break;

    if ( type == ITEM_SUBCLASS_POTION )
    {
      std::string::size_type prefix_offset = match_prefix( consumable_name, potion_prefixes );
      std::string::size_type suffix_offset = match_suffix( consumable_name, potion_suffixes );

      if ( prefix_offset == std::string::npos )
        prefix_offset = 0;

      if ( suffix_offset == std::string::npos )
        suffix_offset = consumable_name.size();
      else if ( suffix_offset <= prefix_offset )
        suffix_offset = consumable_name.size();

      std::string parsed_name = consumable_name.substr( prefix_offset, suffix_offset );
      if ( util::str_compare_ci( name, parsed_name ) )
        break;
    }
  }

  if ( item -> id != 0 )
    return item;

  return 0;
}

const item_data_t* unique_gear::find_item_by_spell( const dbc_t& dbc, unsigned spell_id )
{
  for ( const item_data_t* item = dbc::items( maybe_ptr( dbc.ptr ) ); item -> id != 0; item++ )
  {
    for ( size_t spell_idx = 0, end = sizeof_array( item -> id_spell ); spell_idx < end; spell_idx++ )
    {
      if ( item -> id_spell[ spell_idx ] == static_cast<int>( spell_id ) )
        return item;
    }
  }

  return 0;
}
