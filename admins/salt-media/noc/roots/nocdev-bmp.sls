include:
  - units.juggler-checks.common
  - units.juggler-checks.bmp

bmp-pkgs:
  pkg:
    - installed
    - pkgs:
      - pmacct
      - routinator
      - bmp-monitor

/var/cache/pmbmpd/:
  file.directory:
    - user: root
    - group: root
    - mode: 755

/etc/bmp_monitor:
  file.recurse:
    - source: salt://files/nocdev-bmp/etc/bmp_monitor
    - template: jinja

/etc/pmacct/pmbmpd.conf:
  file.managed:
    - source: salt://files/nocdev-bmp/etc/pmacct/pmbmpd.conf

/etc/routinator/routinator.conf:
  file.managed:
    - source: salt://files/nocdev-bmp/etc/routinator/routinator.conf

/etc/systemd/system/pmbmpd.service:
  file.managed:
    - source: salt://files/nocdev-bmp/etc/systemd/system/pmbmpd.service

bmp-services:
  service.running:
    - enable: True
      names:
      - pmbmpd
      - bmp_analyzer
      - bmp_importer
      - upflow_manager
      - routinator

/etc/logrotate.d/pmbmpd:
  file.managed:
    - source: salt://files/nocdev-bmp/etc/logrotate.d/pmbmpd
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/default/config-caching-dns:
  file.managed:
    - source: salt://files/nocdev-bmp/etc/default/config-caching-dns
