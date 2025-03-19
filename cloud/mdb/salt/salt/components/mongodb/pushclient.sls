{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}
{% set parser = 'mongodb_log_parser.py' %}
{% set mongodb_version = salt.mdb_mongodb.version() %}
{% if (mongodb_version.major_num | int) >= 404 %}
{%   set parser = 'mongodb44_log_parser.py' %}
{% endif %}

/etc/pushclient/mongodb_log_parser.py:
    file.managed:
        - source: salt://components/pushclient/parsers/{{parser}}
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient

statbox-in-mongodb-group:
    group.present:
        - name: mongodb
        - addusers:
            - statbox
        - system: True
        - require:
            - user: statbox-user

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
        - context:
            mongodb: {{mongodb | tojson}}
        - require:
            - pkg: pushclient
            - file: /etc/pushclient/mongodb_log_parser.py
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf
