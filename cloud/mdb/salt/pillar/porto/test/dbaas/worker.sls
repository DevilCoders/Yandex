data:
    dbaas_worker:
        ssh_private_key: |
            {{ salt.yav.get('ver-01g111r421gwcj60am59xpgqzg[private]') | indent(12) }}
        config:
            conductor:
                token: {{ salt.yav.get('ver-01g63b99a413gkkw54f8mrch45[token]') }}
            deploy:
                version: 2
                url_v2: https://deploy-api-test.db.yandex-team.ru
                group: porto-test
                token_v2: {{ salt.yav.get('ver-01g365rr0qc9h8epebkyr0nm6h[token_v2]') }}
            dbm:
                url: https://mdb-test.db.yandex-team.ru/
                token: {{ salt.yav.get('ver-01g36bbq5z2kk40ep2d66eb9c0[token]') }}
            main:
                admin_api_conductor_group: mdb_api_admin_porto_test
                metadb_hosts:
                    - meta-test01f.db.yandex.net
                    - meta-test01h.db.yandex.net
                    - meta-test01k.db.yandex.net
                api_sec_key: {{ salt.yav.get('ver-01e9e21af3yktpmnftm6q0t7cp[secret]') }}
                client_pub_key: {{ salt.yav.get('ver-01e9e21af3yktpmnftm6q0t7cp[public]') }}
                sentry_dsn: {{ salt.yav.get('ver-01e9e2szezdpr18ehhgvkjw3vq[dsn]') }}
                sentry_environment: porto-test
                log_level: DEBUG
            s3:
                access_key_id: ''
                secret_access_key: ''
                backup:
                    endpoint_url: https://s3.mds.yandex.net
                    access_key_id: {{ salt.yav.get('ver-01fvtrs66pp25nvc2stnys7vf6[id]') }}
                    secret_access_key: {{ salt.yav.get('ver-01fvtrs66pp25nvc2stnys7vf6[secret]') }}
                    allow_lifecycle_policies: true
                cloud_storage:
                    endpoint_url: https://s3.mds.yandex.net
                    access_key_id: {{ salt.yav.get('ver-01fvtrs66pp25nvc2stnys7vf6[id]') }}
                    secret_access_key: {{ salt.yav.get('ver-01fvtrs66pp25nvc2stnys7vf6[secret]') }}
                    allow_lifecycle_policies: true
                idm_endpoint_url: ''
            cert_api:
                # share it with Salt ext_pillar
                token: {{ salt.yav.get('ver-01dtvcq2g67tyw5c6n0a0b5y0s[oauth]') }}
                ca_path: /opt/yandex/allCAs.pem
                url: https://mdb-secrets-test.db.yandex.net
                api: MDB_SECRETS
            juggler:
                token: {{ salt.yav.get('ver-01g366s0xzbq0qr6vamev1wf3k[token]') }}
            ssh:
                private_key: /opt/yandex/dbaas-worker/ssh_key
                public_key: 'ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAA8YdlDF+hJ7U6eeVNl3SD8wEOfvh9f6cW0mp8xP8hyd5YdU3/3t50zm3eJsRkWvsPxK29KcVPv90NA0N6fQpFE4wGFNBu4rl7Y5JG2NJIWkU06PZW0MNDKaYHD4cDgdUkbJoNTddo+xx6/jdyrasaHrxRmhzq7oZYjNzR9HoQhhVqRew== robot-pgaas-deploy'
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
                ca_path: /opt/yandex/allCAs.pem
                url: localhost:8080
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
                token: {{ salt.yav.get('ver-01g36bsc1gnyzx5bxxq3x7n0hq[token]') }}
                url: 'https://solomon.yandex-team.ru'
                ca_path: /etc/ssl/certs
                service_label: mdb
            mlock:
                enabled: true
                url: mlock-test.db.yandex.net:443
                cert_file: /opt/yandex/allCAs.pem
                server_name: mlock-test.db.yandex.net
                service_account_id: foo4dqt4ef444d3ltfnm
                key_id: f6o6hive3t11oefvs67r
                private_key: |
                    {{ salt.yav.get('ver-01ef4cf8n3yj0m61va5jzpwpxq[private_key]') | indent(20) }}
            mongod:
                conductor_root_group: mdb_mongod_porto_test
                juggler_checks:
                    - META
                    - mongodb_ping
                    - mongodb_primary_exists
                    - mongodb_locked_queue
                    - mongodb_replication_lag
            mongos:
                conductor_root_group: mdb_mongos_porto_test
                juggler_checks:
                    - META
            mongocfg:
                conductor_root_group: mdb_mongocfg_porto_test
                juggler_checks:
                    - META
                    - mongocfg_up
            mongoinfra:
                conductor_root_group: mdb_mongoinfra_porto_test
                juggler_checks:
                    - META
                    - mongocfg_up
            postgresql:
                conductor_root_group: mdb_postgresql_porto_test
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
                juggler_checks:
                    - META
                    - pg_ping
                    - pg_replication_alive
                    - pg_replication_lag
                    - pg_xlog_files
            clickhouse:
                conductor_root_group: mdb_clickhouse_porto_test
                juggler_checks:
                    - META
                    - ch_ping
            zookeeper:
                conductor_root_group: mdb_zookeeper_porto_test
                juggler_checks:
                    - META
                    - zk_alive
            mysql:
                conductor_root_group: mdb_mysql_porto_test
                dbm_bootstrap_cmd: /usr/local/yandex/porto/mdb_dbaas_mysql_bionic.sh
                juggler_checks:
                    - META
                    - mysql_ping
            redis:
                conductor_root_group: mdb_redis_porto_test
                dbm_bootstrap_cmd: /usr/local/yandex/porto/mdb_dbaas_redis_bionic.sh
                dbm_bootstrap_cmd_template:
                    template: /usr/local/yandex/porto/mdb_dbaas_redis_{major_version}_bionic.sh
                    task_arg: major_version
                    whitelist:
                        '5.0': '50'
                        '6.0': '60'
                        '6.2': '62'
                        '7.0': '70'
                juggler_checks:
                    - META
                    - redis_alive
                    - redis_lag
                    - redis_master
            elasticsearch_data:
                rootfs_space_limit: 10737418240
                conductor_root_group: mdb_elasticsearch_data_porto_test
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
                juggler_checks:
                    - META
            elasticsearch_master:
                rootfs_space_limit: 10737418240
                conductor_root_group: mdb_elasticsearch_master_porto_test
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
                juggler_checks:
                    - META
            opensearch_data:
                rootfs_space_limit: 10737418240
                conductor_root_group: mdb_opensearch_data_porto_test
                dbm_bootstrap_cmd: /usr/local/yandex/porto/mdb_dbaas_opensearch_bionic.sh
                dbm_bootstrap_cmd_template:
                    template: /usr/local/yandex/porto/mdb_dbaas_opensearch_{major_version}_bionic.sh
                    task_arg: major_version
                    whitelist:
                        '21': '21'
                juggler_checks:
                    - META
            opensearch_master:
                rootfs_space_limit: 10737418240
                conductor_root_group: mdb_opensearch_master_porto_test
                dbm_bootstrap_cmd: /usr/local/yandex/porto/mdb_dbaas_opensearch_bionic.sh
                dbm_bootstrap_cmd_template:
                    template: /usr/local/yandex/porto/mdb_dbaas_opensearch_{major_version}_bionic.sh
                    task_arg: major_version
                    whitelist:
                        '21': '21'
                juggler_checks:
                    - META
            kafka:
                conductor_root_group: mdb_kafka_porto_test
            greenplum_master:
                conductor_root_group: mdb_greenplum_master_porto_test
            greenplum_segment:
                conductor_root_group: mdb_greenplum_segment_porto_test
            cloud_storage:
                folder_id: ''
                service_account_id: ''
                key_id: ''
                private_key: ''
            per_cluster_service_accounts:
                folder_id: ''
