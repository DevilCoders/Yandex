include:
    - components.clickhouse.configs.dictionaries
    - components.clickhouse.configs.dictionaries-reload
    - components.clickhouse.configs.s3-credentials
    - components.clickhouse.configs.server-config
    - components.clickhouse.configs.server-cluster
    - components.clickhouse.configs.server-users
    - components.clickhouse.configs.reload
    - components.clickhouse.clickhouse-cleaner.config
    - components.clickhouse.geobase
    - components.clickhouse.sql-users
    - components.clickhouse.service
{% if not salt.dbaas.is_aws() %}
    - components.clickhouse.pushclient
    - components.pushclient2.service
    - components.monrun2.update-jobs
    - components.monrun2.clickhouse.config
{% endif %}
{% if salt.pillar.get('update-firewall', False) %}
    - components.firewall.reload
    - components.clickhouse.firewall
{% endif %}
{% if salt.pillar.get('service-restart') %}
    - components.clickhouse.restart

extend:
    clickhouse-server-config-ready:
        test.nop:
            - require_in:
                - cmd: clickhouse-stop

    clickhouse-server-cluster-ready:
        test.nop:
            - require_in:
                - cmd: clickhouse-stop

    clickhouse-server-users-ready:
        test.nop:
            - require_in:
                - cmd: clickhouse-stop

    clickhouse-dictionaries-ready:
        test.nop:
            - require_in:
                - cmd: clickhouse-stop

    clickhouse-geobase-ready:
        test.nop:
            - require_in:
                - cmd: clickhouse-stop
{% endif %}
{% if salt.pillar.get('update-geobase', False) %}
{%     if salt.mdb_clickhouse.custom_geobase_uri() %}

extend:
    custom_geobase_archive:
        fs.file_present:
            - replace: True

delete_old_custom_geobase:
    file.absent:
        - name: {{ salt.mdb_clickhouse.custom_geobase_path() }}
        - require_in:
            - archive: custom_geobase
{%     endif %}
{% endif %}
