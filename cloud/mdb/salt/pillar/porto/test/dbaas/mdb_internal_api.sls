data:
    mdb-internal-api:
        service_account:
            id: {{ salt.yav.get('ver-01eyr36a2ca25xzjmked74vha9[id]') }}
            key_id: {{ salt.yav.get('ver-01eyr36a2ca25xzjmked74vha9[key_id]') }}
            private_key: {{ salt.yav.get('ver-01eyr36a2ca25xzjmked74vha9[private_key]') | yaml_encode }}
        config:
            app:
                logging:
                    level: debug
            api:
                expose_error_debug: true
                cloud_id_prefix: mdb
            metadb:
                addrs:
                    - meta-test01f.db.yandex.net:6432
                    - meta-test01h.db.yandex.net:6432
                    - meta-test01k.db.yandex.net:6432
                db: dbaas_metadb
                user: dbaas_api
                password: {{ salt.yav.get('ver-01e0t86skk628wjvv36zt1kqha[password]') }}
                sslrootcert: /opt/yandex/allCAs.pem
                max_open_conn: 64
                max_idle_conn: 64
            logsdb:
                addrs:
                    # https://yc.yandex-team.ru/folders/fooi5vu9rdejqc3p4b60/managed-clickhouse/cluster/mdbnq9ar9md7hmkg2kju
                    - man-p5s5gkgoynizguay.db.yandex.net:9440
                    - vla-tx4eotjm7uxp1sms.db.yandex.net:9440
                    - sas-oeu3gejg7jcymg2j.db.yandex.net:9440
                db: mdb
                user: logs_reader
                password: {{ salt.yav.get('ver-01enjmx4h74c8bfte0xdvect8r[reader_password]') }}
                ca_file: /opt/yandex/allCAs.pem
                debug: True
                time_column: timestamp
            perfdiagdb:
                addrs:
                    - man-6uvl3ir1p8leh83j.db.yandex.net:9440
                    - sas-q8j9hlv5nqyminkf.db.yandex.net:9440
                    - vla-v8pg3kiu3ni6m40p.db.yandex.net:9440
                db: perf_diag
                user: dbaas_api_reader
                password: {{ salt.yav.get('ver-01eestjap7qt200r2xdjs019fq[password]') }}
                ca_file: /opt/yandex/allCAs.pem
                debug: True
            perfdiagdb_mongodb:
                disabled: False
                addrs:
                    # https://yc.yandex-team.ru/folders/fooi5vu9rdejqc3p4b60/managed-clickhouse/cluster/mdbsho6mpkr786ktf9e8?section=hosts
                    - sas-kmgey5hbz8z7kzdf.db.yandex.net:9440
                    - vla-uk5j82t821b128ng.db.yandex.net:9440
                db: perfdiag
                user: dbaas_api_reader
                password: {{ salt.yav.get('ver-01fdhthvxz62pk8nbnwwzq3w8y[password]') }}
                ca_file: /opt/yandex/allCAs.pem
                debug: True
            s3:
                host: s3.mds.yandex.net
                access_key: {{ salt.yav.get('ver-01fvtrs66pp25nvc2stnys7vf6[id]') }}
                secret_key: {{ salt.yav.get('ver-01fvtrs66pp25nvc2stnys7vf6[secret]') }}
            access_service:
                addr: as.cloud.yandex-team.ru:4286
                capath: /opt/yandex/allCAs.pem
            token_service:
                addr: ts.cloud.yandex-team.ru:4282
                capath: /opt/yandex/allCAs.pem
            resource_manager:
                addr: rm.cloud.yandex-team.ru:443
                capath: /opt/yandex/allCAs.pem
            iam:
                uri: https://iam.cloud.yandex-team.ru
                http:
                    transport:
                        tls:
                            ca_file: /opt/yandex/allCAs.pem
            health:
                host: mdb-health-test.db.yandex.net
                tls:
                    ca_file: /opt/yandex/allCAs.pem
            crypto:
                private_key: {{ salt.yav.get('ver-01e2nkp8mc9rvxwchpd7wbzyv5[private-key]') }}
                public_key: {{ salt.yav.get('ver-01e2nkp8mc9rvxwchpd7wbzyv5[public-key]') }}
            sentry:
                dsn: {{ salt.yav.get('ver-01e326z4jsr9q8126qsk54awjj[dsn]') }}
                environment: porto-test
            logic:
                flags:
                    allow_move_between_clouds: True
                e2e:
                    cluster_name: dbaas_e2e_porto_qa
                    folder_id: foorv7rnqd9sfo4q6db4
                greenplum:
                    tasks_prefix: t0g
                elasticsearch:
                    allowed_editions:
                        - basic
                saltenvs:
                    production: qa
                    prestable: dev
                generation_names:
                    1: Sandy bridge or later
                    2: Broadwell or later
                    3: Cascade Lake or later
                resource_validation:
                    decommissioned_resource_presets:
                        - db1.nano
                    decommissioned_zones:
                        - man
                        - vla
                service_zk:
                    hosts:
                        dev:
                            - zk-df-e2e01k.db.yandex.net
                            - zk-df-e2e01f.db.yandex.net
                            - zk-df-e2e01h.db.yandex.net
                        qa:
                            - zk-df-e2e01k.db.yandex.net
                            - zk-df-e2e01f.db.yandex.net
                            - zk-df-e2e01h.db.yandex.net
                console:
                    uri: https://yc-test.yandex-team.ru
                monitoring:
                    charts:
                        postgresql:
                        - name: YASM
                          description: YaSM (Golovan) charts
                          link: 'https://yasm.yandex-team.ru/template/panel/dbaas_postgres_metrics/cid={cid};dbname={dbname}'
                        - name: Solomon
                          description: Solomon charts
                          link: 'https://solomon.yandex-team.ru/?cluster=mdb_{cid}&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres'
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-postgresql/cluster/{cid}/monitoring'
                        clickhouse:
                        - name: YASM
                          description: YaSM (Golovan) charts
                          link: 'https://yasm.yandex-team.ru/template/panel/dbaas_clickhouse_metrics/cid={cid}'
                        - name: Solomon
                          description: Solomon charts
                          link: 'https://solomon.yandex-team.ru/?project=internal-mdb&service=mdb&dashboard=mdb-test-cluster-clickhouse&cid={cid}'
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-clickhouse/cluster/{cid}/monitoring'
                        mongodb:
                        - name: YASM
                          description: YaSM (Golovan) charts
                          link: 'https://yasm.yandex-team.ru/template/panel/dbaas_mongodb_metrics/cid={cid}'
                        - name: Solomon
                          description: Solomon charts
                          link: 'https://solomon.yandex-team.ru/?cluster=mdb_{cid}&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mongodb'
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-mongodb/cluster/{cid}/monitoring'
                        redis:
                        - name: Solomon
                          description: Solomon charts
                          link: 'https://solomon.yandex-team.ru/?cluster=mdb_{cid}&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-redis'
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-redis/cluster/{cid}/monitoring'
                        mysql:
                        - name: Solomon
                          description: Solomon charts
                          link: 'https://solomon.yandex-team.ru/?cluster=mdb_{cid}&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mysql'
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-mysql/cluster/{cid}/monitoring'
                        kafka:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-kafka/cluster/{cid}/monitoring'
                        elasticsearch:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-elasticsearch/cluster/{cid}/monitoring'
                        greenplum:
                        - name: Console
                          description: Console charts
                          link: '{console}/folders/{folderExtID}/managed-greenplum/cluster/{cid}/monitoring'
                kafka:
                    zk_zones:
                        - iva
                        - myt
                        - sas
                cloud_default_quota:
                    clusters: 4096
                    cpu: 0
                    memory: 0
                    io: 0
                    network: 0
                    ssd_space: 0
                    hdd_space: 0
                    gpu: 0
                clickhouse:
                    use_backup_service: true
                    external_uri_validation:
                        use_http_client: true
                        regexp: https://(?:[a-zA-Z0-9-]+\.)+(?:yandex\.net|yandexcloud\.net|yandex-team\.ru)/\S+
                        message: URI should match regexp "https://(?:[a-zA-Z0-9-]+\.)+(?:yandex\.net|yandexcloud\.net|yandex-team\.ru)/\S+"
                cluster_stop_supported: false
            racktables:
                endpoint: "https://ro.racktables.yandex-team.ru"
                token: "{{ salt.yav.get('ver-01fwe2snxgrvmbdbcg9m1kf2sd[racktables_oauth]') }}"
            yandex_team_integration:
                uri: "ti.cloud.yandex-team.ru:443"
                config:
                    security:
                        tls:
                            ca_file: "/opt/yandex/allCAs.pem"
        console_default_resources:
            postgresql_cluster:
                postgresql_cluster:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            clickhouse_cluster:
                clickhouse_cluster:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                zk:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            mongodb_cluster:
                mongodb_cluster:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongod:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongos:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongocfg:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongoinfra:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            mysql_cluster:
                mysql_cluster:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            redis_cluster:
                redis_cluster:
                    generation: 2
                    resource_preset_id: m2.nano
                    disk_type_id: local-ssd
                    disk_size: 17179869184
            kafka_cluster:
                kafka_cluster:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 34359738368
                zk:
                    generation: 1
                    resource_preset_id: d1.micro
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            elasticsearch_cluster:
                 elasticsearch_cluster.datanode:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                 elasticsearch_cluster.masternode:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            greenplum_cluster:
                 greenplum_cluster.master_subcluster:
                    generation: 2
                    resource_preset_id: s2.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                 greenplum_cluster.segment_subcluster:
                    generation: 2
                    resource_preset_id: s2.nano
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
            mdb_internal_api: '/mdb/porto/test/iapi-grpc-log'
        tvm:
            client_id: 2018486
            server_id: 2001059
            secret: {{ salt.yav.get('ver-01e0t0enj9rfsy2saqpq0qm11g[client_secret]') }}
