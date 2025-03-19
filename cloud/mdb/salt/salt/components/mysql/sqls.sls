{% from "components/mysql/map.jinja" import mysql with context %}
scripts-sqls-dir:
    file.directory:
        - name: /usr/local/yandex/sqls
        - makedirs: True
        - user: mysql
        - group: mysql
        - mode: 755

/usr/local/yandex/sqls/mdb_repl_mon.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/scripts-sql/mdb_repl_mon.sql
        - user: mysql
        - group: mysql
        - mode: 644
        - require:
          - file: scripts-sqls-dir

/usr/local/yandex/sqls/mdb_sessions.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/scripts-sql/mdb_sessions.sql
        - user: mysql
        - group: mysql
        - mode: 644
        - require:
          - file: scripts-sqls-dir

/usr/local/yandex/sqls/mdb_statements.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/scripts-sql/mdb_statements.sql
        - user: mysql
        - group: mysql
        - mode: 644
        - require:
          - file: scripts-sqls-dir

{% if not mysql.is_replica %}
check-migration-schema:
  mdb_mysql_schema.check_schema:
    - require:
      - test: mysql-ready

create-mysql-repl_mon_event:
  mdb_mysql_schema.apply_migration:
    - migration: /usr/local/yandex/sqls/mdb_repl_mon.sql
    - require:
      - mdb_mysql_schema: check-migration-schema
      - file: /usr/local/yandex/sqls/mdb_repl_mon.sql

create-view-mdb_sessions:
  mdb_mysql_schema.apply_migration:
    - migration: /usr/local/yandex/sqls/mdb_sessions.sql
    - require:
      - mdb_mysql_schema: check-migration-schema
      - file: /usr/local/yandex/sqls/mdb_sessions.sql

create-view-mdb_statements:
  mdb_mysql_schema.apply_migration:
    - migration: /usr/local/yandex/sqls/mdb_statements.sql
    - require:
      - mdb_mysql_schema: check-migration-schema
      - file: /usr/local/yandex/sqls/mdb_statements.sql

{% endif %}
