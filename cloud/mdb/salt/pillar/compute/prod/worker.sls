data:
    dbaas_worker:
        ssh_private_key: ''
        config:
            cert_api:
                token: ''
                ca_name: YcInternalCA
                cert_type: yc-server
                url: https://mdb-secrets.private-api.cloud.yandex.net
                api: MDB_SECRETS
                ca_path: /opt/yandex/allCAs.pem
            postgresql:
                conductor_root_group: mdb_postgresql_compute_prod
                conductor_alt_groups:
                    - group_name: mdb_postgresql_burst_compute_prod
                      matcher:
                          key: flavor
                          values:
                              - b1.medium
                              - b1.micro
                              - b1.nano
                              - b1.small
                              - b2.medium
                              - b2.micro
                              - b2.nano
                              - b2.small
                group_dns:
                    - pattern: 'c-{id}.rw.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.mdb.yandexcloud.net'
                      id: 'cid'
                compute_image_type_template:
                    template: postgresql-{major_version}
                    task_arg: major_version
                    whitelist:
                        '10': '10'
                        '10-1c': '10-1c'
                        '11': '11'
                        '12': '12'
                        '13': '13'
                        '14': '14'
            clickhouse:
                conductor_root_group: mdb_clickhouse_compute_prod
                conductor_alt_groups:
                    - group_name: mdb_clickhouse_burst_compute_prod
                      matcher:
                          key: flavor
                          values:
                              - b1.medium
                              - b1.micro
                              - b1.nano
                              - b1.small
                              - b2.medium
                              - b2.micro
                              - b2.nano
                              - b2.small
                group_dns:
                    - pattern: 'c-{id}.rw.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.yandexcloud.net'
                      id: 'cid'
                    - pattern: '{id}.c-{cid}.rw.db.yandexcloud.net'
                      id: 'shard_name'
                    - pattern: '{id}.c-{cid}.rw.mdb.yandexcloud.net'
                      id: 'shard_name'
                juggler_checks:
                    - META
                    - ch_ping
            redis:
                conductor_root_group: mdb_redis_compute_prod
                conductor_alt_groups:
                    - group_name: mdb_redis_burst_compute_prod
                      matcher:
                          key: flavor
                          values:
                              - b1.medium
                              - b1.micro
                              - b1.nano
                              - b1.small
                              - b2.medium
                              - b2.micro
                              - b2.nano
                              - b2.small
                group_dns:
                    - pattern: 'c-{id}.rw.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.yandexcloud.net'
                      id: 'cid'
                compute_image_type_template:
                    template: redis-{major_version}
                    task_arg: major_version
                    whitelist:
                        '5.0': '50'
                        '6.0': '60'
                        '6.2': '62'
                        '7.0': '70'
            mongod:
                conductor_root_group: mdb_mongodb_compute_prod
                conductor_alt_groups:
                    - group_name: mdb_mongodb_burst_compute_prod
                      matcher:
                          key: flavor
                          values:
                              - b1.micro
                              - b1.nano
                              - b1.small
                              - b2.micro
                              - b2.nano
                              - b2.small
                compute_recreate_on_resize: false
            mongos:
                conductor_root_group: mdb_mongos_compute_prod
            mongocfg:
                conductor_root_group: mdb_mongocfg_compute_prod
                compute_recreate_on_resize: false
            mongoinfra:
                conductor_root_group: mdb_mongoinfra_compute_prod
                compute_recreate_on_resize: false
            zookeeper:
                conductor_root_group: mdb_zookeeper_compute_prod
                conductor_alt_groups:
                    - group_name: mdb_zookeeper_burst_compute_prod
                      matcher:
                          key: flavor
                          values:
                              - b1.medium
                              - b1.micro
                              - b1.nano
                              - b1.small
                              - b2.medium
                              - b2.micro
                              - b2.nano
                              - b2.small
            mysql:
                conductor_root_group: mdb_mysql_compute_prod
                conductor_alt_groups:
                    - group_name: mdb_mysql_burst_compute_prod
                      matcher:
                          key: flavor
                          values:
                              - b1.medium
                              - b1.micro
                              - b1.nano
                              - b1.small
                              - b2.medium
                              - b2.micro
                              - b2.nano
                              - b2.small
                group_dns:
                    - pattern: 'c-{id}.rw.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.mdb.yandexcloud.net'
                      id: 'cid'
            sqlserver:
                compute_root_disk_type_id: 'network-ssd'
                conductor_root_group: mdb_sqlserver_compute_prod
                group_dns:
                    - pattern: 'c-{id}.rw.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.mdb.yandexcloud.net'
                      id: 'cid'
                compute_image_type_template:
                    template: sqlserver-{major_version}
                    task_arg: major_version
                    whitelist:
                        '2016sp2std': '2016sp2std'
                        '2016sp2ent': '2016sp2ent'
                        '2017std':    '2017std'
                        '2017ent':    '2017ent'
                        '2019std':    '2019std'
                        '2019ent':    '2019ent'
            elasticsearch_data:
                conductor_root_group: mdb_elasticsearch_data_compute_prod
                group_dns:
                    - pattern: 'c-{id}.rw.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.yandexcloud.net'
                      id: 'cid'
                compute_image_type_template:
                    template: elasticsearch-{major_version}
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
                conductor_root_group: mdb_elasticsearch_master_compute_prod
                group_dns:
                    - pattern: 'c-{id}.rw.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.yandexcloud.net'
                      id: 'cid'
                compute_image_type_template:
                    template: elasticsearch-{major_version}
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
                conductor_root_group: mdb_opensearch_data_compute_prod
                group_dns:
                    - pattern: 'c-{id}.rw.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.yandexcloud.net'
                      id: 'cid'
                compute_image_type_template:
                    template: opensearch-{major_version}
                    task_arg: major_version
                    whitelist:
                        '21': '21'
            opensearch_master:
                conductor_root_group: mdb_opensearch_master_compute_prod
                group_dns:
                    - pattern: 'c-{id}.rw.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.yandexcloud.net'
                      id: 'cid'
                compute_image_type_template:
                    template: opensearch-{major_version}
                    task_arg: major_version
                    whitelist:
                        '21': '21'
            kafka:
                group_dns:
                    - pattern: 'c-{id}.mdb.yandexcloud.net'
                      id: 'cid'
                conductor_root_group: mdb_kafka_compute_prod
                conductor_alt_groups:
                    - group_name: mdb_kafka_burst_compute_prod
                      matcher:
                          key: flavor
                          values:
                              - b1.medium
                              - b1.micro
                              - b1.nano
                              - b1.small
                              - b2.medium
                              - b2.micro
                              - b2.nano
                              - b2.small
            greenplum_master:
                conductor_root_group: mdb_greenplum_master_compute_prod
                group_dns:
                    - pattern: 'c-{id}.rw.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.mdb.yandexcloud.net'
                      id: 'cid'
            greenplum_segment:
                conductor_root_group: mdb_greenplum_segment_compute_prod
                group_dns:
                    - pattern: 'c-{id}.rw.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.db.yandexcloud.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.mdb.yandexcloud.net'
                      id: 'cid'
            hadoop:
                running_operations_limit: 10
            conductor:
                token: {{ salt.yav.get('ver-01g63b99a413gkkw54f8mrch45[token]') }}
            deploy:
                token_v2: {{ salt.yav.get('ver-01g365rnm9qk39jj0gsn5gzm9y[token]') }}
            dbm:
                token: {{ salt.yav.get('ver-01g36bbfyvhm6hxcg3chw82tk1[token]') }}
            slayer_dns:
                token: {{ salt.yav.get('ver-01e2g83gr0n446z8xrrgynea33[token]') }}
                ca_path: /opt/yandex/CA.pem
                url: https://dns-api.yandex.net/
            main:
                admin_api_conductor_group: mdb_api_admin_compute_prod
                metadb_hosts:
                    - meta-dbaas01f.yandexcloud.net
                    - meta-dbaas01h.yandexcloud.net
                    - meta-dbaas01k.yandexcloud.net
                api_sec_key: {{ salt.yav.get('ver-01e87rxd7dcvtnrq6rc5zzc1j9[secret]') }}
                client_pub_key: {{ salt.yav.get('ver-01e87rxd7dcvtnrq6rc5zzc1j9[public]') }}
                sentry_dsn: {{ salt.yav.get('ver-01e8v96kjsbpqkjr6a50px652k[dsn]') }}
                sentry_environment: compute-prod
                max_tasks: 200
            s3:
                access_key_id: ''
                secret_access_key: ''
                addressing_style: virtual
                backup:
                    endpoint_url: https://s3.mds.yandex.net
                    access_key_id: {{ salt.yav.get('ver-01e87syb5se464d869gaj6aj5x[id]') }}
                    secret_access_key: {{ salt.yav.get('ver-01e87syb5se464d869gaj6aj5x[secret]') }}
                    allow_lifecycle_policies: true
                cloud_storage:
                    endpoint_url: https://storage.yandexcloud.net
                    access_key_id: {{ salt.yav.get('ver-01ehky0qfnyja6e09v8xsewfzy[access_key]') }}
                    secret_access_key: {{ salt.yav.get('ver-01ehky0qfnyja6e09v8xsewfzy[secret_key]') }}
                    allow_lifecycle_policies: true
                secure_backups:
                    endpoint_url: https://storage.yandexcloud.net
                    access_key_id: {{ salt.yav.get('ver-01fpysnnkdhjz70gcex0xtwsq9[id]') }}
                    secret_access_key: {{ salt.yav.get('ver-01fpysnnkdhjz70gcex0xtwsq9[key]') }}
                    secured: true
                    override_default: true
                    folder_id: yc.mdb.backups
                    iam:
                        service_account_id: yc.mdb.backups-admin
                        key_id: ajeg7dfm5odsk108nffk
                        private_key: {{ salt.yav.get('ver-01fpyrzvmcxtebgy68w8m8hz70[private_key]') | yaml_dquote }}
                idm_endpoint_url: https://storage-idm.private-api.cloud.yandex.net:1443
            juggler:
                token: {{ salt.yav.get('ver-01g366se3vfkezypms658ejhtt[token]') }}
            ssh:
                private_key: /opt/yandex/dbaas-worker/ssh_key
                public_key: ''
            compute:
                url: compute-api.cloud.yandex.net:9051
                managed_network_id: enpuj411rutf542oodr8
                folder_id: b1gdepbkva865gm1nbkq
                ca_path: /opt/yandex/CA.pem
                geo_map: {}
                use_security_group: True
            user_compute:
                service_account_id: ajefj4rf3598dbojbaue
                key_id: ajen9k2h88kisdj5m3a0
                private_key: |
                    {{ salt.yav.get('ver-01es8bccevkf822avnd41zaqcp[private_key]') | indent(20) }}
            vpc:
                url: network-api.private-api.cloud.yandex.net:9823
                ca_path: /opt/yandex/CA.pem
            resource_manager:
                url: https://resource-manager.api.cloud.yandex.net/resource-manager/v1
                ca_path: /etc/ssl/certs
            internal_api:
                url: https://mdb.private-api.cloud.yandex.net/
                ca_path: /opt/yandex/CA.pem
                access_id: 3443b6da-788f-4144-846b-529d4a0449f0
                access_secret: {{ salt.yav.get('ver-01e2g6yyp7t2m3vkxn47f2at08[secret]') }}
            solomon:
                url: https://solomon.cloud.yandex-team.ru
                ca_path: /opt/yandex/CA.pem
                project: yandexcloud
                service: yandexcloud_dbaas
                token: {{ salt.yav.get('ver-01g36bse3n30b4ajwp4e3g46sh[token]') }}
                service_label: yandexcloud_dbaas
            dataproc_manager:
                url: dataproc-manager.private-api.yandexcloud.net:443
                cert_file: "/opt/yandex/CA.pem"
            instance_group_service:
                url: instance-group.private-api.ycp.cloud.yandex.net:443
                grpc_timeout: 30
            loadbalancer_service:
                url: api-adapter.private-api.ycp.cloud.yandex.net:443
                grpc_timeout: 30
            lockbox_service:
                url: lockbox.cloud.yandex.net:8443
                grpc_timeout: 30
            managed_kubernetes_service:
                url: mk8s.private-api.ycp.cloud.yandex.net:443
                grpc_timeout: 30
            managed_postgresql_service:
                url: api-adapter.private-api.ycp.cloud.yandex.net:443
                grpc_timeout: 30
            iam_token_service:
                url: ts.private-api.cloud.yandex.net:4282
            iam_access_service:
                url: iam.private-api.cloud.yandex.net:4283
            iam_service:
                url: iam.private-api.cloud.yandex.net:4283
            iam_jwt:
                url: ts.private-api.cloud.yandex.net:4282
                cert_file: /opt/yandex/allCAs.pem
                server_name: ts.private-api.cloud.yandex.net
                service_account_id: yc.mdb.worker
                key_id: ajejqr3ctri80vsis19k
                private_key: {{ salt.yav.get('ver-01f9c8p9gfd2c0fh5x8rtvyz0v[private_key]') | yaml_dquote }}
            mlock:
                enabled: true
                cert_file: /opt/yandex/allCAs.pem
                server_name: mlock.private-api.yandexcloud.net
                url: mlock.private-api.yandexcloud.net:443
            cloud_storage:
                folder_id: b1gdfigaungc1nqo2e3a
                service_account_id: ajenriq1t6etd365c25t
                key_id: ajesmkr7fqthvh9m1tv0
                private_key: {{ salt.yav.get('ver-01eq0g7kyy2qdd76kygc5246p3[private_key]') | yaml_dquote }}
            per_cluster_service_accounts:
                folder_id: b1gdepbkva865gm1nbkq
