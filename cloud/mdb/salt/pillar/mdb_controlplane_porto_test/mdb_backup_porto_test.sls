mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.monrun2
        - components.yasmagent
        - components.logrotate
        - components.dbaas-porto-controlplane
        - components.backup

    monrun2: True

    mdb-backup:
        service_account:
            id: {{ salt.yav.get('ver-01es6s3j8b00tgpm8jsyrypvhn[service_account_id]') }}
            key_id: {{ salt.yav.get('ver-01es6s3j8b00tgpm8jsyrypvhn[key_id]') }}
            private_key: {{ salt.yav.get('ver-01es6s3j8b00tgpm8jsyrypvhn[private_key]') | yaml_encode }}
        scheduler:
            schedule_config:
                cluster_type_rules:
                    mysql_cluster:
                        schedule_mode: 'PLAIN'
                        exclude_subclusters: []
                    mongodb_cluster:
                        schedule_mode: 'PLAIN'
                        exclude_subclusters: ['mongos_subcluster']
                    postgresql_cluster:
                        exclude_subclusters: []
                        schedule_mode: 'INCREMENTAL'
                    clickhouse_cluster:
                        exclude_subclusters: []
                        schedule_mode: 'PLAIN'
        importer:
            import_config:
                cluster_type_rules:
                    postgresql_cluster:
                        import_interval: '24h'
                        batch_size: 100

    s3:
        host: https://s3.mds.yandex.net
        access_key: {{ salt.yav.get('ver-01evvrj6ejwcn85tg28c7gjqtw[id]') }}
        secret_key: {{ salt.yav.get('ver-01evvrj6ejwcn85tg28c7gjqtw[secret]') }}

    zk:
        hosts:
            - zk-df-e2e01f.db.yandex.net:2181
            - zk-df-e2e01h.db.yandex.net:2181
            - zk-df-e2e01k.db.yandex.net:2181

    solomon:
        cluster: mdb_backup_porto_test

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.deploy
    - mdb_controlplane_porto_test.common.health
    - mdb_controlplane_porto_test.common.metadb
    - mdb_controlplane_porto_test.common.selfdns
    - mdb_controlplane_porto_test.common.solomon
    - porto.test.pgusers.backup_worker
    - porto.test.pgusers.backup_scheduler
    - porto.test.pgusers.backup_cli
