/etc/pushclient/push-client-grpc.conf:
{% if salt.pillar.get('data:logship:enabled', True) %}
    file.managed:
        - source: salt://{{ slspath }}/conf/push-client-grpc.conf
        - template: jinja
        - makedirs: True
        - require:
            - file: /etc/pushclient/conf.d/topics-security-grpc.conf
            - pkg: pushclient
        - require_in:
{% if salt['pillar.get']('data:use_monrun', True) %}
            - file: /usr/local/yandex/monitoring/pushclient.py
{% else %}
            - file: /usr/local/yandex/telegraf/scripts/pushclient.py
{% endif %}
        - watch_in:
            - service: pushclient
{% else %}
    file.absent:
        - watch_in:
            - service: pushclient
{% endif %}

/etc/pushclient/conf.d/topics-security-grpc.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/topics-security-grpc.conf
        - template: jinja
        - makedirs: True
        - require:
            - pkg: pushclient

/etc/pushclient/grpc_logs.secret:
    file.managed:
{% if salt.pillar.get('data:logship:lb_producer_key') %}
        - contents_pillar: data:logship:lb_producer_key
{% elif salt.pillar.get('data:logship:tvm') %}
        - contents_pillar: data:logship:tvm:secret
{% elif salt.pillar.get('data:logship:oauth_token') %}
        - contents_pillar: data:logship:oauth_token
{% else %}
        - contents:
          - "secret"
{% endif %}
        - user: root
        - group: statbox
        - mode: '0640'
        - makedirs: True
        - require:
            - pkg: pushclient
        - watch_in:
            - service: pushclient

/var/lib/push-client-grpc:
    file.directory:
        - user: statbox
        - group: statbox
        - makedirs: True
        - require:
            - pkg: pushclient
            - user: statbox-user
        - require_in:
            - service: pushclient

/etc/pushclient/push-client.conf:
    file.absent:
        - watch_in:
            - service: pushclient

/etc/pushclient/conf.d/topics-security-rt.conf:
    file.absent:
        - watch_in:
            - service: pushclient

{% if salt.pillar.get('data:logship:use_cloud_logbroker') %}
/etc/pushclient/lb_producer_key.json:
    file.absent:
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf
{% endif %}
