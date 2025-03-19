/etc/distributed-flock.json:
  file.managed:
    - source: salt://units/distributed-flock/distributed-flock.json
    - template: jinja

zk-flock:
  pkg:
    - installed
    - pkgs:
      - zk-flock
