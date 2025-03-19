{% from "components/greenplum/map.jinja" import gpdbvars with context %}
{% if gpdbvars.version|int == 6175 and gpdbvars.patch_level|int >= 62 or gpdbvars.version|int >= 6193 %}
statbox-in-gpadmin-group:
    group.present:
        - name: gpadmin
        - addusers:
            - statbox
        - system: True
        - watch_in:
            - service: pushclient
        - require_in:
            - service: pushclient
        - require:
            - user: statbox-user

/etc/pushclient/conf.d/topics-logs-grpc.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/pushclient/pushclient-topics-logs-grpc.conf
        - template: jinja
        - mode: 755
        - makedirs: True
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf

/etc/pushclient/greenplum_csv_parser.py:
    file.managed:
        - source: salt://components/pushclient2/parsers/greenplum_csv_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - watch_in:
            - service: pushclient
        - require:
            - pkg: pushclient
{% else %}
{# MDB-17289: for versions <= 6.17.3.68 this file should always be otherwise the pushclient will not start #}
stub.topics-logs-grpc.conf:
    file.managed:
        - name: /etc/pushclient/conf.d/topics-logs-grpc.conf
        - contents: ''
        - mode: 755
        - makedirs: True
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf
{% endif %}
