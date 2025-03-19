include:
  - templates.ipvs_tun
  - templates.parsers
  - templates.yasmagent
  - common.monrun.salt-state
  - templates.certificates
  - templates.juggler-search
  - templates.unistat-lua
  - units.walle_juggler_checks
  - units.monitoring
  - units.mds-logbackup
  - units.yandex-hbf-agent-resizer
  - units.yarl

/etc/monrun/conf.d/valid_traffic.conf:
  file.managed:
    - source: salt://files/resizer-front/etc/monrun/conf.d/valid_traffic.conf
    - user: root
    - group: root
    - mode: 644

/usr/bin/valid_traffic.sh:
  file.managed:
    - source: salt://files/resizer-front/usr/bin/valid_traffic.sh
    - user: root
    - group: root
    - mode: 755
