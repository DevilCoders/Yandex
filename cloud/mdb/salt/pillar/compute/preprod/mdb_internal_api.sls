data:
    mdb-internal-api:
        service_account:
            id: {{ salt.yav.get('ver-01ew0anmtq0y181dn3yesqky30[id]') }}
            key_id: {{ salt.yav.get('ver-01ew0anmtq0y181dn3yesqky30[key_id]') }}
            private_key: {{ salt.yav.get('ver-01ew0anmtq0y181dn3yesqky30[private_key]') | yaml_encode }}
        config:
            app:
                logging:
                    level: debug
            api:
                expose_error_debug: true
            metadb:
                addrs:
                    - meta-dbaas-preprod01f.cloud-preprod.yandex.net:6432
                    - meta-dbaas-preprod01h.cloud-preprod.yandex.net:6432
                    - meta-dbaas-preprod01k.cloud-preprod.yandex.net:6432
                db: dbaas_metadb
                user: dbaas_api
                password: {{ salt.yav.get('ver-01dwew18xwjg8mq1jy8jrea2qg[password]') }}
                sslrootcert: /opt/yandex/allCAs.pem
                max_open_conn: 64
                max_idle_conn: 64
            logsdb:
                addrs:
                    # https://console-preprod.cloud.yandex.ru/folders/aoeme1ci0qvbsjia4ks7/managed-clickhouse/cluster/e4uh169bluasvlmd6eds
                    - rc1b-iqxeckgl38xak5o3.mdb.cloud-preprod.yandex.net:9440
                    - rc1c-emcg2tjnvusyqxjc.mdb.cloud-preprod.yandex.net:9440
                db: mdb
                user: logs_reader
                password: {{ salt.yav.get('ver-01ep744g1d71vn5ysey5xey7ms[reader_password]') }}
                ca_file: /opt/yandex/allCAs.pem
                debug: True
                time_column: timestamp
            perfdiagdb:
                addrs:
                    - man-zhny3myz9wvdb2ex.db.yandex.net:9440
                    - sas-4qhqb5jrc5di39wu.db.yandex.net:9440
                    - vla-vvotlt46pvjr49ji.db.yandex.net:9440
                db: perf_diag
                user: dbaas_api_reader
                password: {{ salt.yav.get('ver-01eestzy76b75tn7km3w1zpcg7[password]') }}
                ca_file: /opt/yandex/allCAs.pem
                debug: True
            perfdiagdb_mongodb:
                disabled: False
                addrs:
                    # https://console-preprod.cloud.yandex.ru/folders/aoeme1ci0qvbsjia4ks7/managed-clickhouse/cluster/e4ufvdnli9to9o0qpek0?section=hosts
                    - rc1b-m5qzzw1kn10q2mkh.mdb.cloud-preprod.yandex.net:9440
                    - rc1c-8lwk2mr2gli8frzs.mdb.cloud-preprod.yandex.net:9440
                db: perfdiag
                user: dbaas_api_reader
                password: {{ salt.yav.get('ver-01fg3gaj6bc7ps41aw8wtthhdx[password]') }}
                ca_file: /opt/yandex/allCAs.pem
                debug: True
            s3:
                host: s3-private.mds.yandex.net
                access_key: {{ salt.yav.get('ver-01dw7pdevntz66n667yrcexg9m[id]') }}
                secret_key: {{ salt.yav.get('ver-01dw7pdevntz66n667yrcexg9m[key]') }}
            s3_secure_backups:
                host: storage.cloud-preprod.yandex.net
                access_key: {{ salt.yav.get('ver-01fpthttms47re0tb3gwwvbbpw[id]') }}
                secret_key: {{ salt.yav.get('ver-01fpthttms47re0tb3gwwvbbpw[key]') }}
            access_service:
                addr: as.private-api.cloud-preprod.yandex.net:4286
                capath: /opt/yandex/allCAs.pem
            token_service:
                addr: ts.private-api.cloud-preprod.yandex.net:4282
                capath: /opt/yandex/allCAs.pem
            license_service:
                addr: https://billing.private-api.cloud-preprod.yandex.net:16465
                capath: /opt/yandex/allCAs.pem
            resource_manager:
                addr: rm.private-api.cloud-preprod.yandex.net:4284
                capath: /opt/yandex/allCAs.pem
            iam:
                uri: https://identity.private-api.cloud-preprod.yandex.net:14336
                http:
                    transport:
                        tls:
                            ca_file: /opt/yandex/allCAs.pem
            health:
                host: mdb-health.private-api.cloud-preprod.yandex.net
                tls:
                    ca_file: /opt/yandex/allCAs.pem
            crypto:
                private_key: {{ salt.yav.get('ver-01e2nmk3y1528t1r2fd2swjf1b[private-key]') }}
                public_key: {{ salt.yav.get('ver-01e2nmk3y1528t1r2fd2swjf1b[public-key]') }}
            sentry:
                dsn: {{ salt.yav.get('ver-01e326z4jsr9q8126qsk54awjj[dsn]') }}
                environment: compute-preprod
            vpc:
                uri: network-api-internal.private-api.cloud-preprod.yandex.net:9823
            compute:
                uri: compute-api.cloud-preprod.yandex.net:9051
            logic:
                flags:
                    allow_move_between_clouds: False
                e2e:
                    cluster_name: dbaas_e2e_compute_preprod
                    folder_id: aoed5i52uquf5jio0oec
                vtypes:
                    compute: mdb.cloud-preprod.yandex.net
                environment_vtype: compute
                saltenvs:
                    production: qa
                    prestable: dev
                generation_names:
                    1: Intel Broadwell
                    2: Intel Cascade Lake
                    3: Intel Ice Lake
                airflow:
                    kubernetes_cluster_id: c49uc4kv7j1igv7vce45
                kafka:
                    zk_zones:
                        - ru-central1-a
                        - ru-central1-b
                        - ru-central1-c
                    sync_topics: True
                metastore:
                    kubernetes_cluster_id: c49g3qmsid7j9g93uc4b
                    postgresql_cluster_id: e4umt9qcfhcc7a9kprco
                    kubernetes_cluster_service_account_id: yc.metastore.kubermaster
                    kubernetes_node_service_account_id: yc.metastore.kubernode
                    postgresql_hostname: c-e4umt9qcfhcc7a9kprco.rw.mdb.cloud-preprod.yandex.net
                    service_subnet_ids:
                      - bucd0genetip31dsb8bg
                clickhouse:
                    use_backup_service: true
                    external_uri_validation:
                        use_http_client: true
                        regexp: https://(?:[a-zA-Z0-9-]+\.)?storage\.cloud-preprod\.yandex\.net/\S+
                        message: URI should be a reference to Yandex Object Storage
                elasticsearch:
                    enable_auto_backups: true
                    allowed_editions:
                        - basic
                        - platinum
                greenplum:
                    tasks_prefix: zgp
                sqlserver:
                    product_ids:
                        standard: dqnversiomdbstdmssql
                        enterprise: dqnversiomdbentmssql
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
                        qa:
                            - zk-dbaas-preprod01f.db.yandex.net
                            - zk-dbaas-preprod01h.db.yandex.net
                            - zk-dbaas-preprod01k.db.yandex.net
                        dev:
                            - zk-dbaas-preprod01f.db.yandex.net
                            - zk-dbaas-preprod01h.db.yandex.net
                            - zk-dbaas-preprod01k.db.yandex.net
                console:
                    uri: https://console-preprod.cloud.yandex.ru
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
                    clusters: 8
                    cpu: 8
                    memory: 34359738368
                    io: 167772160
                    network: 134217728
                    ssd_space: 214748364800
                    hdd_space: 214748364800
                    gpu: 0
        console_default_resources:
            postgresql_cluster:
                postgresql_cluster:
                    generation: 2
                    resource_preset_id: b2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
            clickhouse_cluster:
                clickhouse_cluster:
                    generation: 2
                    resource_preset_id: b2.micro
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
                    resource_preset_id: b2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongod:
                    generation: 2
                    resource_preset_id: b2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongos:
                    generation: 2
                    resource_preset_id: b2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongocfg:
                    generation: 2
                    resource_preset_id: b2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongoinfra:
                    generation: 2
                    resource_preset_id: b2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
            mysql_cluster:
                mysql_cluster:
                    generation: 2
                    resource_preset_id: b2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
            sqlserver_cluster:
                sqlserver_cluster:
                    generation: 2
                    resource_preset_id: s2.micro
                    disk_type_id: network-ssd
                    disk_size: 10737418240
                windows_witness:
                    generation: 2
                    resource_preset_id: s2.small
                    disk_type_id: network-ssd
                    disk_size: 10737418240
            redis_cluster:
                redis_cluster:
                    generation: 2
                    resource_preset_id: b2.medium
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
                    disk_size: 137438953472
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
                    resource_preset_id: b2.medium
                    disk_type_id: network-ssd
                    disk_size: 17179869184
                 elasticsearch_cluster.masternode:
                    generation: 2
                    resource_preset_id: b2.medium
                    disk_type_id: network-ssd
                    disk_size: 10737418240
            greenplum_cluster:
                 greenplum_cluster.master_subcluster:
                    generation: 2
                    resource_preset_id: s2.medium
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                 greenplum_cluster.segment_subcluster:
                    generation: 2
                    resource_preset_id: s2.medium
                    disk_type_id: local-ssd
                    disk_size: 10737418240
    envoy:
        use_health_map: true
        clusters:
            mdb-internal-api:
                prefix: "/"
                port: 50050
    logship:
        use_rt: false
        topics:
            mdb_internal_api: /aoe9shbqc2v314v7fp3d/controlplane/mdb-internal-api-logs
        use_cloud_logbroker: True
        logbroker_host: lb.cc8035oc71oh9um52mv3.ydb.mdb.cloud-preprod.yandex.net
        logbroker_port: 2135
        database: /pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3
        iam_endpoint: iam.api.cloud-preprod.yandex.net
        lb_producer_key: {{ salt.yav.get('ver-01epvzv7xnja97f4dtp1e1m5nx[lb_producer_key]') | tojson }}
