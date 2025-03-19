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
        - components.dbaas-compute-controlplane
        - components.backup

    monrun2: True
    dbaas:
        vtype: compute
    cauth_use: False
    ipv6selfdns: True

    mdb-backup:
        service_account:
            id: {{ salt.yav.get('ver-01f9k967qkch7qjj52m9q30cte[service_account_id]') }}
            key_id: {{ salt.yav.get('ver-01f9k967qkch7qjj52m9q30cte[key_id]') }}
            private_key: {{ salt.yav.get('ver-01f9k967qkch7qjj52m9q30cte[private_key]') | yaml_encode }}
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

    s3:
        host: https://s3-private.mds.yandex.net
        access_key: {{ salt.yav.get('ver-01evvrqdtk8678fcedwehav4fp[key]') }}
        secret_key: {{ salt.yav.get('ver-01evvrqdtk8678fcedwehav4fp[secret]') }}

    zk:
        hosts:
            - zk-dbaas01f.yandexcloud.net:2181
            - zk-dbaas01h.yandexcloud.net:2181
            - zk-dbaas01k.yandexcloud.net:2181

    solomon:
        cluster: mdb_backup_compute_prod
        use_name_tag: True
        iam_token: 'temporary-placeholder-till-we-fix-an-internal-app'

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.deploy
    - mdb_controlplane_compute_prod.common.health
    - mdb_controlplane_compute_prod.common.metadb
    - mdb_controlplane_compute_prod.common.solomon
    - compute.prod.pgusers.backup_worker
    - compute.prod.pgusers.backup_scheduler
    - compute.prod.pgusers.backup_cli
