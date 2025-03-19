include:
  - units.nginx_conf
  - templates.certificates
  - units.netconfig_slb

nginx:
  service.running:
    - enable: True
    - reload: True
    - watch:
      - file: /etc/nginx/*

/etc/checkist/config.json:
  file.managed:
    - source: salt://files/nocdev-4k/etc/checkist/config.json
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/usr/bin/python3:
  pkg.installed:
    - pkgs:
      - python3.7
      - python3.7-dev
      - python3.7-minimal
      - python3.7-venv
    - skip_suggestions: True
    - install_recommends: True

  file.symlink:
    - target: /usr/bin/python3.7

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://files/nocdev-4k/etc/nginx/sites-enabled/
    - template: jinja

/etc/default:
  file.recurse:
    - source: salt://files/nocdev-4k/etc/default/
