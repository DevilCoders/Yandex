include:
  - units.nginx_conf
  - templates.certificates
  - units.netconfig_slb
  - units.nginx_conf

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://files/nocdev-packfw/etc/nginx/sites-enabled/

topka-rbt:
  user.present:
    - createhome: True
    - system: True
    - home: /home/topka-rbt
    - groups:
      - root

/etc/topka/config.yaml:
  file.managed:
    - source: salt://files/nocdev-packfw/etc/topka/config.yaml
    - makedirs: True
    - template: jinja

/home/topka-rbt/.ssh/id_rsa:
  file.managed:
    - source: salt://files/nocdev-packfw/home/topka-rbt/.ssh/id_rsa
    - template: jinja
    - user: topka-rbt
    - group: root
    - mode: 600
    - makedirs: True
