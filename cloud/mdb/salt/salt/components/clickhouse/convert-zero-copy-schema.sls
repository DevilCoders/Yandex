{% set credentials = salt.mdb_clickhouse.get_zk_acl_credentials() %}
zero-copy-schema-converter:
    cmd.run:
        - name: >
            /usr/bin/zero_copy_schema_converter/zero_copy_schema_converter
            --hosts {{ salt.mdb_clickhouse.zookeeper_hosts_port() }}
{% if salt.mdb_clickhouse.zookeeper_is_secure() %}
            --secure
            --cert /etc/clickhouse-server/ssl/server.crt
            --key /etc/clickhouse-server/ssl/server.key
            --ca /etc/clickhouse-server/ssl/allCAs.pem
{% endif %}
{% if credentials[0] and credentials[1] %}
            --user {{ credentials[0] }}
            --password {{ credentials[1] }}
{% endif %}
            --root {{ salt.mdb_clickhouse.zookeeper_root() }}
            --zcroot clickhouse/zero_copy
            --timeout 15
{% if salt.pillar.get('convert_zero_copy_schema', '') == 'cleanup' %}
            --cleanup
{% endif %}
            --logfile /var/log/yandex/zero_copy_schema_converter.log
{% if salt.pillar.get('service-restart', False) %}
        - require:
            - cmd: clickhouse-stop
            - pkg: clickhouse-packages
{% endif %}
