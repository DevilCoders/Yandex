{% set vtype = salt['grains.get']('test:vtype', 'porto') %}
data:
    runlist:
{% if vtype == 'compute' %}
        - components.firewall
        - components.dbaas-compute.apparmor
        - components.dbaas-compute.apparmor.postgresql
        - components.dbaas-compute.scripts
        - components.dbaas-compute.network
        - components.cloud-init
        - components.linux-kernel
{% endif %}
        - components.dbaas-billing
        - components.postgres
        - components.pg-dbs.unmanaged
    config:
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        shared_buffers: 128MB
        archive_mode: 'off'
    monrun:
        pg_replication_alive:
            warn: '-1'
            crit: '-1'
    start_pgsync: False
    walg_periodic_backups: False
    use_postgis: True
    do_index_repack: True
{% if vtype != 'compute' %}
    use_yasmagent: True
{% endif %}

include:
    - porto.prod.pgusers.dev.common
