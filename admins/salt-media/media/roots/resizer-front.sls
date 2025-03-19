include:
   - templates.ipvs_tun
   - templates.parsers
   - templates.yasmagent
   - templates.mds-logbackup
   - common.monrun.salt-state

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

/etc/mds-logbackup.ignore-logs:
  file.managed:
    - source: salt://files/resizer-front/etc/mds-logbackup.ignore-logs
    - user: root
    - group: root
    - mode: 644
