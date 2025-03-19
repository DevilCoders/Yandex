data:
    common:
        dh: |
            {{ salt.yav.get('ver-01e5w4faf59tjb4vxwag5t41q6[private]') | indent(12) }}
    logship:
        use_rt: false
        topics:
            dbaas_internal_api: '/mdb/porto/prod/iapi-log'
        tvm:
            client_id: 2009907
            server_id: 2001059
            secret: {{ salt.yav.get('ver-01dzb5sa9q83xsrxbyfg926r66[secret]') }}
    internal_api:
        server_name: internal-api.db.yandex-team.ru
        config:
            expose_all_task_errors: true
            internal_schema_fields_expose: true
            crypto:
                api_secret_key: {{ salt.yav.get('ver-01dzb6cytp1gc4nrcdzf5ktse3[secret]') }}
                client_public_key: {{ salt.yav.get('ver-01dzb6cytp1gc4nrcdzf5ktse3[public]') }}
            dispenser_used: true
            generation_names:
                1: Sandy bridge or later
                2: Broadwell or later
                3: Cascade Lake or later
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
            charts:
                postgresql_cluster:
                    - ['YASM', 'YaSM (Golovan) charts', 'https://yasm.yandex-team.ru/template/panel/dbaas_postgres_metrics/cid={cid};dbname={dbname}']
                    - ['Solomon', 'Solomon charts', 'https://solomon.yandex-team.ru/?cluster=mdb_{cid}&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres']
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/{cluster_type}/cluster/{cid}?section=monitoring']
                clickhouse_cluster:
                    - ['YASM', 'YaSM (Golovan) charts', 'https://yasm.yandex-team.ru/template/panel/dbaas_clickhouse_metrics/cid={cid}']
                    - ['Solomon', 'Solomon charts', 'https://solomon.yandex-team.ru/?project=internal-mdb&service=mdb&dashboard=mdb-prod-cluster-clickhouse&cid={cid}']
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/{cluster_type}/cluster/{cid}?section=monitoring']
                mongodb_cluster:
                    - ['YASM', 'YaSM (Golovan) charts', 'https://yasm.yandex-team.ru/template/panel/{yasm_dashboard}/cid={cid}']
                    - ['Solomon', 'Solomon charts', 'https://solomon.yandex-team.ru/?project=internal-mdb&service=mdb&dashboard=mdb-prod-cluster-mongodb&cid={cid}']
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
                        flavor: s3.nano
                        volume_size: 10737418240.0
                        node_count: 3
                        disk_type_id: local-ssd
                    shard_count_limit: 50
                mongodb_cluster: {}
                mysql_cluster: {}
                redis_cluster: {}
            yc_access_service:
                endpoint: as.cloud.yandex-team.ru:4286
                timeout: 2
                ca_path: /opt/yandex/allCAs.pem
            default_vtype: porto
            default_disk_type_ids:
                porto: local-ssd
            decommissioning_zones:
                - myt
                - iva
                - man
            decommissioning_flavors:
                - db1.nano
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
                dsn: {{ salt.yav.get('ver-01dzb6kk45ts3kt4zvm0mxwr43[dsn]') }}
                environment: porto-prod
            default_cloud_quota:
                clusters_quota: 0
                cpu_quota: 0
                memory_quota: 0.0
                io_quota: 0.0
                network_quota: 0.0
                ssd_space_quota: 0.0
                hdd_space_quota: 0.0
                gpu_quota: 0.0
            metadb:
                hosts:
                    - meta01h.db.yandex.net
                    - meta01f.db.yandex.net
                    - meta01k.db.yandex.net
                user: dbaas_api
                password: {{ salt.yav.get('ver-01dzb4nyjp1jkh8gxaftptktjg[password]') }}
                dbname: dbaas_metadb
                port: 6432
                maxconn: 30
                minconn: 10
                sslmode: verify-full
                connect_timeout: 1
            env_mapping:
                qa: prestable
                prod: production
            console_address: https://yc.yandex-team.ru
            mdbhealth:
                url: https://health.db.yandex.net
                connect_timeout: 1
                read_timeout: 6
                ca_certs: /etc/nginx/ssl/allCAs.pem
            s3:
                endpoint_url: 'https://s3.mds.yandex.net'
                aws_access_key_id: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[id]') }}
                aws_secret_access_key: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[secret]') }}
            bucket_prefix: internal-dbaas-
            external_uri_validation:
                regexp: https://(?:[a-zA-Z0-9-]+\.)+(?:yandex\.net|yandexcloud\.net|yandex-team\.ru)/\S+
                message: URI should match regexp "https://(?:[a-zA-Z0-9-]+\.)+(?:yandex\.net|yandexcloud\.net|yandex-team\.ru)/\S+"
            envconfig:
                postgresql_cluster:
                    qa:
                        zk:
                            test:
                                - zkeeper-test01e.db.yandex.net:2181
                                - zkeeper-test01h.db.yandex.net:2181
                                - zkeeper-test01f.db.yandex.net:2181
                            test02:
                                - zkeeper-test02e.db.yandex.net:2181
                                - zkeeper-test02h.db.yandex.net:2181
                                - zkeeper-test02k.db.yandex.net:2181
                    prod:
                        zk:
                            prod:
                                - zkeeper01e.db.yandex.net:2181
                                - zkeeper01h.db.yandex.net:2181
                                - zkeeper01f.db.yandex.net:2181
                            prod02:
                                - zkeeper02e.db.yandex.net:2181
                                - zkeeper02h.db.yandex.net:2181
                                - zkeeper02k.db.yandex.net:2181
                            prod03:
                                - zkeeper03h.db.yandex.net:2181
                                - zkeeper03k.db.yandex.net:2181
                                - zkeeper03f.db.yandex.net:2181
                            prod04:
                                - zkeeper04h.db.yandex.net:2181
                                - zkeeper04k.db.yandex.net:2181
                                - zkeeper04f.db.yandex.net:2181
                            prod05:
                                - zkeeper05h.db.yandex.net:2181
                                - zkeeper05k.db.yandex.net:2181
                                - zkeeper05f.db.yandex.net:2181
                mysql_cluster:
                    qa:
                        zk:
                            test:
                                - zkeeper-test01e.db.yandex.net:2181
                                - zkeeper-test01h.db.yandex.net:2181
                                - zkeeper-test01f.db.yandex.net:2181
                    prod:
                        zk:
                            prod:
                                - zkeeper01e.db.yandex.net:2181
                                - zkeeper01h.db.yandex.net:2181
                                - zkeeper01f.db.yandex.net:2181
                            prod02:
                                - zkeeper02e.db.yandex.net:2181
                                - zkeeper02h.db.yandex.net:2181
                                - zkeeper02k.db.yandex.net:2181
                            prod03:
                                - zkeeper03h.db.yandex.net:2181
                                - zkeeper03k.db.yandex.net:2181
                                - zkeeper03f.db.yandex.net:2181
                            prod04:
                                - zkeeper04h.db.yandex.net:2181
                                - zkeeper04k.db.yandex.net:2181
                                - zkeeper04f.db.yandex.net:2181
                            prod05:
                                - zkeeper05h.db.yandex.net:2181
                                - zkeeper05k.db.yandex.net:2181
                                - zkeeper05f.db.yandex.net:2181
                mongodb_cluster:
                    qa:
                        zk_hosts:
                            - zkeeper-test01e.db.yandex.net
                            - zkeeper-test01h.db.yandex.net
                            - zkeeper-test01f.db.yandex.net
                    prod:
                        zk_hosts:
                            - zkeeper01e.db.yandex.net
                            - zkeeper01h.db.yandex.net
                            - zkeeper01f.db.yandex.net
                redis_cluster:
                    qa:
                        zk_hosts:
                            - zkeeper-test01e.db.yandex.net
                            - zkeeper-test01h.db.yandex.net
                            - zkeeper-test01f.db.yandex.net
                    prod:
                        zk_hosts:
                            - zkeeper01e.db.yandex.net
                            - zkeeper01h.db.yandex.net
                            - zkeeper01f.db.yandex.net
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
                oauth_token: '{{ salt.yav.get('ver-01fwgks26s06gnqa3x4z3h8a07[racktables_oauth]') }}'
            yandex_team_integration_config:
                url: 'ti.cloud.yandex-team.ru:443'
                cert_file: '/opt/yandex/allCAs.pem'
            network:
                vtype: 'porto'
                token: ''
        secret_key: {{ salt.yav.get('ver-01dzb6cytp1gc4nrcdzf5ktse3[secret]') }}
        public_key: {{ salt.yav.get('ver-01dzb6cytp1gc4nrcdzf5ktse3[public]') }}
        s3_endpoint_url: 'https://s3.mds.yandex.net'
        aws_access_key_id: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[id]') }}
        aws_secret_access_key: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[secret]') }}
        metadb_hosts:
            - meta01h.db.yandex.net
            - meta01k.db.yandex.net
            - meta01f.db.yandex.net
        logsdb_hosts:
            # https://yc.yandex-team.ru/folders/fooi5vu9rdejqc3p4b60/managed-clickhouse/cluster/mdbmas2jjul3ct5q39sm
            - man-xp8zytv5tag5domn.db.yandex.net:9440
            - vla-e6dc7map462gttd4.db.yandex.net:9440
            - sas-cef5pwxm4ae7f5jc.db.yandex.net:9440
            - sas-ebpnfmkumz7lbehn.db.yandex.net:9440
        tls_key: |
            {{ salt.yav.get('ver-01dzb5nfs6b9ttg5gpd2stvfq2[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01dzb5nfs6b9ttg5gpd2stvfq2[cert]') | indent(12) }}
