data:
    common:
        dh: |
            {{ salt.yav.get('ver-01e9e4m8pt4awh6zph0995h8fs[private]') | indent(12) }}
    logship:
        use_rt: false
        topics:
            dbaas_internal_api: '/mdb/porto/test/iapi-log'
        tvm:
            client_id: 2009893
            server_id: 2001059
            secret: {{ salt.yav.get('ver-01e9e23mpskg2k3m6f30dksfa2[secret]') }}
    internal_api:
        use_arcadia_build: true
        server_name: internal-api-test.db.yandex-team.ru
        config:
            expose_all_task_errors: true
            e2e:
                cluster_name: dbaas_e2e_porto_qa
                folder_id: foorv7rnqd9sfo4q6db4
            internal_schema_fields_expose: true
            crypto:
                api_secret_key: {{ salt.yav.get('ver-01e9e21af3yktpmnftm6q0t7cp[secret]') }}
                client_public_key: {{ salt.yav.get('ver-01e9e21af3yktpmnftm6q0t7cp[public]') }}
            dispenser_used: true
            generation_names:
                1: Sandy bridge or later
                2: Broadwell or later
                3: Cascade Lake or later
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
            charts:
                postgresql_cluster:
                    - ['YASM', 'YaSM (Golovan) charts', 'https://yasm.yandex-team.ru/template/panel/dbaas_postgres_metrics/cid={cid};dbname={dbname}']
                    - ['Solomon', 'Solomon charts', 'https://solomon.yandex-team.ru/?cluster=mdb_{cid}&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres']
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/{cluster_type}/cluster/{cid}?section=monitoring']
                clickhouse_cluster:
                    - ['YASM', 'YaSM (Golovan) charts', 'https://yasm.yandex-team.ru/template/panel/dbaas_clickhouse_metrics/cid={cid}']
                    - ['Solomon', 'Solomon charts', 'https://solomon.yandex-team.ru/?project=internal-mdb&service=mdb&dashboard=mdb-test-cluster-clickhouse&cid={cid}']
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/{cluster_type}/cluster/{cid}?section=monitoring']
                mongodb_cluster:
                    - ['YASM', 'YaSM (Golovan) charts', 'https://yasm.yandex-team.ru/template/panel/{yasm_dashboard}/cid={cid}']
                    - ['Solomon', 'Solomon charts', 'https://solomon.yandex-team.ru/?project=internal-mdb&service=mdb&dashboard=mdb-test-cluster-mongodb&cid={cid}']
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/{cluster_type}/cluster/{cid}?section=monitoring']
                mysql_cluster:
                    - ['YASM', 'YaSM (Golovan) charts', 'https://yasm.yandex-team.ru/template/panel/dbaas_mysql_metrics/cid={cid}']
                    - ['Solomon', 'Solomon charts', 'https://solomon.yandex-team.ru/?cluster=mdb_{cid}&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mysql']
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/{cluster_type}/cluster/{cid}?section=monitoring']
                redis_cluster:
                    - ['Solomon', 'Solomon charts', 'https://solomon.yandex-team.ru/?cluster=mdb_{cid}&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-redis']
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/{cluster_type}/cluster/{cid}?section=monitoring']
            ctype_config:
                postgresql_cluster: {}
                clickhouse_cluster:
                    zk:
                        flavor: s2.nano
                        volume_size: 10737418240.0
                        node_count: 3
                        disk_type_id: local-ssd
                    shard_count_limit: 50
                mongodb_cluster: {}
            console_address: https://yc-test.yandex-team.ru
            yc_access_service:
                endpoint: as.cloud.yandex-team.ru:4286
                timeout: 2
                ca_path: /opt/yandex/allCAs.pem
            default_vtype: porto
            default_disk_type_ids:
                porto: local-ssd
            decommissioning_zones:
                - man
                - vla
            decommissioning_flavors:
                - db1.nano
            iam_jwt_config:
                url: ts.cloud.yandex-team.ru:4282
                cert_file: "/opt/yandex/allCAs.pem"
                server_name: ts.cloud.yandex-team.ru
                service_account_id: {{ salt.yav.get('ver-01eyr36a2ca25xzjmked74vha9[id]') }}
                key_id: {{ salt.yav.get('ver-01eyr36a2ca25xzjmked74vha9[key_id]') }}
                private_key: {{ salt.yav.get('ver-01eyr36a2ca25xzjmked74vha9[private_key]') | yaml_encode }}
                insecure: False
                audience: https://iam.api.cloud.yandex.net/iam/v1/tokens
                expire_thresh: 180
                request_expire: 3600
            sentry:
                dsn: {{ salt.yav.get('ver-01e9e1vd49t2fse8h5wddbtsv7[dsn]') }}
                environment: porto-test
            identity:
                override_folder: null
                override_cloud: null
                create_missing: True
                allow_move_between_clouds: True
            yc_identity:
                base_url: https://iam.cloud.yandex-team.ru
                connect_timeout: 2
                read_timeout: 5
                token: ''
                ca_path: /opt/yandex/allCAs.pem
            resource_manager_config_grpc:
                url: rm.cloud.yandex-team.ru:443
                cert_file: /opt/yandex/allCAs.pem
            metadb:
                hosts:
                    - meta-test01f.db.yandex.net
                    - meta-test01h.db.yandex.net
                    - meta-test01k.db.yandex.net
                user: dbaas_api
                password: {{ salt.yav.get('ver-01e9dwm1s2c9p4fqd7b1d1x4nm[password]') }}
                dbname: dbaas_metadb
                port: 6432
                maxconn: 30
                minconn: 10
                sslmode: verify-full
                connect_timeout: 1
            default_cloud_quota:
                clusters_quota: 0
                cpu_quota: 0
                memory_quota: 0.0
                io_quota: 0.0
                network_quota: 0.0
                ssd_space_quota: 0.0
                hdd_space_quota: 0.0
                gpu_quota: 0.0
            env_mapping:
                dev: prestable
                qa: production
            mdbhealth:
                url: https://mdb-health-test.db.yandex.net
                connect_timeout: 1
                read_timeout: 2
                ca_certs: /etc/nginx/ssl/allCAs.pem
            s3:
                endpoint_url: 'https://s3.mds.yandex.net'
                aws_access_key_id: {{ salt.yav.get('ver-01fvtrs66pp25nvc2stnys7vf6[id]') }}
                aws_secret_access_key: {{ salt.yav.get('ver-01fvtrs66pp25nvc2stnys7vf6[secret]') }}
            bucket_prefix: internal-dbaas-
            external_uri_validation:
                regexp: https://(?:[a-zA-Z0-9-]+\.)+(?:yandex\.net|yandexcloud\.net|yandex-team\.ru)/\S+
                message: URI should match regexp "https://(?:[a-zA-Z0-9-]+\.)+(?:yandex\.net|yandexcloud\.net|yandex-team\.ru)/\S+"
            envconfig:
                postgresql_cluster:
                    dev:
                        zk:
                            test:
                                - zk-df-e2e01k.db.yandex.net:2181
                                - zk-df-e2e01f.db.yandex.net:2181
                                - zk-df-e2e01h.db.yandex.net:2181
                    qa:
                        zk:
                            df_e2e:
                                - zk-df-e2e01k.db.yandex.net:2181
                                - zk-df-e2e01f.db.yandex.net:2181
                                - zk-df-e2e01h.db.yandex.net:2181
                mysql_cluster:
                    dev:
                        zk:
                            test:
                                - zk-df-e2e01k.db.yandex.net:2181
                                - zk-df-e2e01f.db.yandex.net:2181
                                - zk-df-e2e01h.db.yandex.net:2181
                    qa:
                        zk:
                            df_e2e:
                                - zk-df-e2e01k.db.yandex.net:2181
                                - zk-df-e2e01f.db.yandex.net:2181
                                - zk-df-e2e01h.db.yandex.net:2181
                mongodb_cluster:
                    dev:
                        zk_hosts:
                            - zk-df-e2e01k.db.yandex.net
                            - zk-df-e2e01f.db.yandex.net
                            - zk-df-e2e01h.db.yandex.net
                    qa:
                        zk_hosts:
                            - zk-df-e2e01k.db.yandex.net
                            - zk-df-e2e01f.db.yandex.net
                            - zk-df-e2e01h.db.yandex.net
                redis_cluster:
                    dev:
                        zk_hosts:
                            - zk-df-e2e01k.db.yandex.net
                            - zk-df-e2e01f.db.yandex.net
                            - zk-df-e2e01h.db.yandex.net
                    qa:
                        zk_hosts:
                            - zk-df-e2e01k.db.yandex.net
                            - zk-df-e2e01f.db.yandex.net
                            - zk-df-e2e01h.db.yandex.net
            default_proxy_cluster_pillar_template:
                postgresql_cluster:
                    id: null
                    dbname: null
                    db_port: 6432
                    max_lag: 30
                    max_sessions: 1000
                    auth_user: null
                    auth_dbname: 'postgres'
                    zk_cluster: null
                    direct: False
                    acl:
                        - type: subnet
                          value: ::/0
            versions:
                redis_cluster:
                    - version: '5.0'
                      deprecated: true
                      allow_deprecated_feature_flag: 'MDB_REDIS_ALLOW_DEPRECATED_5'
                    - version: '6.0'
                      deprecated: true
                      allow_deprecated_feature_flag: 'MDB_REDIS_ALLOW_DEPRECATED_6'
                    - version: '6.2'
                      feature_flag: 'MDB_REDIS_62'
                    - version: '7.0'
                      feature_flag: 'MDB_REDIS_70'
            default_backup_schedule:
                clickhouse_cluster:
                    start:
                        hours: 23
                        minutes: 0
                        seconds: 0
                        nanos: 0
                    sleep: 1800
                mongodb_cluster:
                    start:
                        hours: 22
                        minutes: 0
                        seconds: 0
                        nanos: 0
                    sleep: 7200
                    retain_period: 7
                redis_cluster:
                    start:
                        hours: 22
                        minutes: 0
                        seconds: 0
                        nanos: 0
                    sleep: 7200
                postgresql_cluster:
                    start:
                        hours: 22
                        minutes: 0
                        seconds: 0
                        nanos: 0
                    sleep: 7200
                mysql_cluster:
                    start:
                        hours: 22
                        minutes: 0
                        seconds: 0
                        nanos: 0
                    sleep: 7200
            racktables_client_config:
                base_url: 'https://ro.racktables.yandex-team.ru'
                oauth_token: '{{ salt.yav.get('ver-01fwe2snxgrvmbdbcg9m1kf2sd[racktables_oauth]') }}'
            yandex_team_integration_config:
                url: 'ti.cloud.yandex-team.ru:443'
                cert_file: '/opt/yandex/allCAs.pem'
            network:
                vtype: 'porto'
                token: ''
        tls_key: |
            {{ salt.yav.get('ver-01e9e1gfz3z8wd8wqr3q31n2se[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01e9e1gfz3z8wd8wqr3q31n2se[cert]') | indent(12) }}
