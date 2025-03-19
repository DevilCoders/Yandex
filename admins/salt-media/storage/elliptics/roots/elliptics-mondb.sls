include:
  - units.iface-ip-conf
  - templates.certificates

/etc/cron.d:
  file.recurse:
    - source: salt://files/elliptics-mondb/etc/cron.d/

/etc/rc.conf.local:
  file.managed:
    - source: salt://files/elliptics-mondb/etc/rc.conf.local

/etc/nginx/sites-enabled/00-default.conf:
  file.managed:
    - source: salt://files/elliptics-mondb/etc/nginx/sites-enabled/00-default.conf

/etc/clickhouse-server/config.xml:
  file.managed:
    - source: salt://files/elliptics-mondb/etc/clickhouse-server/config.xml
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

# Fix me
/etc/yandex-pkgver-ignore.d/mds-bionic-pkgver-ignore:
  file.managed:
    - contents: |
        libschedule-cron-events-perl
        nginx
        virt-what
        ipmimonitoring-metrika-yandex
    - user: root
    - group: root
    - mode: 644
