/etc/clickhouse-server/ssl:
    file.directory:
        - user: root
        - group: clickhouse
        - makedirs: True
        - mode: 755

/etc/clickhouse-server/ssl/server.key:
    file.managed:
        - contents_pillar: cert.key
        - template: jinja
        - user: root
        - group: clickhouse
        - mode: 640
        - require:
            - file: /etc/clickhouse-server/ssl
        - require_in:
            - service: clickhouse-server

/etc/clickhouse-server/ssl/server.crt:
    file.managed:
        - contents_pillar: cert.crt
        - template: jinja
        - user: root
        - group: clickhouse
        - mode: 644
        - require:
            - file: /etc/clickhouse-server/ssl
        - require_in:
            - service: clickhouse-server

{% set ca_key = 'letsencrypt_cert.ca' if salt.get('dbaas.is_aws')() else 'cert.ca' %}
ca_bundle:
    file.managed:
        - name: {{ salt.mdb_clickhouse.ca_path() }}
        - contents_pillar: {{ ca_key }}
        - template: jinja
        - user: root
        - group: clickhouse
        - mode: 644
        - require:
            - file: /etc/clickhouse-server/ssl
        - require_in:
            - service: clickhouse-server

{% if salt.get('mdb_clickhouse.version_ge')('22.2') %}
clickhouse-reload-config-for-cert:
    mdb_clickhouse.reload_config:
        - onchanges:
            - file: /etc/clickhouse-server/ssl/server.key
            - file: /etc/clickhouse-server/ssl/server.crt
{% endif %}
