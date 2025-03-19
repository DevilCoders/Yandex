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
        importer:
            import_config:
                cluster_type_rules:
                    postgresql_cluster:
                        import_interval: '24h'
                        batch_size: 100
                    mongodb_cluster:
                        import_interval: '24h'
                        batch_size: 100

    s3:
        host: https://s3.mds.yandex.net
        access_key: {{ salt.yav.get('ver-01evvrj6ejwcn85tg28c7gjqtw[id]') }}
        secret_key: {{ salt.yav.get('ver-01evvrj6ejwcn85tg28c7gjqtw[secret]') }}

    zk:
        hosts:
            - zkeeper01e.db.yandex.net:2181
            - zkeeper01f.db.yandex.net:2181
            - zkeeper01h.db.yandex.net:2181

    solomon:
        cluster: mdb_backup_porto_prod

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - mdb_controlplane_porto_prod.common.deploy
    - mdb_controlplane_porto_prod.common.health
    - mdb_controlplane_porto_prod.common.metadb
    - mdb_controlplane_porto_prod.common.solomon
    - porto.prod.pgusers.backup_worker
    - porto.prod.pgusers.backup_scheduler
    - porto.prod.pgusers.backup_cli
