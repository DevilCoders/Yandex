{% from "components/postgres/pg.jinja" import pg with context %}
scripts-sqls-dir:
    file.directory:
        - name: /usr/local/yandex/sqls
        - makedirs: True
        - user: postgres
        - group: postgres
        - mode: 700

{% if salt['pillar.get']('data:pgbouncer:count', 1) > 1 %}
/usr/local/yandex/sqls/pgbouncer.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/scripts-sql/multiple_pgbouncer.sql
        - user: postgres
        - group: postgres
        - mode: 700
        - require:
            - service: postgresql-service
            - file: scripts-sqls-dir
{% else %}
/usr/local/yandex/sqls/pgbouncer.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/scripts-sql/pgbouncer.sql
        - user: postgres
        - group: postgres
        - mode: 700
        - require:
            - service: postgresql-service
            - file: scripts-sqls-dir
{% endif %}

/usr/local/yandex/sqls/repl_mon.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/scripts-sql/repl_mon.sql
        - user: postgres
        - group: postgres
        - mode: 700
        - require:
            - service: postgresql-service
            - file: scripts-sqls-dir

/usr/local/yandex/sqls/pg_sli_test.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/scripts-sql/pg_sli_test.sql
        - user: postgres
        - group: postgres
        - mode: 700
        - require:
            - service: postgresql-service
            - file: scripts-sqls-dir


/usr/local/yandex/sqls/show_activity_stats.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/scripts-sql/show_activity_stats.sql
        - user: postgres
        - group: postgres
        - mode: 700
        - require:
            - service: postgresql-service
            - file: scripts-sqls-dir

/usr/local/yandex/sqls/table_bloat.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/scripts-sql/table_bloat.sql
        - user: postgres
        - group: postgres
        - mode: 700
        - require:
            - service: postgresql-service
            - file: scripts-sqls-dir

/usr/local/yandex/sqls/index_bloat.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/scripts-sql/index_bloat.sql
        - user: postgres
        - group: postgres
        - mode: 700
        - require:
            - service: postgresql-service
            - file: scripts-sqls-dir

/usr/local/yandex/sqls/unapplied_settings.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/scripts-sql/unapplied_settings.sql
        - user: postgres
        - group: postgres
        - mode: 700
        - require:
            - service: postgresql-service
            - file: scripts-sqls-dir

{% if pg.is_master %}

check_admin_relations:
    mdb_postgresql.test_relation_exists:
        - relnames:
            - pg_sli_test
        - require:
            - cmd: postgresql-service

create_pgbouncer_views:
    postgresql_cmd.psql_file:
        - name: /usr/local/yandex/sqls/pgbouncer.sql
        - onchanges:
            - file: /usr/local/yandex/sqls/pgbouncer.sql
        - require:
            - cmd: postgresql-service

create_show_activity_stats_function:
    postgresql_cmd.psql_file:
        - name: /usr/local/yandex/sqls/show_activity_stats.sql
        - onchanges:
            - file: /usr/local/yandex/sqls/show_activity_stats.sql
        - require:
            - cmd: postgresql-service

create_pq_sli_test_table:
    postgresql_cmd.psql_file:
        - name: /usr/local/yandex/sqls/pg_sli_test.sql
        - require:
            - cmd: postgresql-service
        - onchanges:
            - file: /usr/local/yandex/sqls/pg_sli_test.sql
            - mdb_postgresql: check_admin_relations

create_get_unapplied_settings_function:
    postgresql_cmd.psql_file:
        - name: /usr/local/yandex/sqls/unapplied_settings.sql
        - onchanges:
            - file: /usr/local/yandex/sqls/unapplied_settings.sql
        - require:
            - cmd: postgresql-service
{% endif %}
