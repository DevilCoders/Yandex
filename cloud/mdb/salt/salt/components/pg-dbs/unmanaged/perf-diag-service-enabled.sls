{% from "components/postgres/pg.jinja" import pg with context %}
include:
    - components.pushclient2.service
    - components.pushclient2.files_nop
    - components.dbaas-cron.service
    - .perf-diag-enabled

{% if pg.version.major_num < 1400 %}
pg_sync_databases:
    mdb_postgresql.extension_present:
       - name: 'mdb_perf_diag'
{% endif %}
