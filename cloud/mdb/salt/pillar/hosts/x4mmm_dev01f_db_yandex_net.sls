data:
    use_nat64_dns: True
    monrun2: True
    config:
        shared_buffers: 512MB
        lock_timeout: 30s
        wal_log_hints: 'on'
        shared_preload_libraries: pg_stat_statements,repl_mon
        pool_mode: transaction
        server_reset_query_always: 1
    sysctl:
        vm.nr_hugepages: 0
    use_pg_partman: True
    use_postgis: True
    s3_bucket: 'dbaas'

include:
    - index_repack
    - envs.dev
    - porto.prod.s3.pgaas_s3backup
    - porto.prod.pgusers.dev.common
