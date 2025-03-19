data:
    mdb-internal-api:
        service_account:
            id: {{ salt.yav.get('ver-01ewdq9ee90wtddzhfv0e8pvq4[id]') }}
            key_id: {{ salt.yav.get('ver-01ewdq9ee90wtddzhfv0e8pvq4[key_id]') }}
            private_key: {{ salt.yav.get('ver-01ewdq9ee90wtddzhfv0e8pvq4[private_key]') | yaml_encode }}
        config:
            app:
                logging:
                    level: debug
            api:
                expose_error_debug: false
                cloud_id_prefix: c9q
            metadb:
                addrs:
                    - meta-dbaas01f.yandexcloud.net:6432
                    - meta-dbaas01h.yandexcloud.net:6432
                    - meta-dbaas01k.yandexcloud.net:6432
                db: dbaas_metadb
                user: dbaas_api
                password: {{ salt.yav.get('ver-01e87pdajgwnnenhj9qy64e618[password]') }}
                sslrootcert: /opt/yandex/allCAs.pem
                max_open_conn: 64
                max_idle_conn: 64
            logsdb:
                addrs:
                    # https://console.cloud.yandex.ru/folders/b1g0r9fh49hee3rsc0aa/managed-clickhouse/cluster/c9q2oes4qc45bh9s965s?section=hosts
                    - rc1a-7nuxwviowmqfq1e6.mdb.yandexcloud.net:9440
                    - rc1c-1sq33a12mzvdpon9.mdb.yandexcloud.net:9440
                    - rc1a-vnz6wzifsx61r46p.mdb.yandexcloud.net:9440
                    - rc1c-ikkzgm07dd9kqy2k.mdb.yandexcloud.net:9440
                db: mdb
                user: logs_reader
                password: {{ salt.yav.get('ver-01enjmxbgvenpgp3css7dezwmc[reader_password]') }}
                ca_file: /opt/yandex/allCAs.pem
                debug: True
                time_column: timestamp
            perfdiagdb:
                addrs:
                    - man-wsbx7u3dg0uyssqr.db.yandex.net:9440
                    - sas-j5673gg9rqifv7cf.db.yandex.net:9440
                    - vla-yu0twskroeafy1a5.db.yandex.net:9440
                db: perf_diag
                user: dbaas_api_reader
                password: {{ salt.yav.get('ver-01eesv5ptrgrywkbh6hdhx77kh[password]') }}
                ca_file: /opt/yandex/allCAs.pem
                debug: True
            perfdiagdb_mongodb:
                disabled: False
                addrs:
                    # https://yc.yandex-team.ru/folders/fooi5vu9rdejqc3p4b60/managed-clickhouse/cluster/mdbsho6mpkr786ktf9e8?section=hosts
                    - rc1a-5ow37jfcwf5knl5n.mdb.yandexcloud.net:9440
                    - rc1b-1liw6v4z4mjsjnas.mdb.yandexcloud.net:9440
                db: perfdiag
                user: dbaas_api_reader
                password: {{ salt.yav.get('ver-01fg3fk2bpy78p0prx4p9f84dn[password]') }}
                ca_file: /opt/yandex/allCAs.pem
                debug: True
            s3:
                host: s3-private.mds.yandex.net
                access_key: {{ salt.yav.get('ver-01e87syb5se464d869gaj6aj5x[id]') }}
                secret_key: {{ salt.yav.get('ver-01e87syb5se464d869gaj6aj5x[secret]') }}
            s3_secure_backups:
                host: storage.yandexcloud.net
                access_key: {{ salt.yav.get('ver-01fpysnnkdhjz70gcex0xtwsq9[id]') }}
                secret_key: {{ salt.yav.get('ver-01fpysnnkdhjz70gcex0xtwsq9[key]') }}
            access_service:
                addr: as.private-api.cloud.yandex.net:4286
                capath: /opt/yandex/allCAs.pem
            token_service:
                addr: ts.private-api.cloud.yandex.net:4282
                capath: /opt/yandex/allCAs.pem
            license_service:
                addr: https://billing.private-api.cloud.yandex.net:16465
                capath: /opt/yandex/allCAs.pem
            resource_manager:
                addr: rm.private-api.cloud.yandex.net:4284
                capath: /opt/yandex/allCAs.pem
            iam:
                uri: https://identity.private-api.cloud.yandex.net:14336
                http:
                    transport:
                        tls:
                            ca_file: /opt/yandex/allCAs.pem
            health:
                host: mdb-health.private-api.cloud.yandex.net
                tls:
                    ca_file: /opt/yandex/allCAs.pem
            crypto:
                private_key: {{ salt.yav.get('ver-01e87rxd7dcvtnrq6rc5zzc1j9[secret]') }}
                public_key: {{ salt.yav.get('ver-01e87rxd7dcvtnrq6rc5zzc1j9[public]') }}
            sentry:
                dsn: {{ salt.yav.get('ver-01e326z4jsr9q8126qsk54awjj[dsn]') }}
                environment: compute-prod
            vpc:
                uri: network-api.private-api.cloud.yandex.net:9823
            compute:
                uri: compute-api.cloud.yandex.net:9051
            logic:
                flags:
                    allow_move_between_clouds: False
                e2e:
                    cluster_name: dbaas_e2e_compute_prod
                    folder_id: b1gdepbkva865gm1nbkq
                vtypes:
                    compute: mdb.yandexcloud.net
                environment_vtype: compute
                saltenvs:
                    production: compute-prod
                    prestable: prod
                generation_names:
                    1: Intel Broadwell
                    2: Intel Cascade Lake
                    3: Intel Ice Lake
                kafka:
                    tasks_prefix: dqh
                    zk_zones:
                        - ru-central1-a
                        - ru-central1-b
                        - ru-central1-c
                    sync_topics: True
                metastore:
                    kubernetes_cluster_id: tbd
                    postgresql_cluster_id: tbd
                    kubernetes_cluster_service_account_id: tbd
                    kubernetes_node_service_account_id: tbd
                    postgresql_hostname: tbd
                    service_subnet_ids:
                      - tbd
                elasticsearch:
                    tasks_prefix: ba4
                    enable_auto_backups: true
                    allowed_editions:
                        - basic
                        - platinum
                sqlserver:
                    tasks_prefix: ajm
                    product_ids:
                        standard: f2eversiomdbstdmssql
                        enterprise: f2eversiomdbentmssql
                greenplum:
                    tasks_prefix: mgp
                clickhouse:
                    external_uri_validation:
                        use_http_client: true
                        regexp: https://(?:[a-zA-Z0-9-]+\.)?storage\.yandexcloud\.net/\S+
                        message: URI should be a reference to Yandex Object Storage
                resource_validation:
                    decommissioned_resource_presets:
                        - s1.nano
                        - m1.micro
                        - m1.small
                        - m1.medium
                        - m1.large
                        - m1.xlarge
                        - m1.2xlarge
                        - m1.3xlarge
                        - m1.4xlarge
                    minimal_disk_unit: 4194304
                service_zk:
                    hosts:
                        compute-prod:
                            - zk-dbaas01f.db.yandex.net
                            - zk-dbaas01h.db.yandex.net
                            - zk-dbaas01k.db.yandex.net
                        prod:
                            - zk-dbaas01f.db.yandex.net
                            - zk-dbaas01h.db.yandex.net
                            - zk-dbaas01k.db.yandex.net
                console:
                    uri: https://console.cloud.yandex.ru
                monitoring:
                    charts:
                        postgresql:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-postgresql/cluster/{cid}/monitoring'
                        clickhouse:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-clickhouse/cluster/{cid}/monitoring'
                        mongodb:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-mongodb/cluster/{cid}/monitoring'
                        redis:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-redis/cluster/{cid}/monitoring'
                        mysql:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-mysql/cluster/{cid}/monitoring'
                        sqlserver:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-sqlserver/cluster/{cid}/monitoring'
                        greenplum:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-greenplum/cluster/{cid}/monitoring'
                        kafka:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-kafka/cluster/{cid}/monitoring'
                        elasticsearch:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-elasticsearch/cluster/{cid}/monitoring'
                cloud_default_quota:
                    clusters: 16
                    cpu: 64
                    memory: 549755813888
                    io: 549755813888
                    network: 549755813888
                    ssd_space: 4398046511104
                    hdd_space: 4398046511104
                    gpu: 0
        console_default_resources:
            postgresql_cluster:
                postgresql_cluster:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
            clickhouse_cluster:
                clickhouse_cluster:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
                zk:
                    generation: 2
                    resource_preset_id: b2.medium
                    disk_type_id: network-hdd
                    disk_size: 10737418240
            mongodb_cluster:
                mongodb_cluster:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongod:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongos:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongocfg:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongoinfra:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
            mysql_cluster:
                mysql_cluster:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
            sqlserver_cluster:
                sqlserver_cluster:
                    generation: 2
                    resource_preset_id: s2.small
                    disk_type_id: network-ssd
                    disk_size: 10737418240
            redis_cluster:
                redis_cluster:
                    generation: 2
                    resource_preset_id: hm2.nano
                    disk_type_id: network-ssd
                    disk_size: 17179869184
            hadoop_cluster:
                hadoop_cluster.masternode:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 137438953472
                hadoop_cluster.datanode:
                    generation: 2
                    resource_preset_id: s2.small
                    disk_type_id: network-hdd
                    disk_size: 137438953472
                hadoop_cluster.computenode:
                    generation: 2
                    resource_preset_id: s2.small
                    disk_type_id: network-ssd
                    disk_size: 68719476736
            kafka_cluster:
                kafka_cluster:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 34359738368
                zk:
                    generation: 2
                    resource_preset_id: b2.medium
                    disk_type_id: network-ssd
                    disk_size: 10737418240
            elasticsearch_cluster:
                 elasticsearch_cluster.datanode:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 17179869184
                 elasticsearch_cluster.masternode:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
            greenplum_cluster:
                 greenplum_cluster.master_subcluster:
                    generation: 2
                    resource_preset_id: s2.medium
                    disk_type_id: local-ssd
                    disk_size: 34359738368
                 greenplum_cluster.segment_subcluster:
                    generation: 2
                    resource_preset_id: s2.medium
                    disk_type_id: local-ssd
                    disk_size: 137438953472
    envoy:
        use_health_map: true
        clusters:
            mdb-internal-api:
                prefix: "/"
                port: 50050
    logship:
        use_rt: false
        topics:
            mdb_internal_api: /b1ggh9onj7ljr7m9cici/controlplane/mdb-internal-api-logs
        use_cloud_logbroker: True
        logbroker_host: lb.etn03iai600jur7pipla.ydb.mdb.yandexcloud.net
        logbroker_port: 2135
        database: /global/b1gvcqr959dbmi1jltep/etn03iai600jur7pipla
        iam_endpoint: iam.api.cloud.yandex.net
        lb_producer_key: {{ salt.yav.get('ver-01eqaxn8w89qc84nans1z1z19z[lb_producer_key]') | tojson }}
