{
  'targets': [
    {
      'target_name': 'heapdump',
      'sources': ['src/heapdump.cc'],
      'conditions': [
        ['OS=="win"', {
          'sources': ['src/platform-win32.cc'],
        }, {
          'sources': ['src/platform-posix.cc'],
        }]
      ]
    }
  ]
}
