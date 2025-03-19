/etc/pushclient/elasticsearch_log_parser.py:
    file.managed:
        - source: salt://components/pushclient/parsers/elasticsearch_log_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient
        - require_in:
            - file: /etc/pushclient/conf.d/topics-logs-grpc.conf

{% if salt.pillar.get('data:elasticsearch:kibana:enabled', False) %}
/etc/pushclient/kibana_log_parser.py:
    file.managed:
        - source: salt://components/pushclient/parsers/kibana_log_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient
        - require_in:
            - file: /etc/pushclient/conf.d/topics-logs-grpc.conf
{% endif %}

/etc/pushclient/conf.d/topics-logs-rt.conf:
    file.absent:
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client.conf

/etc/pushclient/conf.d/topics-logs-grpc.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/pushclient-topics-logs-grpc.conf
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf
