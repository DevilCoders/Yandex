include:
    - templates.certificates
    - templates.distributed-flock

/etc/solomonsyncer/config.yaml:
  file.managed:
    - source: salt://files/elliptics-l3mon/etc/solomonsyncer/config.yaml
    - template: jinja
    - makedirs: True

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://files/elliptics-l3mon/etc/nginx/sites-enabled/

/etc/cron.d:
  file.recurse:
    - source: salt://files/elliptics-l3mon/etc/cron.d/

/etc/rc.conf.local:
  file.managed:
    - source: salt://files/elliptics-l3mon/etc/rc.conf.local

/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://files/elliptics-l3mon/etc/logrotate.d/nginx

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

/etc/kitty/kitty.yaml:
  file.managed:
    - source: salt://files/elliptics-l3mon/etc/kitty/kitty.yaml
    - makedirs: True

/etc/natasha/natasha.yaml:
  file.managed:
    - source: salt://files/elliptics-l3mon/etc/natasha/natasha.yaml
    - template: jinja
    - makedirs: True
