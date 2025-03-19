{% set ship_logs = salt.pillar.get('data:ship_logs', False) %}

clickhouse-pushclient-req:
    test.nop

clickhouse-pushclient-ready:
    test.nop:
        - watch_in:
            - service: pushclient

/etc/pushclient/clickhouse_server_log_parser.py:
    file.managed:
        - source: salt://components/pushclient/parsers/clickhouse_server_log_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - test: clickhouse-pushclient-req
        - watch_in:
            - test: clickhouse-pushclient-ready

statbox-in-clickhouse-group:
    group.present:
        - name: clickhouse
        - addusers:
            - statbox
        - system: True
        - require:
            - test: clickhouse-pushclient-req
        - watch_in:
            - test: clickhouse-pushclient-ready

/etc/pushclient/conf.d/topics-logs-rt.conf:
    file.absent:
        - watch_in:
            - service: pushclient

/etc/pushclient/conf.d/topics-logs-grpc.conf:
    file.managed:
{%     if ship_logs %}
        - source: salt://{{ slspath }}/conf/pushclient-topics-logs-grpc.conf
        - template: jinja
{%     else %}
        - contents: ''
{%     endif %}
        - mode: 755
        - makedirs: True
        - require:
            - test: clickhouse-pushclient-req
        - watch_in:
            - test: clickhouse-pushclient-ready
