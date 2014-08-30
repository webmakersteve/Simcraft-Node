{
   "targets": [{

        'cflags_cc!': [ '-fno-rtti' ],
        "cflags": [ '-fexceptions' ],
        'cflags_cc': ['-fexceptions'],
        "conditions": [
          ['OS=="mac"', {
            'xcode_settings': {
              'GCC_ENABLE_CPP_RTTI': 'YES',
              'GCC_ENABLE_CPP_EXCEPTIONS': 'YES', # -fexceptions
            }
          }]

        ],

        'defines': [
            'PIC',
            'HAVE_CONFIG_H',
            'SRC_UTIL_H_'
        ],

        "target_name": "simc",

        "include_dirs": [""],

        'direct_dependent_settings': {
          'include_dirs': [
            'simc/engine'
          ]
        },

        "sources": [
            "simulationcraft.hpp",
            "sc_generic.hpp",
            "sc_timespan.hpp",
            "util/sample_data.hpp",
            "util/sc_resourcepaths.hpp",
            "util/timeline.hpp",
            "util/rng.hpp",
            "util/sc_io.hpp",
            "dbc/specialization.hpp",
            "dbc/dbc.hpp",
            "dbc/data_enums.hh",
            "dbc/data_definitions.hh",
            "utf8.h",
            "utf8/core.h",
            "utf8/checked.h",
            "utf8/unchecked.h",
            "util/rng.cpp",
            "util/sc_io.cpp",
            "sc_thread.cpp",
            "sc_util.cpp",
            "action/sc_action_state.cpp",
            "action/sc_action.cpp",
            "action/sc_attack.cpp",
            "action/sc_dot.cpp",
            "action/sc_sequence.cpp",
            "action/sc_spell.cpp",
            "action/sc_stats.cpp",
            "buff/sc_buff.cpp",
            "player/sc_consumable.cpp",
            "player/sc_item.cpp",
            "player/sc_pet.cpp",
            "player/sc_player.cpp",
            "player/sc_set_bonus.cpp",
            "player/sc_unique_gear.cpp",
            "sim/sc_cooldown.cpp",
            "sim/sc_event.cpp",
            "sim/sc_expressions.cpp",
            "sim/sc_gear_stats.cpp",
            "sim/sc_option.cpp",
            "sim/sc_plot.cpp",
            "sim/sc_raid_event.cpp",
            "sim/sc_reforge_plot.cpp",
            "sim/sc_scaling.cpp",
            "sim/sc_sim.cpp",
            "sim/sc_core_sim.cpp",
            "report/sc_report_html_player.cpp",
            "report/sc_report_html_sim.cpp",
            "report/sc_report_text.cpp",
            "report/sc_report_xml.cpp",
            "report/sc_report_csv_data.cpp",
            "report/sc_report.cpp",
            "report/sc_chart.cpp",
            "dbc/sc_const_data.cpp",
            "dbc/sc_item_data.cpp",
            "dbc/sc_spell_data.cpp",
            "dbc/sc_spell_info.cpp",
            "dbc/sc_data.cpp",
            "interfaces/sc_bcp_api.cpp",
            "interfaces/sc_chardev.cpp",
            "interfaces/sc_http.cpp",
            "interfaces/sc_js.cpp",
            "interfaces/sc_rawr.cpp",
            "interfaces/sc_wowhead.cpp",
            "interfaces/sc_xml.cpp",
            "class_modules/sc_death_knight.cpp",
            "class_modules/sc_druid.cpp",
            "class_modules/sc_enemy.cpp",
            "class_modules/sc_hunter.cpp",
            "class_modules/sc_mage.cpp",
            "class_modules/sc_monk.cpp",
            "class_modules/sc_paladin.cpp",
            "class_modules/sc_priest.cpp",
            "class_modules/sc_rogue.cpp",
            "class_modules/sc_shaman.cpp",
            "class_modules/sc_warlock.cpp",
            "class_modules/sc_warrior.cpp",
    	      "sc_main.cpp",
            "addon.cc"

        ]
      }]
}
