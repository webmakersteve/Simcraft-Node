{

  'targets': [{
    'target_name': 'simc',
    'type': 'static_library',

    'defines': [
        'PIC',
        'HAVE_CONFIG_H',
        'SRC_UTIL_H_'
    ],

    'direct_dependent_settings': {
      'include_dirs': [
        'simc/engine'
      ]
    },

    'include_dirs': [
        'simc/engine'
    ],

    'sources': [
        'simc/engine/util/xml.cpp',
        'simc/engine/util/str.cpp',
        'simc/engine/util/rng.cpp',
        'simc/engine/util/io.cpp',
        'simc/engine/util/concurrency.cpp',
        'simc/engine/sim/sc_sim.cpp',
        'simc/engine/sim/sc_scaling.cpp',
        'simc/engine/sim/sc_reforge_plot.cpp',
        'simc/engine/sim/sc_raid_event.cpp',
        'simc/engine/sim/sc_plot.cpp',
        'simc/engine/sim/sc_option.cpp',
        'simc/engine/sim/sc_gear_stats.cpp',
        'simc/engine/sim/sc_expressions.cpp',
        'simc/engine/sim/sc_event.cpp',
        'simc/engine/sim/sc_core_sim.cpp',
        'simc/engine/sim/sc_cooldown.cpp',
        'simc/engine/report/sc_report_xml.cpp',
        'simc/engine/report/sc_report_text.cpp',
        'simc/engine/report/sc_report_html_sim.cpp',
        'simc/engine/report/sc_report_html_player.cpp',
        'simc/engine/report/sc_report_csv_data.cpp',
        'simc/engine/report/sc_report.cpp',
        'simc/engine/report/sc_chart.cpp',
        'simc/engine/player/sc_unique_gear.cpp',
        'simc/engine/player/sc_set_bonus.cpp',
        'simc/engine/player/sc_proc.cpp',
        'simc/engine/player/sc_player.cpp',
        'simc/engine/player/sc_pet.cpp',
        'simc/engine/player/sc_item.cpp',
        'simc/engine/player/sc_enchant.cpp',
        'simc/engine/player/sc_consumable.cpp',
        'simc/engine/interfaces/sc_wowhead.cpp',
        'simc/engine/interfaces/sc_http.cpp',
        'simc/engine/interfaces/sc_bcp_api.cpp',
        'simc/engine/dbc/sc_spell_info.cpp',
        'simc/engine/dbc/sc_spell_data.cpp',
        'simc/engine/dbc/sc_item_data.cpp',
        'simc/engine/dbc/sc_data.cpp',
        'simc/engine/dbc/sc_const_data.cpp',
        'simc/engine/class_modules/sc_warrior.cpp',
        'simc/engine/class_modules/sc_warlock.cpp',
        'simc/engine/class_modules/sc_shaman.cpp',
        'simc/engine/class_modules/sc_rogue.cpp',
        'simc/engine/class_modules/sc_priest.cpp',
        'simc/engine/class_modules/sc_paladin.cpp',
        'simc/engine/class_modules/sc_monk.cpp',
        'simc/engine/class_modules/sc_mage.cpp',
        'simc/engine/class_modules/sc_hunter.cpp',
        'simc/engine/class_modules/sc_enemy.cpp',
        'simc/engine/class_modules/sc_druid.cpp',
        'simc/engine/class_modules/sc_death_knight.cpp',
        'simc/engine/buff/sc_buff.cpp',
        'simc/engine/action/sc_stats.cpp',
        'simc/engine/action/sc_spell.cpp',
        'simc/engine/action/sc_sequence.cpp',
        'simc/engine/action/sc_dot.cpp',
        'simc/engine/action/sc_attack.cpp',
        'simc/engine/action/sc_action_state.cpp',
        'simc/engine/action/sc_action.cpp',
        'simc/engine/sc_util.cpp',
        'simc/engine/sc_main.cpp'
    ],

    'cflags_cc!': [ '-fno-rtti' ],
    "cflags": [ '-fexceptions' ],
    'cflags_cc': ['-fexceptions'],
    'conditions': [
      ['OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_RTTI': 'YES',
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES', # -fexceptions
          'GCC_ENABLE_CPP_RTTI': 'YES'
        }
      }]

    ]

  }]
}
