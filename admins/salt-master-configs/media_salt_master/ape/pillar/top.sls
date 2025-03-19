base:
  'c:ape-test-salt':
    - match: grain
    - master
    - storage-salt-master
  'c:ape-salt':
    - match: grain
    - master
    - storage-salt-master
  'c:ape-prestable':
    - ape-prestable
    - match: grain
  'c:ape-test':
    - storage-salt-minion
    - match: grain
  'c:ape-prod':
    - storage-salt-minion
    - match: grain
