{% set unit = 'mds-distributed-flock' %}

/etc/distributed-flock.json:
  yafile.managed:
    - source: salt://{{ slspath }}/files/distributed-flock.json
    - template: jinja

zk-flock-{{ unit }}:
  pkg.installed:
    - pkgs:
      - zk-flock
