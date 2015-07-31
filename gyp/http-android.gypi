{
  'targets': [
    { 'target_name': 'http-android',
      'product_name': 'mbgl-http-android',
      'type': 'static_library',
      'standalone_static_library': 1,
      'hard_dependency': 1,

      'sources': [
        '../platform/android/http_request_android.cpp',
      ],

      'include_dirs': [
        '../include',
        '../src',
      ],

      'variables': {
        'cflags_cc': [
          '<@(uv_cflags)',
          '<@(curl_cflags)',
          '<@(boost_cflags)',
          '<@(zip_cflags)',
          '<@(openssl_cflags)',
        ],
        'ldflags': [
          '<@(uv_ldflags)',
          '<@(curl_ldflags)',
          '<@(zip_ldflags)',
        ],
        'libraries': [
          '<@(uv_static_libs)',
          '<@(curl_static_libs)',
          '<@(zip_static_libs)',
        ],
        'defines': [
          '-DMBGL_HTTP_CURL'
        ],
      },

      'conditions': [
        ['OS == "mac"', {
          'xcode_settings': {
            'OTHER_CPLUSPLUSFLAGS': [ '<@(cflags_cc)' ],
          },
        }, {
         'cflags_cc': [ '<@(cflags_cc)' ],
        }],
      ],

      'direct_dependent_settings': {
        'conditions': [
          ['OS == "mac"', {
            'xcode_settings': {
              'OTHER_CFLAGS': [ '<@(defines)' ],
              'OTHER_CPLUSPLUSFLAGS': [ '<@(defines)' ],
            }
          }, {
            'cflags': [ '<@(defines)' ],
            'cflags_cc': [ '<@(defines)' ],
          }]
        ],
      },

      'link_settings': {
        'conditions': [
          ['OS == "mac"', {
            'libraries': [ '<@(libraries)' ],
            'xcode_settings': { 'OTHER_LDFLAGS': [ '<@(ldflags)' ] }
          }, {
            'libraries': [ '<@(libraries)', '<@(ldflags)' ],
          }]
        ],
      },
    },
  ],
}
