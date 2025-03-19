base:
  '*':
    - minions
  '^kp-(test-|stable-)?salt.*':
    - match: pcre
    - master
    - master-salt

