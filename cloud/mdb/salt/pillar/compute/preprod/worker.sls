data:
    dbaas_worker:
        ssh_private_key: |
            {{ salt.yav.get('ver-01dw7fcc2rxwfz6sj84hx9yppy[private]') | indent(12) }}
        config:
            cert_api:
                token: ''
                ca_name: YcInternalCA
                cert_type: yc-server
                url: https://mdb-secrets.private-api.cloud-preprod.yandex.net
                api: MDB_SECRETS
                ca_path: /opt/yandex/allCAs.pem
            postgresql:
                conductor_root_group: mdb_postgresql_compute_preprod
                group_dns:
                    - pattern: 'c-{id}.rw.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.mdb.cloud-preprod.yandex.net'
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
            clickhouse:
                conductor_root_group: mdb_clickhouse_compute_preprod
                group_dns:
                    - pattern: 'c-{id}.rw.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: '{id}.c-{cid}.rw.db.cloud-preprod.yandex.net'
                      id: 'shard_name'
                    - pattern: '{id}.c-{cid}.rw.mdb.cloud-preprod.yandex.net'
                      id: 'shard_name'
                juggler_checks:
                    - META
                    - ch_ping
            mysql:
                conductor_root_group: mdb_mysql_compute_preprod
                group_dns:
                    - pattern: 'c-{id}.rw.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
            sqlserver:
                conductor_root_group: mdb_sqlserver_compute_preprod
                group_dns:
                    - pattern: 'c-{id}.rw.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
                compute_image_type_template:
                    template: sqlserver-{major_version}
                    task_arg: major_version
                    whitelist:
                        '2016sp2dev': '2016sp2dev'
                        '2016sp2std': '2016sp2std'
                        '2016sp2ent': '2016sp2ent'
                        '2017dev':    '2017dev'
                        '2017std':    '2017std'
                        '2017ent':    '2017ent'
                        '2019dev':    '2019dev'
                        '2019std':    '2019std'
                        '2019ent':    '2019ent'
            windows_witness:
                conductor_root_group: mdb_sqlserver_witness_compute_preprod
            redis:
                conductor_root_group: mdb_redis_compute_preprod
                group_dns:
                    - pattern: 'c-{id}.rw.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.cloud-preprod.yandex.net'
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
                conductor_root_group: mdb_mongodb_compute_preprod
                compute_recreate_on_resize: true
            mongos:
                conductor_root_group: mdb_mongos_compute_preprod
            mongocfg:
                conductor_root_group: mdb_mongocfg_compute_preprod
                compute_recreate_on_resize: true
            mongoinfra:
                conductor_root_group: mdb_mongoinfra_compute_preprod
                compute_recreate_on_resize: true
            zookeeper:
                conductor_root_group: mdb_zookeeper_compute_preprod
            kafka:
                group_dns:
                    - pattern: 'c-{id}.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
                conductor_root_group: mdb_kafka_compute_preprod
            greenplum_master:
                conductor_root_group: mdb_greenplum_master_compute_preprod
                group_dns:
                    - pattern: 'c-{id}.rw.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
            greenplum_segment:
                conductor_root_group: mdb_greenplum_segment_compute_preprod
                group_dns:
                    - pattern: 'c-{id}.rw.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.ro.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
            elasticsearch_data:
                conductor_root_group: mdb_elasticsearch_data_compute_preprod
                group_dns:
                    - pattern: 'c-{id}.rw.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.cloud-preprod.yandex.net'
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
                conductor_root_group: mdb_elasticsearch_master_compute_preprod
                group_dns:
                    - pattern: 'c-{id}.rw.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.cloud-preprod.yandex.net'
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
                conductor_root_group: mdb_opensearch_data_compute_preprod
                group_dns:
                    - pattern: 'c-{id}.rw.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
                compute_image_type_template:
                    template: opensearch-{major_version}
                    task_arg: major_version
                    whitelist:
                        '21': '21'
            opensearch_master:
                conductor_root_group: mdb_opensearch_master_compute_preprod
                group_dns:
                    - pattern: 'c-{id}.rw.db.cloud-preprod.yandex.net'
                      id: 'cid'
                    - pattern: 'c-{id}.rw.mdb.cloud-preprod.yandex.net'
                      id: 'cid'
                compute_image_type_template:
                    template: opensearch-{major_version}
                    task_arg: major_version
                    whitelist:
                        '21': '21'
            hadoop:
                running_operations_limit: 10
            conductor:
                token: {{ salt.yav.get('ver-01g63b99a413gkkw54f8mrch45[token]') }}
            deploy:
                token_v2: {{ salt.yav.get('ver-01g365rfjp48rh3kdm5ayxhpwd[token]') }}
            dbm:
                token: {{ salt.yav.get('ver-01g36bbmxvwtqvcxbh0rnw5j19[token]') }}
            slayer_dns:
                token: {{ salt.yav.get('ver-01dw7mpmcc77axtrf8pm9a5kzq[token2]') }}
                ca_path: /opt/yandex/CA.pem
                url: https://dns-api.yandex.net/
            main:
                admin_api_conductor_group: mdb_api_admin_compute_preprod
                metadb_hosts:
                    - meta-dbaas-preprod01f.cloud-preprod.yandex.net
                    - meta-dbaas-preprod01h.cloud-preprod.yandex.net
                    - meta-dbaas-preprod01k.cloud-preprod.yandex.net
                api_sec_key: {{ salt.yav.get('ver-01dw7n4fhvrrbk6wwxgc2d6wbs[private]') }}
                client_pub_key: {{ salt.yav.get('ver-01dw7n4fhvrrbk6wwxgc2d6wbs[public]') }}
                sentry_dsn: {{ salt.yav.get('ver-01dw7r0n8f3fdtsfcr7bttnb9d[dsn]') }}
                sentry_environment: compute-preprod
            s3:
                access_key_id: ''
                secret_access_key: ''
                addressing_style: virtual
                backup:
                    endpoint_url: https://s3.mds.yandex.net
                    access_key_id: {{ salt.yav.get('ver-01dw7pdevntz66n667yrcexg9m[id]') }}
                    secret_access_key: {{ salt.yav.get('ver-01dw7pdevntz66n667yrcexg9m[key]') }}
                    allow_lifecycle_policies: true
                cloud_storage:
                    endpoint_url: https://storage.cloud-preprod.yandex.net
                    access_key_id: {{ salt.yav.get('ver-01ehkxx4jvghgy1f99be9xkqhr[access_key]') }}
                    secret_access_key: {{ salt.yav.get('ver-01ehkxx4jvghgy1f99be9xkqhr[secret_key]') }}
                    allow_lifecycle_policies: true
                secure_backups:
                    endpoint_url: https://storage.cloud-preprod.yandex.net
                    access_key_id: {{ salt.yav.get('ver-01fpthttms47re0tb3gwwvbbpw[id]') }}
                    secret_access_key: {{ salt.yav.get('ver-01fpthttms47re0tb3gwwvbbpw[key]') }}
                    secured: true
                    override_default: true
                    folder_id: yc.mdb.backups
                    iam:
                        service_account_id: yc.mdb.backups-admin
                        key_id: bfbutc132c4ug5kd7r0g
                        private_key: {{ salt.yav.get('ver-01fpthrqcwarjhagvdrqbrbzy2[private_key]') | yaml_dquote }}
                idm_endpoint_url: https://storage-idm.private-api.cloud-preprod.yandex.net:1443
            juggler:
                token: {{ salt.yav.get('ver-01g366sb0w6hdg76gh40f5sm3x[token]') }}
            ssh:
                private_key: /opt/yandex/dbaas-worker/ssh_key
                public_key: 'ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAGP1Nxazgt9k+5i70Iae/7KHNc5SY8v+FI9oj+eSZ2qqEj+mCQqNYFGGC7MhINW9hlpvxeb+lXMUz7Vp4CJRx2cgAApMAo7cnvIGBCm0z5dv1+oJgqBr7qOETghpVnUVHkKOWPeOy9EHJAi00THSAug0nbN9QPePI2fpiTUqxwKtoiEbQ== robot-pgaas-deploy'
            compute:
                url: compute-api.cloud-preprod.yandex.net:9051
                managed_network_id: c64nbdvblhel8i3mh89j
                folder_id: aoed5i52uquf5jio0oec
                ca_path: /opt/yandex/CA.pem
                geo_map: {}
                placement_group: {
                    best_effort: True
                }
            vpc:
                url: network-api-internal.private-api.cloud-preprod.yandex.net:9823
                ca_path: /opt/yandex/CA.pem
            user_compute:
                service_account_id: yc.mdb.worker
                key_id: bfbe44bcprfank3dmt5b
                private_key: {{ salt.yav.get('ver-01f87p103s9qjcayv9gy070pxt[private_key]') | yaml_encode }}
            internal_api:
                url: https://mdb.private-api.cloud-preprod.yandex.net/
                ca_path: /opt/yandex/CA.pem
                access_id: {{ salt.yav.get('ver-01dw7qq43p84ss4hwpwcz77d1d[id]') }}
                access_secret: {{ salt.yav.get('ver-01dw7qq43p84ss4hwpwcz77d1d[secret]') }}
            iam_token_service:
                url: ts.private-api.cloud-preprod.yandex.net:4282
            iam_access_service:
                url: iam.private-api.cloud-preprod.yandex.net:4283
            iam_service:
                url: iam.private-api.cloud-preprod.yandex.net:4283
            iam_jwt:
                url: ts.private-api.cloud-preprod.yandex.net:4282
                cert_file: /opt/yandex/allCAs.pem
                server_name: ts.private-api.cloud-preprod.yandex.net
                service_account_id: yc.mdb.worker
                key_id: bfbe44bcprfank3dmt5b
                private_key: {{ salt.yav.get('ver-01f87p103s9qjcayv9gy070pxt[private_key]') | yaml_encode }}
            mlock:
                enabled: true
                cert_file: /opt/yandex/allCAs.pem
                server_name: mlock.private-api.cloud-preprod.yandex.net
                url: mlock.private-api.cloud-preprod.yandex.net:443
            solomon:
                url: https://solomon.cloud.yandex-team.ru
                ca_path: /opt/yandex/CA.pem
                project: yandexcloud
                service: yandexcloud_dbaas
                token: {{ salt.yav.get('ver-01g36bs9pgfgs31hf35pa0de9f[token]') }}
                service_label: yandexcloud_dbaas
            resource_manager:
                url: https://resource-manager.api.cloud-preprod.yandex.net/resource-manager/v1
                ca_path: /etc/ssl/certs
            dataproc_manager:
                url: dataproc-manager.private-api.cloud-preprod.yandex.net:443
                cert_file: "/opt/yandex/CA.pem"
            instance_group_service:
                url: instance-group.private-api.ycp.cloud-preprod.yandex.net:443
                grpc_timeout: 30
            loadbalancer_service:
                url: api-adapter.private-api.ycp.cloud-preprod.yandex.net:443
                grpc_timeout: 30
            lockbox_service:
                url: lockbox.cloud-preprod.yandex.net:8443
                grpc_timeout: 30
            managed_kubernetes_service:
                url: mk8s.private-api.ycp.cloud-preprod.yandex.net:443
                grpc_timeout: 30
            managed_postgresql_service:
                url: api-adapter.private-api.ycp.cloud-preprod.yandex.net:443
                grpc_timeout: 30
            cloud_storage:
                folder_id: aoe5la57iqss43dqic0d
                service_account_id: bfbvl80ombu8rh97u0qv
                key_id: bfbi105vhlvjvctfmp44
                private_key: {{ salt.yav.get('ver-01eq0g92jbymn13jn0fqzrmqf2[private_key]') | yaml_dquote }}
            per_cluster_service_accounts:
                folder_id: aoed5i52uquf5jio0oec
