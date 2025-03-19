include:
  - units.iface-ip-conf
  - templates.certificates
  - templates.unistat-lua-base
  - units.netconfig_slb
  - units.nginx_conf

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://files/elliptics-valve/etc/nginx/sites-enabled/
    - template: jinja

/usr/sbin/valve_check.py:
  file.managed:
    - source: salt://files/elliptics-valve/usr/sbin/valve_check.py
    - mode: 755

/usr/sbin/weight.sh:
  file.managed:
    - source: salt://files/elliptics-valve/usr/sbin/weight.sh
    - mode: 755

/etc/nginx/conf.d:
  file.recurse:
    - source: salt://files/elliptics-valve/etc/nginx/conf.d/

/etc/monrun/conf.d:
  file.recurse:
    - source: salt://files/elliptics-valve/etc/monrun/conf.d/

/etc/valve/valve.yaml:
  pkg.installed:
    - pkgs:
      - python3-psycopg2
      - python3-requests
      - python3-dateutil
  file.managed:
    - source: salt://files/elliptics-valve/etc/valve/valve.yaml
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/root/.postgresql/root.crt:
  file.managed:
    - source: salt://files/elliptics-valve/root/.postgresql/root.crt
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/default/yasmagent:
  file.managed:
    - source: salt://files/elliptics-valve/etc/default/yasmagent

/usr/local/yasmagent/CONF/agent.valve.conf:
  file.managed:
    - source: salt://files/elliptics-valve/usr/local/yasmagent/CONF/agent.valve.conf
    - makedirs: True

/etc/valve/env:
  file.managed:
    - source: salt://files/elliptics-valve/etc/valve/env
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/logrotate.d/valve:
  file.managed:
    - source: salt://files/elliptics-valve/etc/logrotate.d/valve

/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://files/elliptics-valve/etc/logrotate.d/nginx

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
