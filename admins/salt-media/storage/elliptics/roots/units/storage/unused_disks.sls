/usr/bin/find_unused_disks.py:
  yafile.managed:
    - source: salt://files/storage/find_unused_disks.py
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

unused_disks:
  monrun.present:
    - command: "/usr/bin/find_unused_disks.py"
    - execution_interval: 300
    - execution_timeout: 180
    - type: storage
