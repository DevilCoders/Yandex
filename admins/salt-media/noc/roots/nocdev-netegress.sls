include:
  - units.nginx_conf
  - units.juggler-checks.common
  - templates.certificates
 
/etc/yandex-pkgver-ignore.d/egress:
  file.managed:
    - source: salt://files/nocdev-netegress/etc/yandex-pkgver-ignore.d/egress
    - makedirs: True

/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://files/nocdev-netegress/etc/nginx/nginx.conf

/etc/nginx/sites-enabled/default:
  file.managed:
    - source: salt://files/nocdev-netegress/etc/nginx/sites-enabled/default

/etc/rc.local:
  file.managed:
    - source: salt://files/nocdev-netegress/etc/rc.local
    - mode: 755

/etc/sysctl.d/:
  file.recurse:
    - source: salt://files/nocdev-netegress/etc/sysctl.d/

/etc/network/interfaces:
  file.append:
    - text:
      - "source /etc/network/interfaces.d/*.cfg"

/etc/network/interfaces.d/90-tun.cfg:
  file.managed:
    - source: salt://files/nocdev-netegress/etc/network/interfaces.d/90-tun.cfg
    - template: jinja
    - makedirs: True

/var/www/html/:
  file.recurse:
    - source: salt://files/nocdev-netegress/var/www/html/
    - makedirs: True

/etc/telegraf/telegraf.conf:
  file.managed:
    - source: salt://files/nocdev-netegress/etc/telegraf/telegraf.conf
    - makedirs: True

/etc/systemd/system/nginx.service.d/override.conf:
  file.managed:
    - makedirs: True
    - source: salt://files/nocdev-netegress/etc/systemd/system/nginx.service.d/override.conf
