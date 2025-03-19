mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    condgroup: mail_pgtest
    monrun2: True
    runlist:
        - components.postgres
        - components.pg-dbs.pgmeta
    monrun:
        pg_replication_alive:
            warn: '-1'
            crit: '-1'
        load_average_relative:
            warn: 2
            crit: 4
    pgsync:
        zk_hosts: zk-test01i.mail.yandex.net:2181,zk-test01f.mail.yandex.net:2181,zk-test01h.mail.yandex.net:2181
    config:
        shared_buffers: 128MB
        pool_mode: transaction
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        pg_hba:
            - 'host all pgproxy 0.0.0.0/0 md5'
            - 'host all pgproxy ::/0 md5'
        databases:
            - rpopdb
            - s3db
    db_code_map:
        s3db: s3db.sql
    sysctl:
        vm.nr_hugepages: 0

include:
    - index_repack
    - envs.load
    - porto.prod.pgusers.load.common
    - porto.prod.pgusers.load.pgproxy_common
    - porto.prod.pgusers.load.pgproxy
    - porto.prod.s3.pgaas_s3backup
