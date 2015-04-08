{
	'targets': [
		{
			'target_name': 'simcraft',

			'sources': [
				'src/simulationcraft.cc',
				'src/simnode_t.cc'
			],

			'dependencies': [
				'<(module_root_dir)/deps/simc.gyp:simc'
			],

			'include_dirs': [
				'deps/v8-convert'
			],

			'cflags_cc!': [ '-fno-rtti' ],
	        'cflags': [ '-fexceptions' ],
	        'cflags_cc': ['-fexceptions'],

			'defines': [
	            'PIC',
	            'HAVE_CONFIG_H',
	            'SRC_UTIL_H_'
	        ],

			'conditions': [
				['OS=="mac"', {
					'xcode_settings': {
						'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
						'GCC_ENABLE_CPP_RTTI': 'YES'
					}
				}]
			]
		}
	]
}
