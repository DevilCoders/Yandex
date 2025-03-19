{% set tables = ['sessions', 'statements'] %}

perf-diag-req:
    test.nop

/var/log/dbaas-my-perf-reporter:
    file.directory:
        - user: monitor
        - group: monitor
        - require:
            - test: perf-diag-req

/etc/pushclient/my-perf-reporter.secret:
    file.managed:
        - contents:
          - {{ salt.pillar.get('data:perf_diag:tvm_secret', 'secret') }}
        - makedirs: True
        - watch_in:
            - service: pushclient

/etc/pushclient/conf.d/topics-perfdiag-grpc.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/perf_diag/topics-perfdiag-grpc.conf
        - context:
            tables: {{ tables | tojson }}
        - makedirs: True
        - watch_in:
            - service: pushclient

/etc/pushclient/pushclient-perfdiag.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/perf_diag/pushclient-perfdiag.conf
        - template: jinja
        - makedirs: True
        - require:
            - test: perf-diag-req
            - file: /var/log/dbaas-my-perf-reporter
            - file: /etc/pushclient/my-perf-reporter.secret
            - file: /etc/pushclient/conf.d/topics-perfdiag-grpc.conf
        - watch_in:
            - service: pushclient

/var/lib/push-client-perfdiag:
    file.directory:
        - user: statbox
        - group: statbox
        - makedirs: True
        - require:
            - test: perf-diag-req

{% for table in tables %}
/etc/dbaas-cron/conf.d/my_{{ table }}.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/perf_diag/my_{{ table }}.conf
        - require:
            - test: perf-diag-req
            - file: /var/log/dbaas-my-perf-reporter
        - watch_in:
            - service: dbaas-cron

/etc/dbaas-cron/modules/my_{{ table }}.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/perf_diag/my_{{ table }}.py
        - require:
            - test: perf-diag-req
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
