{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    condgroup: mail_pgproxy
    connection_pooler: pgbouncer
    use_pgsync: False
    use_walg: False
    runlist:
        - components.postgres
        - components.pg-dbs.pgproxy
        - components.network
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400::107
    lxc_used: True
    diagnostic_tools: False
    pgmeta:
        server: pgmeta.mail.yandex.net
        read_port: 5432
    monrun2: True
    monrun:
        tls:
            warn: 30
        pg_replication_alive:
            warn: '-1'
            crit: '-1'
        pg_log_errors:
            warn: '20\ 200'
        bouncer_log_errors:
            crit: '2000\ 2000'
    config:
        max_connections: 4096
        work_mem: 16MB
        maintenance_work_mem: 16MB
        bgwriter_delay: 200ms
        bgwriter_lru_maxpages: 100
        checkpoint_segments: 64
        checkpoint_timeout: 5min
        checkpoint_completion_target: 0.5
        checkpoint_warning: 30s
        log_hostname: 'off'
        log_lock_waits: 'off'
        shared_buffers: 4GB
        shared_preload_libraries: pg_stat_statements,repl_mon
        archive_mode: 'off'
        dynamic_shared_memory_type: none
        pool_mode: statement
        application_name_add_host: 0
        databases:
            - rpopdb
            - rpopdb_transfer
        db_users_map:
            rpopdb_transfer:
                user: rpop_transfer
                code:
                    - pgproxy/rpopdb.sql
    sysctl:
        vm.nr_hugepages: 0
        net.ipv6.icmp.ratelimit: 0
    yasmagent:
        prj_split_by_shard: False
    pg_ssl_balancer: pgproxy.mail.yandex.net
    pgbouncer:
        count: 16
        internal_count: 4
{% set edition = 'default' %}
{% set pg_major_version = 10 %}
{% set env = 'prod' %}
    versions:
        postgres:
            major_version: {{ pg_major_version }}
            minor_version: {{ pg_versions_package_map[(env, pg_major_version * 100, edition)]['minor_version'] | tojson }}
            package_version: {{ pg_versions_package_map[(env, pg_major_version * 100, edition)]['package_version'] }}
            edition: {{ edition }}
        odyssey:
            major_version: {{ pg_major_version }}
            minor_version: {{ odyssey_versions_package_map[(env, pg_major_version * 100, edition)]['minor_version'] | tojson }}
            package_version: {{ odyssey_versions_package_map[(env, pg_major_version * 100, edition)]['package_version'] }}
            edition: {{ edition }}

include:
    - index_repack
    - envs.{{ env }}
    - porto.prod.selfdns.realm-mail
    - porto.prod.pgusers.common
    - porto.prod.pgusers.pgproxy_common
    - porto.prod.pgusers.pgproxy
    - porto.prod.pgusers.rpop
    - porto.prod.pgusers.rpop_transfer
