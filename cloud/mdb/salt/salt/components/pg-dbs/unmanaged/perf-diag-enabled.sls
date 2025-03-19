{% set logdir = '/var/log/dbaas-pg-stat-reporter' %}
{% set tables = ['activity', 'statements'] %}

{{ logdir }}:
    file.directory:
        - user: monitor
        - group: monitor
        - watch_in:
            - service: dbaas-cron
            - service: pushclient

/etc/pushclient/pg-stat-reporter.secret:
    file.managed:
        - contents:
          - {{ salt.pillar.get('data:perf_diag:tvm_secret', 'secret') }}
        - makedirs: True
        - watch_in:
            - service: pushclient

/etc/pushclient/conf.d/topics-perfdiag-grpc.conf:
    file.managed:
        - template: jinja
        - source: salt://components/pg-dbs/unmanaged/conf/perf_diag/topics-perfdiag-grpc.conf
        - context:
            tables: {{ tables | tojson }}
        - makedirs: True
        - watch_in:
            - service: pushclient

/var/lib/push-client-perfdiag:
    file.directory:
        - user: statbox
        - group: statbox
        - makedirs: True
        - watch_in:
            - service: pushclient

/etc/pushclient/pushclient-perfdiag.conf:
    file.managed:
        - source: salt://components/pg-dbs/unmanaged/conf/perf_diag/pushclient-perfdiag.conf
        - template: jinja
        - makedirs: True
        - require:
            - file: /etc/pushclient/pg-stat-reporter.secret
            - file: /var/log/dbaas-pg-stat-reporter
            - file: /etc/pushclient/conf.d/topics-perfdiag-grpc.conf
        - watch_in:
            - service: pushclient

/var/log/dbaas-pg-stat-reporter/pg-stat-statements-query.log:
    file.absent

{% for table in tables %}
/etc/pushclient/pg_stat_{{ table }}.conf:
    file.absent

/var/lib/push-client-pg_stat_{{ table }}:
    file.absent:
        - require:
            - file: /etc/pushclient/pushclient-perfdiag.conf
            - service: pushclient

/etc/dbaas-cron/conf.d/pg_stat_{{ table }}.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/perf_diag/pg_stat_{{ table }}.conf
        - require:
            - file: {{ logdir }}
        - watch_in:
            - service: dbaas-cron
/etc/dbaas-cron/modules/pg_stat_{{ table }}.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/perf_diag/pg_stat_{{ table }}.py
        - template: jinja
        - watch_in:
            - service: dbaas-cron
{% endfor %}

/etc/pushclient/conf.d/topics-perf-diag-grpc.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/perf_diag/pushclient-topics-perf-diag-grpc.conf.enabled
        - template: jinja
        - mode: 755
        - makedirs: True
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf
