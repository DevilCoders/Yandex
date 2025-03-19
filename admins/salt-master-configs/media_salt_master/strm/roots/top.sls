base:
  '*':
    - minion
  'strm-.*salt-\d+':  # all salt masters
    - match: pcre
    - master
  'salt\d+[a-z]+':  # all salt masters
    - match: pcre
    - master
  'c:strm-salt':  # all salt masters
    - match: grain
    - master
  'c:strm-test-salt':  # all salt masters
    - match: grain
    - master
