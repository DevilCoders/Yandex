base:
  'kassa-salt*':
    - master
    - master-salt
  'salt*':
    - master
    - master-salt
  'c:kassa-load':
    - match: grain
    - kassa-load
  'c:kassa-dev':
    - match: grain
    - kassa-dev
  'c:kassa-prestable':
    - match: grain
    - kassa-prestable
