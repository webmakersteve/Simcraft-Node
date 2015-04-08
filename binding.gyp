{
	'targets': [
		{
			'target_name': 'simulationcraft',

			'sources': [
				'src/simulationcraft.cc',
				'src/simnode_t.cc'
			],

			'dependencies': [
				'<(module_root_dir)/deps/simc.gyp:simc'
			],

			'include_dirs': [
				'deps/simc',
				'deps/v8-convert'
			],

			'cflags': [
				'-Wall'
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
