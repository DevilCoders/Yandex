/etc/nginx/sites-enabled/netmap:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/netmap
    - template: jinja

/etc/netmap/config.yaml:
  file.managed:
    - user: robot-netmap
    - group: root
    - mode: 600
    - source: salt://{{ slspath }}/files/etc/netmap/config.yaml
    - template: jinja

/home/robot-netmap/.ssh/id_rsa:
  file.managed:
    - user: robot-netmap
    - group: root
    - mode: 400
    - dir_mode: 700
    - makedirs: True
    - contents: {{ pillar['robot_netmap']['private'] | json }}

/netmap:
  file.directory:
    - makedirs: True
    - user: robot-netmap
    - mode: 0755
