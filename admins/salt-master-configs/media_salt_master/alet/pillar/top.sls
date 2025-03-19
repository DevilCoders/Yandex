base:
  'salt*':
    - master
    - master-salt
  'alet-salt*':
    - master
    - master-salt
  'c:alet-dev':
    - match: grain
    - alet-dev
