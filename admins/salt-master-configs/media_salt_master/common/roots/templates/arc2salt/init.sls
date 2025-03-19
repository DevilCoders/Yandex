{% set salt_master = salt['pillar.get']('master') %}
{% set arc2salt = salt['pillar.get']('arc2salt') %}

/etc/yandex/salt/arc2salt.yaml:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - source: salt://{{ slspath }}/files/arc2salt.yaml
    - template: jinja

/usr/local/bin/arc2salt:
  file.managed:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
    - source: salt://{{ slspath }}/files/arc2salt.py

/etc/cron.d/arc2salt:
  file.managed:
    - source: salt://{{ slspath }}/files/cron
    - template: jinja

/etc/logrotate.d/arc2salt:
  file.managed:
    - source: salt://{{ slspath }}/files/logrotate

/var/log/arc2salt.log:
  file.managed:
    - user:  {{ salt_master['user'] }}

/home/{{ salt_master.user }}/.arc/token:
  file.managed:
    - mode: 0600
    - makedirs: True
    - user: {{ salt_master['user'] }}
    - group: dpt_virtual_robots
    - contents: {{ arc2salt['arc-token'] | json }}

/usr/lib/monrun/checks/arc2salt.sh:
  file.managed:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
    - source: salt://{{ slspath }}/files/monrun/arc2salt.sh

/etc/monrun/conf.d/arc2salt.conf:
  file.managed:
    - user: root
    - group: root
    - source: salt://{{ slspath }}/files/monrun/arc2salt.conf

arc2salt_pkgs:
  pkg.installed:
    - pkgs:
      - python3-requests
      - yandex-arc-launcher
