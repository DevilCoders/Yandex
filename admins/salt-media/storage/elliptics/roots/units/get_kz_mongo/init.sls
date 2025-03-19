{% set cluster = pillar.get('cluster') %}
{% set unit = 'get_kz_mongo' %}

# TODO: move to unit
mastermind-lock-check-script:
  file.managed:
    - name: /usr/local/bin/get_kz_mongo.py
    - source: salt://units/{{ unit }}/files/usr/bin/get_kz_mongo.py
    - user: root
    - group: root
    - mode: 755

mastermind-lock-old:
  monrun.present:
    - command: "/usr/local/bin/get_kz_mongo.py -a lock -p planner --min-depth 2"
    - execution_interval: 600
    - execution_timeout: 120
    - type: mastermind

mastermind-lock-old-jobs:
  monrun.present:
    - command: "/usr/local/bin/get_kz_mongo.py -a lock -p job"
    - execution_interval: 600
    - execution_timeout: 120
    - type: mastermind

mastermind-locks-lost:
  monrun.present:
    - command: zk-flock -x 33 elliptics-cloud-monrun-mastermind-locks-lost '/usr/local/bin/get_kz_mongo.py -p couple group fs -a job'; [ $? -eq 33 ] && echo '0;lock busy'
    - execution_interval: 3600
    - execution_timeout: 1800
    - type: mastermind
