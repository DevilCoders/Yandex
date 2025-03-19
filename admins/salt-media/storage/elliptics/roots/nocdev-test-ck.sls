include:
  - templates.certificates
  - units.netconfig_slb
  - units.nginx_conf

/etc/noc-ck/:
  file.recurse:
    - source: salt://files/nocdev-test-ck/etc/noc-ck/
    - template: jinja
    - user: root
    - group: root
    - file_mode: 644
    - makedirs: True

robot-noc-ck:
  user.present:
    - createhome: True
    - system: True
    - home: /home/robot-noc-ck
    - groups:
      - root
  pkg.installed:
    - pkgs:
      - python3.7
    - skip_suggestions: True
    - install_recommends: True

/home/robot-noc-ck/.ssh/id_rsa:
  file.managed:
    - source: salt://files/nocdev-test-ck/home/robot-noc-ck/.ssh/id_rsa
    - template: jinja
    - user: robot-noc-ck
    - group: root
    - mode: 600
    - makedirs: True

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://files/nocdev-ck/etc/nginx/sites-enabled/
    - template: jinja
