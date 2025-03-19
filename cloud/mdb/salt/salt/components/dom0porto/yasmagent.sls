yasmagent-packages:
    pkg.installed:
        - pkgs:
            - python-psycopg2
            - python3-psycopg2
            - python-pymongo
            - python-bson
        - require_in:
            - pkg: yasmagent
        - prereq_in:
            - cmd: repositories-ready

{% set accumulator_filename = '/lib/systemd/system/yasmagent.service' %}
yasmagent-instance-getter-config:
    file.accumulated:
        - name: yasmagent-instance-getter
        - filename: {{ accumulator_filename }}
        - text: '/usr/local/yasmagent/dom0porto_getter.py'
        - require_in:
            - file: {{ accumulator_filename }}

yasmagent-instance-getter:
    file.managed:
        - name: /usr/local/yasmagent/dom0porto_getter.py
        - template: jinja
        - source: salt://{{ slspath }}/conf/dom0porto_getter.py
        - mode: 755
        - require:
            - pkg: yasmagent
        - watch_in:
            - service: yasmagent

put_server_info_in_etc:
    schedule.present:
        - function: state.apply
        - job_args:
            - components.dom0porto.server_info
        - seconds: 1200
        - splay: 1200

/etc/cron.d/yasmagent-porto-fix:
    file.managed:
        - source: salt://{{ slspath }}/conf/yasmagent-porto-fix.cron
