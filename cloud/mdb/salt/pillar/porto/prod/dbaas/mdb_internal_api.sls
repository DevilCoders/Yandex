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
                expose_error_debug: false
                cloud_id_prefix: mdb
            metadb:
                addrs:
                    - meta01h.db.yandex.net:6432
                    - meta01f.db.yandex.net:6432
                    - meta01k.db.yandex.net:6432
                db: dbaas_metadb
                user: dbaas_api
                password: {{ salt.yav.get('ver-01dzb4nyjp1jkh8gxaftptktjg[password]') }}
                sslrootcert: /opt/yandex/allCAs.pem
                max_open_conn: 64
                max_idle_conn: 64
            logsdb:
                addrs:
                    # https://yc.yandex-team.ru/folders/fooi5vu9rdejqc3p4b60/managed-clickhouse/cluster/mdbmas2jjul3ct5q39sm
                    - vla-xp8zytv5tag5domn.db.yandex.net:9440
                    - vla-e6dc7map462gttd4.db.yandex.net:9440
                    - vla-908k4s3q0m0v6sqa.db.yandex.net:9440
                    - sas-cef5pwxm4ae7f5jc.db.yandex.net:9440
                    - sas-ebpnfmkumz7lbehn.db.yandex.net:9440
                    - sas-d5qgpae0c144hpr9.db.yandex.net:9440
                db: mdb
                user: logs_reader
                password: {{ salt.yav.get('ver-01ep74h1bba53q3b1cgqs2c03v[reader_password]') }}
                ca_file: /opt/yandex/allCAs.pem
                time_column: timestamp
            perfdiagdb:
                addrs:
                    - man-socqm4odk1u2fyww.db.yandex.net:9440
                    - sas-3evubsv0shrua2uh.db.yandex.net:9440
                    - vla-b0lnslcod8b7dpmn.db.yandex.net:9440
                db: perf_diag
                user: dbaas_api_reader
                password: {{ salt.yav.get('ver-01eestt7db5fcbdf5f57wm11wm[password]') }}
                ca_file: /opt/yandex/allCAs.pem
                debug: True
            perfdiagdb_mongodb:
                disabled: False
                addrs:
                    - sas-kmgey5hbz8z7kzdf.db.yandex.net:9440
                    - vla-uk5j82t821b128ng.db.yandex.net:9440
                db: perfdiag
                user: dbaas_api_reader
                password: {{ salt.yav.get('ver-01fdhthvxz62pk8nbnwwzq3w8y[password]') }}
                ca_file: /opt/yandex/allCAs.pem
                debug: True
            s3:
                host: s3.mds.yandex.net
                access_key: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[id]') }}
                secret_key: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[secret]') }}
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
                host: health.db.yandex.net
                tls:
                    ca_file: /opt/yandex/allCAs.pem
            crypto:
                private_key: {{ salt.yav.get('ver-01e2nn6xsxtkq0xap7zf87vdfh[private-key]') }}
                public_key: {{ salt.yav.get('ver-01e2nn6xsxtkq0xap7zf87vdfh[public-key]') }}
            sentry:
                dsn: {{ salt.yav.get('ver-01e326z4jsr9q8126qsk54awjj[dsn]') }}
                environment: porto-prod
            logic:
                flags:
                    allow_move_between_clouds: True
                saltenvs:
                    production: prod
                    prestable: qa
                generation_names:
                    1: Sandy bridge or later
                    2: Broadwell or later
                    3: Cascade Lake or later
                elasticsearch:
                    tasks_prefix: d1p
                    allowed_editions:
                        - basic
                resource_validation:
                    decommissioned_resource_presets:
                        - db1.nano
                    decommissioned_zones:
                        - myt
                        - iva
                        - man
                greenplum:
                    tasks_prefix: p0g
                service_zk:
                    hosts:
                        qa:
                            - zkeeper-test01e.db.yandex.net
                            - zkeeper-test01h.db.yandex.net
                            - zkeeper-test01f.db.yandex.net
                        prod:
                            - zkeeper01e.db.yandex.net
                            - zkeeper01h.db.yandex.net
                            - zkeeper01f.db.yandex.net
                console:
                    uri: https://yc.yandex-team.ru
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
                          link: 'https://solomon.yandex-team.ru/?project=internal-mdb&service=mdb&dashboard=mdb-prod-cluster-clickhouse&cid={cid}'
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
                    tasks_prefix: fm2
                    zk_zones:
                        - myt
                        - vla
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
                    external_uri_validation:
                        use_http_client: true
                        regexp: https://(?:[a-zA-Z0-9-]+\.)+(?:yandex\.net|yandexcloud\.net|yandex-team\.ru)/\S+
                        message: URI should match regexp "https://(?:[a-zA-Z0-9-]+\.)+(?:yandex\.net|yandexcloud\.net|yandex-team\.ru)/\S+"
                cluster_stop_supported: false
            racktables:
                endpoint: "https://ro.racktables.yandex-team.ru"
                token: "{{ salt.yav.get('ver-01fwgks26s06gnqa3x4z3h8a07[racktables_oauth]') }}"
            yandex_team_integration:
                uri: "ti.cloud.yandex-team.ru:443"
                config:
                    security:
                        tls:
                            ca_file: "/opt/yandex/allCAs.pem"
        console_default_resources:
            postgresql_cluster:
                postgresql_cluster:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            clickhouse_cluster:
                clickhouse_cluster:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                zk:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            mongodb_cluster:
                mongodb_cluster:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongod:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongos:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongocfg:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                mongodb_cluster.mongoinfra:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            mysql_cluster:
                mysql_cluster:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            redis_cluster:
                redis_cluster:
                    generation: 3
                    resource_preset_id: m3.nano
                    disk_type_id: local-ssd
                    disk_size: 17179869184
            kafka_cluster:
                kafka_cluster:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 34359738368
                zk:
                    generation: 1
                    resource_preset_id: d1.micro
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            elasticsearch_cluster:
                 elasticsearch_cluster.datanode:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
                 elasticsearch_cluster.masternode:
                    generation: 3
                    resource_preset_id: s3.nano
                    disk_type_id: local-ssd
                    disk_size: 10737418240
            greenplum_cluster:
                 greenplum_cluster.master_subcluster:
                    generation: 3
                    resource_preset_id: s3.medium
                    disk_type_id: local-ssd
                    disk_size: 34359738368
                 greenplum_cluster.segment_subcluster:
                    generation: 3
                    resource_preset_id: s3.medium
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
            mdb_internal_api: '/mdb/porto/prod/iapi-grpc-log'
        tvm:
            client_id: 2018488
            server_id: 2001059
            secret: {{ salt.yav.get('ver-01e0t0f74qeym9b3k9jw8b0pg9[client_secret]') }}
