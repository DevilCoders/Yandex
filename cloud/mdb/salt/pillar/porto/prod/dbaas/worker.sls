data:
    dbaas_worker:
        ssh_private_key: |
            {{ salt.yav.get('ver-01g1fv6kh57d8vbznpady41g5a[private]') | indent(12) }}
        config:
            conductor:
                token: {{ salt.yav.get('ver-01g63b99a413gkkw54f8mrch45[token]') }}
            deploy:
                token_v2: {{ salt.yav.get('ver-01g365rkd7es5v97w9n5e1szas[token]') }}
            dbm:
                token: {{ salt.yav.get('ver-01g36bbjbgmn2v31gqk3c8gkrg[token]') }}
            main:
                admin_api_conductor_group: mdb_api_admin_porto_prod
                metadb_hosts:
                    - meta01h.db.yandex.net
                    - meta01k.db.yandex.net
                    - meta01f.db.yandex.net
                api_sec_key: {{ salt.yav.get('ver-01dzb6cytp1gc4nrcdzf5ktse3[secret]') }}
                client_pub_key: {{ salt.yav.get('ver-01dzb6cytp1gc4nrcdzf5ktse3[public]') }}
                sentry_dsn: {{ salt.yav.get('ver-01e08ar9bfyj0cxkvpy26qjrnk[dsn]') }}
                sentry_environment: porto-prod
                max_tasks: 200
            s3:
                access_key_id: ''
                secret_access_key: ''
                backup:
                    endpoint_url: https://s3.mds.yandex.net
                    access_key_id: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[id]') }}
                    secret_access_key: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[secret]') }}
                    allow_lifecycle_policies: true
                cloud_storage:
                    endpoint_url: https://s3.mds.yandex.net
                    access_key_id: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[id]') }}
                    secret_access_key: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[secret]') }}
                    allow_lifecycle_policies: true
                idm_endpoint_url: ''
            cert_api:
                token: {{ salt.yav.get('ver-01dz8nm5w4p4fqfxbczsqcc78k[oauth]') }}
                ca_path: /opt/yandex/allCAs.pem
                url: https://mdb-secrets.db.yandex.net
                api: MDB_SECRETS
                cert_type: mdb
            juggler:
                token: {{ salt.yav.get('ver-01g366shhekqps2r49rapsdz0s[token]') }}
            ssh:
                private_key: /opt/yandex/dbaas-worker/ssh_key
                public_key: 'ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAEfkPwnAQwbUBXTnPz+7qMH5nNQwc3YZmGQs6AggqRjCOryaVHTKzGaz0XC6uMup7zFfTQ69dpHFVN8K4+n7yIZDQDuXohSuxaRpi8sW4ZQji0TxWCE0I193YvePFvkbqcYySVt9kGNucbUgU/n+cGyZqQdRXjKGgWsdeF+zmBuBcu7tg== robot-pgaas-deploy'
            internal_api:
                ca_path: /opt/yandex/allCAs.pem
            compute:
                url: localhost:8080
                ca_path: /opt/yandex/allCAs.pem
                managed_network_id: dummy
                folder_id: dummy
                service_account_id: foobshcpm3f1pcqd5d24
                key_id: foomturefdgnf1ddapsn
                private_key: |
                    {{ salt.yav.get('ver-01eehtzrh6w507z3j8fc33dyt3[private_key]') | indent(20) }}
                use_security_group: False
            vpc:
                ca_path: dummy
                url: localhost:9823
            iam_token_service:
                url: ts.cloud.yandex-team.ru:4282
            iam_access_service:
                url: ''
            iam_service:
                url: ''
            iam_jwt:
                cert_file: /opt/yandex/allCAs.pem
                server_name: ts.cloud.yandex-team.ru
                url: ts.cloud.yandex-team.ru:4282
                service_account_id: foobshcpm3f1pcqd5d24
                key_id: foomturefdgnf1ddapsn
                private_key: {{ salt.yav.get('ver-01eehtzrh6w507z3j8fc33dyt3[private_key]') | yaml_dquote }}
            solomon:
                token: {{ salt.yav.get('ver-01g36bsgqnb8jc5adgr596spzn[token]') }}
                url: 'https://solomon.yandex-team.ru'
                ca_path: /etc/ssl/certs
                service_label: mdb
            mlock:
                enabled: true
                url: mlock.db.yandex.net:443
                cert_file: /opt/yandex/allCAs.pem
                server_name: mlock.db.yandex.net
                service_account_id: foo4dqt4ef444d3ltfnm
                key_id: f6o6hive3t11oefvs67r
                private_key: |
                    {{ salt.yav.get('ver-01ef4cf8n3yj0m61va5jzpwpxq[private_key]') | indent(20) }}
            mongod:
                conductor_root_group: mdb_mongod_porto_prod
            mongocfg:
                conductor_root_group: mdb_mongocfg_porto_prod
            mongos:
                conductor_root_group: mdb_mongos_porto_prod
            mongoinfra:
                conductor_root_group: mdb_mongoinfra_porto_prod
            postgresql:
                conductor_root_group: mdb_postgresql_porto_prod
                dbm_bootstrap_cmd: /usr/local/yandex/porto/mdb_dbaas_pg_bionic.sh
                dbm_bootstrap_cmd_template:
                    template: /usr/local/yandex/porto/mdb_dbaas_pg_{major_version}_bionic.sh
                    task_arg: major_version
                    whitelist:
                        '10': '10'
                        '10-1c': '10-1c'
                        '11': '11'
                        '12': '12'
                        '13': '13'
            clickhouse:
                conductor_root_group: mdb_clickhouse_porto_prod
                juggler_checks:
                    - META
                    - ch_ping
            zookeeper:
                conductor_root_group: mdb_zookeeper_porto_prod
            mysql:
                conductor_root_group: mdb_mysql_porto_prod
                dbm_bootstrap_cmd: /usr/local/yandex/porto/mdb_dbaas_mysql_bionic.sh
            redis:
                conductor_root_group: mdb_redis_porto_prod
                dbm_bootstrap_cmd: /usr/local/yandex/porto/mdb_dbaas_redis_bionic.sh
                dbm_bootstrap_cmd_template:
                    template: /usr/local/yandex/porto/mdb_dbaas_redis_{major_version}_bionic.sh
                    task_arg: major_version
                    whitelist:
                        '5.0': '50'
                        '6.0': '60'
                        '6.2': '62'
                        '7.0': '70'
            elasticsearch_data:
                conductor_root_group: mdb_elasticsearch_data_porto_prod
                dbm_bootstrap_cmd: /usr/local/yandex/porto/mdb_dbaas_elasticsearch_bionic.sh
                dbm_bootstrap_cmd_template:
                    template: /usr/local/yandex/porto/mdb_dbaas_elasticsearch_{major_version}_bionic.sh
                    task_arg: major_version
                    whitelist:
                        '710': '710'
                        '711': '711'
                        '712': '712'
                        '713': '713'
                        '714': '714'
                        '715': '715'
                        '716': '716'
                        '717': '717'
            elasticsearch_master:
                conductor_root_group: mdb_elasticsearch_master_porto_prod
                dbm_bootstrap_cmd: /usr/local/yandex/porto/mdb_dbaas_elasticsearch_bionic.sh
                dbm_bootstrap_cmd_template:
                    template: /usr/local/yandex/porto/mdb_dbaas_elasticsearch_{major_version}_bionic.sh
                    task_arg: major_version
                    whitelist:
                        '710': '710'
                        '711': '711'
                        '712': '712'
                        '713': '713'
                        '714': '714'
                        '715': '715'
                        '716': '716'
                        '717': '717'
            opensearch_data:
                conductor_root_group: mdb_opensearch_data_porto_prod
                dbm_bootstrap_cmd: /usr/local/yandex/porto/mdb_dbaas_opensearch_bionic.sh
                dbm_bootstrap_cmd_template:
                    template: /usr/local/yandex/porto/mdb_dbaas_opensearch_{major_version}_bionic.sh
                    task_arg: major_version
                    whitelist:
                        '21': '21'
            opensearch_master:
                conductor_root_group: mdb_opensearch_master_porto_prod
                dbm_bootstrap_cmd: /usr/local/yandex/porto/mdb_dbaas_opensearch_bionic.sh
                dbm_bootstrap_cmd_template:
                    template: /usr/local/yandex/porto/mdb_dbaas_opensearch_{major_version}_bionic.sh
                    task_arg: major_version
                    whitelist:
                        '21': '21'
            kafka:
                conductor_root_group: mdb_kafka_porto_prod
            greenplum_master:
                conductor_root_group: mdb_greenplum_master_porto_prod
            greenplum_segment:
                conductor_root_group: mdb_greenplum_segment_porto_prod
            cloud_storage:
                folder_id: ''
                service_account_id: ''
                key_id: ''
                private_key: ''
            per_cluster_service_accounts:
                folder_id: ''
