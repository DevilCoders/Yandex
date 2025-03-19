/etc/squid3/squid.conf:
  file.managed:
    - source: salt://{{slspath}}/files/squid.conf
    - replace: true
    - user: root
    - group: root
    - mode: 644
    - watch_in:
      - service: squid3

/etc/logrotate.d/squid3:
  file.managed:
    - source: salt://{{slspath}}/files/squid.logrotate
    - user: root
    - group: root
    - mode: 644
    - watch_in:
      - service: squid3

squid3:
  pkg.installed:
    - pkgs:
      - squid3
      - squidclient
  service.running:
    - enable: True
    - reload: True

/usr/bin/monrun_squid.sh:
  file.managed:
    - source: salt://{{slspath}}/files/monrun_squid.sh
    - user: root
    - group: root
    - mode: 755

/etc/codechecker_squid_5xx.conf:
  file.managed:
    - source: salt://{{slspath}}/files/codechecker_squid_5xx.conf
    - user: root
    - group: root
    - mode: 644

/etc/monrun/conf.d/monrun_squid.conf:
  file.managed:
    - source: salt://{{slspath}}/files/monrun_squid.conf
    - user: root
    - group: root
    - mode: 644

/etc/monrun/conf.d/monrun_squid_5xx.conf:
  file.managed:
    - source: salt://{{slspath}}/files/monrun_squid_5xx.conf
    - user: root
    - group: root
    - mode: 644

regenerate-monrun-tasks:
  cmd.wait:
    - name: regenerate-monrun-tasks
    - watch:
      - file: /usr/bin/monrun_squid.sh
    - watch:
      - file: /etc/monrun/conf.d/monrun_squid.conf
    - watch:
      - file: /etc/monrun/conf.d/monrun_squid_5xx.conf

/usr/lib/yandex-graphite-checks/enabled/squid_util.sh:
  file.managed:
    - source: salt://{{slspath}}/files/squid_util.sh
    - user: root
    - group: root
    - mode: 754

/etc/yandex-pkgver-ignore-file:
  file.managed:
    - source: salt://{{slspath}}/files/yandex-pkgver-ignore-file
    - user: root
    - group: root
    - mode: 644
