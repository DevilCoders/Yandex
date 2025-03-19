base:
  'c:alet-all':
    - match: grain
    - templates.usr_envs
    - common.monrun.salt-state
    - templates.media-common
    - templates.selfdns
    - templates.packages
    - yandex-cauth-conf
  'c:alet-dev-cores':
    - match: grain
    - templates.core-agg
  'c:alet-librarian-mongo':
    - match: grain
    - librarian-mongo
  'c:alet-stable-logstore':
    - match: grain
    - logstore
  'c:alet-librarian-redis':
    - match: grain
    - librarian-redis
  'c:alet-stable-smartpassproxy':
    - match: grain
    - smartpassproxy
