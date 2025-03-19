{% set sql_user_management = salt.pillar.get('data:clickhouse:sql_user_management', False) %}
{% set user_management_v2  = salt.pillar.get('data:clickhouse:user_management_v2', False) %}

clickhouse-sql-users-req:
    test.nop

clickhouse-sql-users-ready:
    test.nop

{% if sql_user_management %}
ensure_admin_user:
    mdb_clickhouse.ensure_admin_user:
        - require:
            - test: clickhouse-sql-users-req
        - require_in:
            - test: clickhouse-sql-users-ready
{% endif %}

{% if user_management_v2 and not sql_user_management %}

cleanup_clickhouse_users:
    mdb_clickhouse.cleanup_users:
        - require:
            - test: clickhouse-sql-users-req
        - require_in:
            - test: clickhouse-sql-users-ready

cleanup_clickhouse_user_quotas:
    mdb_clickhouse.cleanup_user_quotas:
        - require:
            - test: clickhouse-sql-users-req
        - require_in:
            - test: clickhouse-sql-users-ready

cleanup_clickhouse_user_settings_profiles:
    mdb_clickhouse.cleanup_user_settings_profiles:
        - require:
            - test: clickhouse-sql-users-req
        - require_in:
            - test: clickhouse-sql-users-ready

{%     for user in salt.pillar.get('data:clickhouse:users', {}).keys() %}
clickhouse_user_{{ user }}:
    mdb_clickhouse.ensure_sql_user:
        - name: {{ user }}
        - require:
            - test: clickhouse-sql-users-req
        - require_in:
            - test: clickhouse-sql-users-ready

{%     endfor %}
{% endif %}
