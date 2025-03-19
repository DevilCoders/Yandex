/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://files/elliptics-rs/etc/nginx/sites-enabled/

/etc/cron.d:
  file.recurse:
    - source: salt://files/elliptics-rs/etc/cron.d/

/etc/rc.conf.local:
  file.managed:
    - source: salt://files/elliptics-rs/etc/rc.conf.local

/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://files/elliptics-valve/etc/logrotate.d/nginx

/etc/whiskas/whiskas.yaml:
  file.managed:
    - source: salt://files/elliptics-rs/etc/whiskas/whiskas.yaml
    - template: jinja
    - makedirs: True

# kostyl: fake racktables info to yandex-netconfig (add new string at 362 line)
/usr/lib/yandex-netconfig/ya-slb-tun:
  file.managed:
    - source: salt://files/elliptics-rs/ya-slb-tun

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
