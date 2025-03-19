shelves-sensors:
  monrun.present:
    - command: "sudo /usr/local/bin/shelves-sensors.py"
    - execution_interval: 600
    - execution_timeout: 60
    - type: storage

  file.managed:
    - name: /usr/local/bin/shelves-sensors.py
    - source: salt://templates/monrun-shelves-sensors/shelves-sensors.py
    - user: root
    - group: root
    - mode: 755

monrun_criticals:
  file.managed:
    - name: /etc/sudoers.d/monrun_shelves_sensors
    - source: salt://templates/monrun-shelves-sensors/monrun_shelves_sensors
    - user: root
    - group: root
    - mode: 440

modules_load_d_conf:
  file.managed:
    - name: /etc/modules-load.d/mptctl.conf
    - contents: |
        mptctl
    - user: root
    - group: root
    - mode: 644
