{
	'targets': [
		{
			'target_name': 'simulationcraft',

			'sources': [
				'src/simulationcraft.cc'
			],

			'dependencies': [
				'<(module_root_dir)/deps/simc.gyp:simc'
			],

			'include_dirs': [
				'deps/simc',
			],

			'cflags': [
				'-Wall'
			],

			'conditions': [
				['OS=="mac"', {
					'xcode_settings': {
						'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
					}
				}]
			]
		}
	]
}
