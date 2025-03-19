{% set tables = ['sessions', 'statements'] %}

perf-diag-req:
    test.nop

/var/log/dbaas-my-perf-reporter:
    file.absent

/etc/pushclient/my-perf-reporter.secret:
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

{% for table in tables %}
/etc/dbaas-cron/conf.d/my_{{ table }}.conf:
    file.absent:
        - watch_in:
            - service: dbaas-cron

/etc/dbaas-cron/modules/my_{{ table }}.py:
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
