/etc/pushclient/conf.d/topics-logs-grpc.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/pushclient-topics-logs-grpc.conf
        - template: jinja
        - mode: 755
        - makedirs: True
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf
