include:
  - templates.nginx
  - .backend
  - .nginx

/usr/lib/yandex/icecream/backend/cert/lxd.crt:
  file.managed:
    - makedirs: True
    - dir_mode: 700
    - mode: 0400
    - user: root
    - group: root
    - contents: {{ salt['pillar.get']('yav:lxd.crt') | json }}

/usr/lib/yandex/icecream/backend/cert/lxd.key:
  file.managed:
    - makedirs: True
    - dir_mode: 700
    - mode: 0400
    - user: root
    - group: root
    - contents: {{ salt['pillar.get']('yav:lxd.key') | json }}

/etc/nginx/ssl/i.yandex-team.ru.pem:
  file.managed:
    - makedirs: True
    - dir_mode: 700
    - mode: 0400
    - user: root
    - group: root
    - contents: {{ salt['pillar.get']('yav:i.yandex-team.ru.pem') | json }}

/etc/nginx/ssl/ice.yandex-team.ru.pem:
  file.managed:
    - makedirs: True
    - dir_mode: 700
    - mode: 0400
    - user: root
    - group: root
    - contents: {{ salt['pillar.get']('yav:ice.yandex-team.ru.pem') | json }}

# pip packages, apt packages
python-pip-packages:
  pkg.installed:
    - pkgs:
      - python-pip
      - python3-pip
dependencies:
  pip.installed:
    - names:
      - python-blackboxer
      - conductor-client>0.1.7
    - bin_env: '/usr/bin/pip3'
    - index_url: https://pypi.yandex-team.ru/simple
    - require:
      - pkg: python-pip-packages

/etc/rc.conf.local:
  file.managed:
    - mode: 0440
    - user: root
    - group: root
    - contents: |
        ya_slb_enable="YES"
        ya_slb6_enable="YES"
        ya_slb_tunnel_enable="YES"
        ya_slb6_tunnel_enable="YES"
