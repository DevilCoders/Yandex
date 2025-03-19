{% set unit="fs_align_check" %}

fs_align_check:
  file.managed:
    - name: /usr/local/bin/fs_align_check.sh
    - source: salt://units/{{unit}}/fs_align_check.sh
    - user: root
    - group: root
    - mode: 755

  monrun.present:
    - command: "sudo /usr/local/bin/fs_align_check.sh"
    - execution_interval: 3600
    - execution_timeout: 20
    - type: common
