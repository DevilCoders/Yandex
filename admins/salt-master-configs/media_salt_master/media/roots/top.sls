base:
  '*':
    - templates.salt-minion
  '^salt0.*$':  # all salt masters
    - match: pcre
    - templates.salt-master
