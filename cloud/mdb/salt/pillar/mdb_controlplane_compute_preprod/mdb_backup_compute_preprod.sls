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
            id: {{ salt.yav.get('ver-01f9hpwgmc9mn40ddxec8y5pb3[service_account_id]') }}
            key_id: {{ salt.yav.get('ver-01f9hpwgmc9mn40ddxec8y5pb3[key_id]') }}
            private_key: {{ salt.yav.get('ver-01f9hpwgmc9mn40ddxec8y5pb3[private_key]') | yaml_encode }}
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
        host: https://s3-private.mds.yandex.net
        access_key: {{ salt.yav.get('ver-01evvrqdtk8678fcedwehav4fp[key]') }}
        secret_key: {{ salt.yav.get('ver-01evvrqdtk8678fcedwehav4fp[secret]') }}

    zk:
        hosts:
            - zk-dbaas-preprod01f.cloud-preprod.yandex.net:2181
            - zk-dbaas-preprod01h.cloud-preprod.yandex.net:2181
            - zk-dbaas-preprod01k.cloud-preprod.yandex.net:2181

    solomon:
        cluster: mdb_backup_compute_preprod
        use_name_tag: True

include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.deploy
    - mdb_controlplane_compute_preprod.common.health
    - mdb_controlplane_compute_preprod.common.metadb
    - mdb_controlplane_compute_preprod.common.solomon
    - compute.preprod.pgusers.backup_worker
    - compute.preprod.pgusers.backup_scheduler
    - compute.preprod.pgusers.backup_cli
