{
  'includes': [
    '../gyp/common.gypi',
  ],
  'targets': [
    { 'target_name': 'qtapp',
      'product_name': 'qmapbox-gl',
      'type': 'executable',

      'dependencies': [
        '../mbgl.gyp:core',
        '../mbgl.gyp:platform-<(platform_lib)',
        '../mbgl.gyp:http-<(http_lib)',
        '../mbgl.gyp:asset-<(asset_lib)',
        '../mbgl.gyp:cache-<(cache_lib)',
        '../mbgl.gyp:copy_styles',
        '../mbgl.gyp:copy_certificate_bundle',
      ],

      'sources': [
        'main.cpp',
        '../include/mbgl/platform/qt/qmapboxgl.hpp',
        '../platform/default/default_styles.cpp',
        '../platform/default/log_stderr.cpp',
        '../platform/qt/qmapboxgl.cpp',
        '../platform/qt/qmapboxgl_p.hpp',
      ],

      'cflags_cc': [
        '<@(qt_cflags)',
        '-Wno-error'
      ],

      'libraries': [
        '<@(qt_ldflags)'
      ],
      'rules': [
        {
          'rule_name': 'MOC files',
          'extension': 'hpp',
          'process_outputs_as_sources': 1,
          'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/<(RULE_INPUT_DIRNAME)/moc_<(RULE_INPUT_ROOT).cpp' ],
          'action': [ '<(qt_moc)', '<(RULE_INPUT_PATH)', '-o', '<(SHARED_INTERMEDIATE_DIR)/<(RULE_INPUT_DIRNAME)/moc_<(RULE_INPUT_ROOT).cpp' ],
          'message': 'Generating MOC <(RULE_INPUT_ROOT).cpp',
        },
      ],
    },
  ],
}
