mds_8821:
  monrun.present:
    - command: "/usr/bin/mds-8821.py"
    - execution_interval: 300
    - execution_timeout: 200

  file.managed:
    - name: /usr/bin/mds-8821.py
    - source: salt://files/storage/mds-8821.py
    - user: root
    - group: root
    - mode: 755
