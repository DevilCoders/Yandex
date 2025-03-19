{% set unit = 'wd-mastermind-stat' %}

wd-mastermind-stat-pkg:
  pkg.installed:
      - name: jq

/usr/bin/wd-mastermind-stat.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/wd-mastermind-stat.sh
    - user: root
    - group: root
    - mode: 755

/etc/cron.d/wd-mastermind-stat:
  file.managed:
    - source: salt://{{ slspath }}/files/wd-mastermind-stat.cron
    - user: root
    - group: root
    - mode: 644

# MDS-7756
scheduler-status:
  file.managed:
    - name: /usr/bin/mm_scheduler_monitor.py
    - source: salt://{{ slspath }}/files/mm_scheduler_monitor.py
    - user: root
    - group: root
    - mode: 755

  monrun.present:
    - command: "/usr/bin/mm_scheduler_monitor.py"
    - execution_interval: 300
    - execution_timeout: 240
    - type: mastermind
