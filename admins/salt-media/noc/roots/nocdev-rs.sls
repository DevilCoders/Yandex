include:
  - units.juggler-checks.common

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://files/nocdev-rs/etc/nginx/sites-enabled/

/etc/cron.d:
  file.recurse:
    - source: salt://files/nocdev-rs/etc/cron.d/

/etc/rc.local:
  file.managed:
    - source: salt://files/nocdev-rs/etc/rc.local
    - mode: 755

whiskas:
  service.running:
    - enable: True

nginx:
  service.running:
    - enable: True

/etc/whiskas/whiskas.yaml:
  file.managed:
    - source: salt://files/nocdev-rs/etc/whiskas/whiskas.yaml
    - template: jinja
    - makedirs: True

# kostyl: fake racktables info to slb yandex-netconfig (add new string at 362 line)
/etc/network/ya-slb-tun.sh:
  file.managed:
    - source: salt://files/nocdev-rs/ya-slb-tun.sh
    - mode: 755
