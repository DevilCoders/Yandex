/etc/pushclient/mysql_error_log_parser.py:
    file.managed:
        - source: salt://components/pushclient/parsers/mysql_error_log_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient
            - file: /etc/pushclient/log_parser.py
        - watch_in:
            - service: pushclient

/etc/pushclient/mysql_general_log_parser.py:
    file.managed:
        - source: salt://components/pushclient/parsers/mysql_general_log_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient
            - file: /etc/pushclient/log_parser.py
        - watch_in:
            - service: pushclient

/etc/pushclient/mysql_slow_query_log_parser.py:
    file.managed:
        - source: salt://components/pushclient/parsers/mysql_slow_query_log_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient
            - file: /etc/pushclient/log_parser.py
        - watch_in:
            - service: pushclient

/etc/pushclient/mysql_audit_log_parser.py:
    file.managed:
        - source: salt://components/pushclient/parsers/mysql_audit_log_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient
            - file: /etc/pushclient/log_parser.py
        - watch_in:
            - service: pushclient

/etc/pushclient/secrets_regex:
    file.managed:
        - contents: |
            {{ salt['dbaas.password2regex'](salt['pillar.get']('data:mysql:users:admin:password')) }}
            {{ salt['dbaas.password2regex'](salt['pillar.get']('data:mysql:users:monitor:password')) }}
            {{ salt['dbaas.password2regex'](salt['pillar.get']('data:mysql:users:repl:password')) }}
        - user: statbox
        - mode: 400
        - makedirs: True
        - require:
            - pkg: pushclient
            - file: /etc/pushclient/log_parser.py
        - watch_in:
            - service: pushclient

statbox-in-mysql-group:
    group.present:
        - name: mysql
        - addusers:
            - statbox
        - system: True
        - require:
            - user: statbox-user
        - require_in:
            - service: pushclient

/etc/pushclient/conf.d/topics-logs-rt.conf:
    file.absent:
        - watch_in:
            - service: pushclient

/etc/pushclient/conf.d/topics-logs-grpc.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/pushclient-topics-logs-grpc.conf
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - file: /etc/pushclient/mysql_error_log_parser.py
            - file: /etc/pushclient/mysql_general_log_parser.py
            - file: /etc/pushclient/mysql_slow_query_log_parser.py
            - file: /etc/pushclient/mysql_audit_log_parser.py
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf
