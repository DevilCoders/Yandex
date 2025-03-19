{% from "components/postgres/pg.jinja" import pg with context %}
statbox-in-postgres-group:
    group.present:
        - name: postgres
        - addusers:
            - statbox
        - system: True
        - watch_in:
            - service: pushclient
        - require_in:
            - service: pushclient
        - require:
            - user: statbox-user

/etc/pushclient/pgbouncer_parser.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/pushclient/pgbouncer_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - watch_in:
            - service: pushclient
        - require:
            - pkg: pushclient

/etc/pushclient/postgres_csv_parser.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/pushclient/postgres_csv_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - watch_in:
            - service: pushclient
        - require:
            - pkg: pushclient

/etc/pushclient/conf.d/topics-logs-rt.conf:
    file.absent:
        - watch_in:
            - service: pushclient

/etc/pushclient/conf.d/topics-logs-grpc.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/pushclient/pushclient-topics-logs-grpc.conf
        - template: jinja
        - mode: 755
        - makedirs: True
        - context:
            pg: {{ pg | tojson }}
        - require:
            - file: /etc/pushclient/pgbouncer_parser.py
            - file: /etc/pushclient/postgres_csv_parser.py
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf
