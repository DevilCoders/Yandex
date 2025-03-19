{% set logdir = '/var/log/dbaas-pg-stat-reporter' %}
{% set tables = ['activity', 'statements'] %}

{{ logdir }}:
    file.absent

/etc/pushclient/pg-stat-reporter.secret:
    file.absent:
        - watch_in:
            - service: pushclient

/etc/pushclient/conf.d/topics-perfdiag-grpc.conf:
    file.absent:
        - watch_in:
            - service: pushclient

/etc/pushclient/pushclient-perfdiag.conf:
    file.absent:
        - watch_in:
            - service: pushclient

/var/lib/push-client-perfdiag:
    file.absent

/var/log/dbaas-pg-stat-reporter/pg-stat-statements-query.log:
    file.absent

{% for table in tables %}
/etc/pushclient/pg_stat_{{ table }}.conf:
    file.absent

/var/lib/push-client-pg_stat_{{ table }}:
    file.absent

/etc/dbaas-cron/conf.d/pg_stat_{{ table }}.conf:
    file.absent:
        - watch_in:
            - service: dbaas-cron
/etc/dbaas-cron/modules/pg_stat_{{ table }}.py:
    file.absent:
        - watch_in:
            - service: dbaas-cron
{% endfor %}

/etc/pushclient/conf.d/topics-perf-diag-grpc.conf:
    file.managed:
        - source: salt://components/pushclient2/conf/pushclient-topics-perf-diag-grpc.conf.default
        - template: jinja
        - mode: 755
        - makedirs: True
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf
