/usr/bin/child_killer.sh:
  yafile.managed:
    - source: salt://{{ slspath }}/files/child_killer.sh
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

child_killer:
  monrun.present:
    - command: "/usr/bin/child_killer.sh"
    - execution_interval: 300
    - execution_timeout: 180
    - type: storage
