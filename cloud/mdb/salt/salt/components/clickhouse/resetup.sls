clickhouse-resetup-req:
    mdb_clickhouse.resetup_required:
        - require:
            - file: /usr/local/yandex/ch_wait_started.py
            - test: ch-backup-config-ready
            - test: clickhouse-server-users-ready
            - test: clickhouse-server-config-ready
            - file: /usr/local/yandex/ch_wait_replication_sync.py
{% if salt.mdb_clickhouse.ssl_enabled() %}
            - file: /etc/clickhouse-server/ssl/server.crt
            - file: /etc/clickhouse-server/ssl/server.key
{%   if not salt.dbaas.is_public_ca() %}
            - file: ca_bundle
{%   endif %}
{% endif %}

/etc/clickhouse-server/config.d/resetup_config.xml:
    fs.file_present:
        - contents_function: mdb_clickhouse.render_server_resetup_config
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - onchanges:
            - mdb_clickhouse: clickhouse-resetup-req
        - require_in:
            - cmd: clickhouse-resetup-server-restart

/etc/clickhouse-client/config.d/resetup_config.xml:
    fs.file_present:
        - contents_function: mdb_clickhouse.render_client_resetup_config
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - onchanges:
            - mdb_clickhouse: clickhouse-resetup-req
        - require_in:
            - cmd: clickhouse-resetup-server-restart

clickhouse-resetup-server-restart:
    cmd.run:
        - name: >
            service clickhouse-server restart && /usr/local/yandex/ch_wait_started.py -q
        - onchanges:
            - mdb_clickhouse: clickhouse-resetup-req
        - require:
            - test: clickhouse-server-config-ready
            - test: clickhouse-server-users-ready
            - test: clickhouse-server-cluster-ready
        - require_in:
            - cmd: clickhouse-resetup-restore-schema

clickhouse-resetup-restore-schema:
    cmd.run:
        - name: >
            {{ salt.mdb_clickhouse.restore_schema_command() }}
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
        - onchanges:
            - mdb_clickhouse: clickhouse-resetup-req
        - require_in:
            - file: clickhouse-resetup-post-cleanup

{% if salt.pillar.get('data:clickhouse:sql_user_management', False) %}
{%     if salt.pillar.get('replica_schema_backup') %}
{%         if not salt.mdb_clickhouse.user_replication_enabled() %}
/etc/yandex/ch-backup/ch-backup-schema-copy.conf:
    fs.file_present:
        - contents_function: mdb_clickhouse.backup_config_for_schema_copy
        - contents_format: yaml
        - mode: 644
        - makedirs: True
        - onchanges:
            - mdb_clickhouse: clickhouse-resetup-req
        - require:
            - cmd: clickhouse-resetup-restore-schema
        - require_in:
            - cmd: clickhouse-resetup-restore-access-control
    file.absent:
        - onchanges:
            - mdb_clickhouse: clickhouse-resetup-req
        - require:
            - cmd: clickhouse-resetup-restore-access-control

clickhouse-resetup-restore-access-control:
    cmd.run:
        - name: >
            /usr/bin/ch-backup --config /etc/yandex/ch-backup/ch-backup-schema-copy.conf
            --port {{salt.mdb_clickhouse.resetup_ports_config()['https_port']}}
            restore-access-control {{ salt.pillar.get('replica_schema_backup') }}
        - env:
            - LC_ALL: C.UTF-8
            - LANG: C.UTF-8
        - onchanges:
            - mdb_clickhouse: clickhouse-resetup-req
        - require_in:
            - cmd: clickhouse-resetup-wait-replication-sync
{%         endif %}

clickhouse-resetup-wait-replication-sync:
    cmd.run:
        - name: >
            /usr/local/yandex/ch_wait_replication_sync.py
        - onchanges:
            - mdb_clickhouse: clickhouse-resetup-req
        - require:
            - cmd: clickhouse-resetup-restore-schema
        - require_in:
            - file: clickhouse-resetup-post-cleanup
{%     else %}
fail-if-sql-user-management-resetup:
    test.fail_without_changes:
        - onchanges:
            - mdb_clickhouse: clickhouse-resetup-req
        - require_in:
            - cmd: clickhouse-resetup-restore-schema
{%     endif %}
{% endif %}

clickhouse-resetup-post-cleanup:
    file.absent:
        - names:
            - /etc/clickhouse-server/config.d/resetup_config.xml
            - /etc/clickhouse-client/config.d/resetup_config.xml
        - require_in:
            - cmd: clickhouse-resetup-server-stop

clickhouse-resetup-server-stop:
    cmd.run:
        - name: >
            service clickhouse-server restart && /usr/local/yandex/ch_wait_started.py -q
        - onchanges:
            - mdb_clickhouse: clickhouse-resetup-req
        - require_in:
            - service: clickhouse-server
