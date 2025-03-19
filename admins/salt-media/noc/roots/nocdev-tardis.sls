include:
  - units.nginx_conf
  - units.juggler-checks.common

robot-tardis:
  user.present:
    - createhome: True
    - system: True
    - home: /home/robot-tardis

/home/robot-tardis/.ssh/id_rsa:
  file.managed:
    - source: salt://files/nocdev-tardis/home/robot-tardis/.ssh/id_rsa
    - template: jinja
    - user: robot-tardis
    - group: root
    - mode: 600
    - makedirs: True
