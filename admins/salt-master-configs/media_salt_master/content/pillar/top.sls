base:
  'c:content_dev':
    - match: grain
    - dev
    - renderer
  'c:content_test_salt':
    - match: grain
    - master
    - master-salt
  'c:content_unstable_all':
    - match: grain
    - unstable
    - renderer
  'c:(content_test_all|tv-test|kino-test)$':
    - match: grain_pcre
    - testing
    - renderer
  'c:content_load_salt':
    - match: grain
    - master
    - load-master
  'c:(content_load_all|tv-load|kino-load)$':
    - match: grain_pcre
    - load
    - renderer
  'c:content_salt':
    - match: grain
    - master
    - master-salt
  'c:(content-all-stable|kino-prod|tv-prod)$':
    - match: grain_pcre
    - stable
    - renderer
  'c:content-all-prestable':
    - match: grain
    - stable
    - renderer
