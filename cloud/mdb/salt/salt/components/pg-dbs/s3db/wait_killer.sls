{% from "components/postgres/pg.jinja" import pg with context %}

{% if salt['pillar.get']('data:dbaas-cron:pg_wait_killer:enabled', False) %}
/usr/local/yandex/sqls/mdb_terminate_backend.sql:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb_terminate_backend.sql
        - user: postgres
        - group: postgres
        - mode: 700
        - require:
            - service: postgresql-service
            - file: scripts-sqls-dir

/etc/dbaas-cron/conf.d/pg_wait_killer.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/pg_wait_killer.conf
        - require:
            - pkg: dbaas-cron
            - file: /home/monitor/.postgresql/root.crt
        - watch_in:
            - service: dbaas-cron

/etc/dbaas-cron/modules/pg_wait_killer.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/pg_wait_killer.py
        - require:
            - pkg: dbaas-cron
        - watch_in:
            - service: dbaas-cron

{% if pg.is_master %}
create_mdb_terminate_backend:
    postgresql_cmd.psql_file:
        - name: /usr/local/yandex/sqls/mdb_terminate_backend.sql
        - onchanges:
            - file: /usr/local/yandex/sqls/mdb_terminate_backend.sql
        - require:
            - cmd: postgresql-service
{% endif %}

{% else %}
/etc/dbaas-cron/modules/pg_wait_killer.py:
    file.absent
/etc/dbaas-cron/conf.d/pg_wait_killer.conf:
    file.absent
{% endif %}
