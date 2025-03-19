{% from slspath ~ "/map.jinja" import mongodb, walg with context %}
{% set service_file_path = "/lib/systemd/system/wal-g-oplog-push.service" %}

{% if walg.oplog_push %}

wal-g-oplog-push-service:
    file.managed:
        - name: {{ service_file_path }}
        - source: salt://{{ slspath }}/conf/wal-g-oplog-push.service
        - template: jinja
        - defaults:
            log_dir: {{ walg.logdir }}
            user: {{ walg.user }}
            group: {{ walg.group }}
        - require:
            - test: walg-ready
{%      for srv in mongodb.services_deployed if srv != 'mongos' %}
            - {{srv}}-replicaset-member
{%      endfor %}
        - onchanges_in:
            - module: systemd-reload

wal-g-oplog-push-service-running:
    service.running:
        - name: wal-g-oplog-push
        - enable: True
        - require:
            - file: wal-g-oplog-push-service
            - test: walg-ready
        - watch:
            - file: {{walg.confdir}}/wal-g.yaml

{% else %}

wal-g-oplog-push-service-absent:
    file.absent:
        - name: {{ service_file_path }}
        - onchanges_in:
            - module: systemd-reload

wal-g-oplog-push-service-dead:
    service.dead:
        - name: wal-g-oplog-push
        - enable: False
        - require_in:
            - file: wal-g-oplog-push-service-absent
        - onlyif:
            - stat {{ service_file_path }}

{% endif %}
