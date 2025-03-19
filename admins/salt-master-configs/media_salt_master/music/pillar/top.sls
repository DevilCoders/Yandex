base:
  'salt*':
    - master-salt
    - master
  '*':
    - minion-salt
