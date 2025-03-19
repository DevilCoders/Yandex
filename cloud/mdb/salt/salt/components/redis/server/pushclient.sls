/etc/pushclient/redis_server_log_parser.py:
    file.managed:
        - source: salt://components/pushclient/parsers/redis_server_log_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient
        - watch_in:
            - service: pushclient

statbox-in-redis-group:
    group.present:
        - name: redis
        - addusers:
            - statbox
        - system: True
        - require:
            - user: statbox-user
        - require_in:
            - service: pushclient

monitor-not-in-redis-group:
    group.present:
        - name: redis
        - delusers:
            - monitor
        - system: True
        - require_in:
            - service: pushclient
        - require:
            - pkg: juggler-pgks
        - watch_in:
            - cmd: juggler-client-restart

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
            - pkg: pushclient
            - file: /etc/pushclient/redis_server_log_parser.py
            - file: /var/log/redis
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf
