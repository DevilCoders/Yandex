data:
    logship:
        use_rt: false
        topics:
            dbaas_internal_api: /aoe9shbqc2v314v7fp3d/controlplane/dbaas-internal-api-logs
        use_cloud_logbroker: True
        logbroker_host: lb.cc8035oc71oh9um52mv3.ydb.mdb.cloud-preprod.yandex.net
        logbroker_port: 2135
        database: /pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3
        iam_endpoint: iam.api.cloud-preprod.yandex.net
        lb_producer_key: {{ salt.yav.get('ver-01eppc793js8gpv82j4cvqby47[lb_producer_key]') | tojson }}
    dbaas:
        vtype: compute
    common:
        dh: |
            {{ salt.yav.get('ver-01e5w2m26vebrqwptfeqh7d9zs[private]') | indent(12) }}
    internal_api:
        server_name: mdb.private-api.cloud-preprod.yandex.net
        config:
            apispec_swagger_url: null
            apispec_swagger_ui_url: null
            expose_all_task_errors: true
            ctype_config:
                postgresql_cluster: {}
                clickhouse_cluster:
                    zk:
                        flavor: b2.medium
                        volume_size: 10737418240.0
                        node_count: 3
                        disk_type_id: network-ssd
                    shard_count_limit: 50
                mongodb_cluster: {}
                mysql_cluster: {}
            console_address: 'https://console-preprod.cloud.yandex.ru'
            e2e:
                cluster_name: dbaas_e2e_compute_preprod
                folder_id: aoed5i52uquf5jio0oec
            minimal_disk_unit: 4194304
            charts:
                postgresql_cluster:
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/managed-postgresql/cluster/{cid}?section=monitoring']
                clickhouse_cluster:
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/managed-clickhouse/cluster/{cid}?section=monitoring']
                mongodb_cluster:
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/managed-mongodb/cluster/{cid}?section=monitoring']
                mysql_cluster:
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/managed-mysql/cluster/{cid}?section=monitoring']
                redis_cluster:
                    - ['Console', 'Console charts', '{console}/folders/{folderId}/managed-redis/cluster/{cid}?section=monitoring']
            crypto:
                api_secret_key: {{ salt.yav.get('ver-01dw7n4fhvrrbk6wwxgc2d6wbs[private]') }}
                client_public_key: {{ salt.yav.get('ver-01dw7n4fhvrrbk6wwxgc2d6wbs[public]') }}
            generation_names:
                1: Intel Broadwell
                2: Intel Cascade Lake
                3: Intel Ice Lake
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
                        disk_type_id: network-ssd
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
            yc_access_service:
                endpoint: as.private-api.cloud-preprod.yandex.net:4286
                timeout: 2
                ca_path: /opt/yandex/allCAs.pem
            yc_id_prefix: e4u
            identity:
                override_folder: null
                override_cloud: null
                create_missing: True
                allow_move_between_clouds: False
            sentry:
                dsn: {{ salt.yav.get('ver-01dw7rfkd5bpjye3kb030psakk[dsn]') }}
                environment: compute-preprod
            default_cloud_quota:
                clusters_quota: 8
                cpu_quota: 8
                memory_quota: 34359738368.0
                io_quota: 167772160.0
                network_quota: 134217728.0
                ssd_space_quota: 214748364800.0
                hdd_space_quota: 214748364800.0
                gpu_quota: 0.0
            metadb:
                hosts:
                    - meta-dbaas-preprod01f.cloud-preprod.yandex.net
                    - meta-dbaas-preprod01h.cloud-preprod.yandex.net
                    - meta-dbaas-preprod01k.cloud-preprod.yandex.net
                user: dbaas_api
                password: {{ salt.yav.get('ver-01dwew18xwjg8mq1jy8jrea2qg[password]') }}
                dbname: dbaas_metadb
                port: 6432
                maxconn: 30
                minconn: 10
                sslmode: verify-full
                connect_timeout: 1
            env_mapping:
                dev: prestable
                qa: production
            mdbhealth:
                url: https://mdb-health.private-api.cloud-preprod.yandex.net
                connect_timeout: 1
                read_timeout: 5
                ca_certs: /home/web-api/.postgresql/root.crt
            s3:
                endpoint_url: https://s3-private.mds.yandex.net
                aws_access_key_id: {{ salt.yav.get('ver-01dw7pdevntz66n667yrcexg9m[id]') }}
                aws_secret_access_key: {{ salt.yav.get('ver-01dw7pdevntz66n667yrcexg9m[key]') }}
            instance_group_service:
                url: instance-group.private-api.ycp.cloud-preprod.yandex.net:443
                grpc_timeout: 30
                token: ''
            logging_service_config:
                url: logging-cpl.private-api.ycp.cloud-preprod.yandex.net:443
                grpc_timeout: 30
            logging_read_service_config:
                url: reader.logging.cloud-preprod.yandex.net:8443
                grpc_timeout: 30
            compute_quota_service:
                url: compute-api.cloud-preprod.yandex.net:9051
                grpc_timeout: 30
                token: ''
            boto_config:
                s3:
                    addressing_style: virtual
                    region_name: us-east-1
            bucket_prefix: yandexcloud-dbaas-
            external_uri_validation:
                regexp: https://(?:[a-zA-Z0-9-]+\.)?storage\.cloud-preprod\.yandex\.net/\S+
                message: "URI should be a reference to Yandex Object Storage"
            envconfig:
                postgresql_cluster:
                    qa:
                        zk:
                            test:
                              - zk-dbaas-preprod01f.db.yandex.net
                              - zk-dbaas-preprod01h.db.yandex.net
                              - zk-dbaas-preprod01k.db.yandex.net
                    dev:
                        zk:
                            df_e2e:
                              - zk-dbaas-preprod01f.db.yandex.net
                              - zk-dbaas-preprod01h.db.yandex.net
                              - zk-dbaas-preprod01k.db.yandex.net
                mongodb_cluster:
                    qa:
                        zk_hosts:
                            - zk-dbaas-preprod01f.db.yandex.net
                            - zk-dbaas-preprod01h.db.yandex.net
                            - zk-dbaas-preprod01k.db.yandex.net
                    dev:
                        zk_hosts:
                            - zk-dbaas-preprod01f.db.yandex.net
                            - zk-dbaas-preprod01h.db.yandex.net
                            - zk-dbaas-preprod01k.db.yandex.net
                mysql_cluster:
                    qa:
                        zk:
                            test:
                              - zk-dbaas-preprod01f.db.yandex.net
                              - zk-dbaas-preprod01h.db.yandex.net
                              - zk-dbaas-preprod01k.db.yandex.net
                    dev:
                        zk:
                            df_e2e:
                              - zk-dbaas-preprod01f.db.yandex.net
                              - zk-dbaas-preprod01h.db.yandex.net
                              - zk-dbaas-preprod01k.db.yandex.net
                redis_cluster:
                    qa:
                        zk_hosts:
                            - zk-dbaas-preprod01f.db.yandex.net
                            - zk-dbaas-preprod01h.db.yandex.net
                            - zk-dbaas-preprod01k.db.yandex.net
                    dev:
                        zk_hosts:
                            - zk-dbaas-preprod01f.db.yandex.net
                            - zk-dbaas-preprod01h.db.yandex.net
                            - zk-dbaas-preprod01k.db.yandex.net
            versions:
                hadoop_cluster:
                    - version: '1.4'
                    - version: '2.0'
                    - version: '2.1'
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
            default_vtype: compute
            default_disk_type_ids:
                compute: network-ssd
            decommissioning_flavors:
                - s1.nano
                - m1.micro
                - m1.small
                - m1.medium
                - m1.large
                - m1.xlarge
                - m1.2xlarge
                - m1.3xlarge
                - m1.4xlarge
            vtypes:
                compute: mdb.cloud-preprod.yandex.net
            network:
                url: network-api-internal.private-api.cloud-preprod.yandex.net:9823
                timeout: 10
                token: ''
                ca_path: /home/web-api/.postgresql/root.crt
            compute_grpc:
                url: compute-api.cloud-preprod.yandex.net:9051
                token: ''
                ca_path: /opt/yandex/allCAs.pem
            iam_jwt_config:
                url: ts.private-api.cloud-preprod.yandex.net:4282
                cert_file: "/opt/yandex/allCAs.pem"
                server_name: ts.private-api.cloud-preprod.yandex.net
                service_account_id: {{ salt.yav.get('ver-01ew0anmtq0y181dn3yesqky30[id]') }}
                key_id: {{ salt.yav.get('ver-01ew0anmtq0y181dn3yesqky30[key_id]') }}
                private_key: {{ salt.yav.get('ver-01ew0anmtq0y181dn3yesqky30[private_key]') | yaml_encode }}
                insecure: False
                audience: https://iam.api.cloud.yandex.net/iam/v1/tokens
                expire_thresh: 180
                request_expire: 3600
            hadoop_default_resources:
                resource_preset_id: 's2.micro'
                disk_size: 21474836480
                disk_type_id: 'network-ssd'
            decommission_timeout:
                min: 0
                max: 86400
            public_images_folder: aoen0gulfkoaicqsvt1i
            hadoop_default_version_prefix: '2.0'
            hadoop_images:
                '1.4.0':
                    name: '1.4.0'
                    version: '1.4.0'
                    imageMinSize: 21474836480
                    supported_pillar: stable-1-0
                    services:
                        hdfs:
                            version: '2.10.0'
                            default: true
                        yarn:
                            version: '2.10.0'
                            deps: ['hdfs']
                            default: true
                        mapreduce:
                            version: '2.10.0'
                            deps: ['yarn']
                            default: true
                        tez:
                            version: '0.9.2'
                            deps: ['yarn']
                            default: true
                        zookeeper:
                            version: '3.4.14'
                            default: false
                        hbase:
                            version: '1.3.5'
                            deps: ['zookeeper', 'hdfs', 'yarn']
                            default: false
                        hive:
                            version: '2.3.6'
                            deps: ['yarn', 'mapreduce']
                            default: false
                        sqoop:
                            version: '1.4.7'
                            default: false
                        flume:
                            version: '1.9.0'
                            default: false
                        spark:
                            version: '2.4.6'
                            deps: ['yarn', 'hdfs']
                            default: true
                        zeppelin:
                            version: '0.8.2'
                            default: false
                        oozie:
                            version: '5.2.0'
                            default: false
                        livy:
                            version: '0.7.0'
                            deps: ['spark']
                            default: false
                    available_services: ['hdfs', 'yarn', 'mapreduce', 'tez', 'zookeeper', 'hbase', 'hive', 'sqoop', 'flume', 'spark', 'zeppelin', 'oozie', 'livy']
                    service_deps:
                        mapreduce: ['yarn']
                        hive: ['yarn']
                        tez: ['yarn']
                        spark: ['yarn']
                        hbase: ['zookeeper', 'hdfs', 'yarn']
                        oozie: ['zookeeper']
                        livy: ['spark']
                    roles_services:
                        'hadoop_cluster.masternode': ['mapreduce', 'zookeeper', 'hive', 'sqoop', 'spark', 'zeppelin', 'oozie', 'hbase', 'hdfs', 'yarn', 'livy']
                        'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark', 'flume']
                        'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume']
                '2.0.0':
                    name: '2.0.0'
                    version: '2.0.0'
                    imageMinSize: 21474836480
                    supported_pillar: stable-1-0
                    services:
                        hdfs:
                            version: '3.2.2'
                            default: true
                        yarn:
                            version: '3.2.2'
                            default: true
                        mapreduce:
                            version: '3.2.2'
                            deps: ['yarn', 'hdfs']
                            default: true
                        tez:
                            version: '0.10.0'
                            deps: ['yarn']
                            default: true
                        zookeeper:
                            version: '3.4.14'
                            default: false
                        hbase:
                            version: '2.2.7'
                            deps: ['zookeeper', 'hdfs', 'yarn']
                            default: false
                        hive:
                            version: '3.1.2'
                            deps: ['yarn', 'mapreduce']
                            default: false
                        spark:
                            version: '3.0.2'
                            deps: ['yarn']
                            default: true
                        zeppelin:
                            version: '0.9.0'
                            default: false
                        livy:
                            version: '0.8.0'
                            deps: ['spark']
                            default: false
                        oozie:
                            version: '5.2.1'
                            deps: ['hdfs']
                            default: false
                    available_services: ['hdfs', 'yarn', 'mapreduce', 'tez', 'zookeeper', 'hbase', 'hive', 'spark', 'zeppelin', 'oozie', 'livy']
                    service_deps:
                        mapreduce: ['yarn']
                        hive: ['yarn']
                        tez: ['yarn']
                        spark: ['yarn']
                        hbase: ['zookeeper', 'hdfs', 'yarn']
                        oozie: ['hdfs']
                        livy: ['spark']
                    roles_services:
                        'hadoop_cluster.masternode': ['mapreduce', 'zookeeper', 'hive', 'spark', 'hbase', 'hdfs', 'yarn', 'zeppelin', 'livy', 'oozie']
                        'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark']
                        'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez']
                '2.1.0':
                    name: '2.1.0'
                    version: '2.1.0'
                    imageMinSize: 21474836480
                    supported_pillar: stable-1-0
                    services:
                        hdfs:
                            version: '3.3.2'
                            default: true
                        yarn:
                            version: '3.3.2'
                            default: true
                        mapreduce:
                            version: '3.3.2'
                            deps: ['yarn', 'hdfs']
                            default: false
                        tez:
                            version: '0.10.1'
                            deps: ['yarn']
                            default: true
                        zookeeper:
                            version: '3.5.9'
                            default: false
                        hbase:
                            version: '2.4.9'
                            deps: ['zookeeper', 'hdfs', 'yarn']
                            default: false
                        spark:
                            version: '3.2.1'
                            deps: ['yarn']
                            default: true
                        zeppelin:
                            version: '0.10.0'
                            default: false
                        livy:
                            version: '0.9.0'
                            deps: ['spark']
                            default: false
                    available_services: ['hdfs', 'yarn', 'mapreduce', 'tez', 'zookeeper', 'hbase', 'spark', 'zeppelin', 'livy']
                    service_deps:
                        mapreduce: ['yarn']
                        tez: ['yarn']
                        spark: ['yarn']
                        hbase: ['zookeeper', 'hdfs', 'yarn']
                        livy: ['spark']
                    roles_services:
                        'hadoop_cluster.masternode': ['mapreduce', 'zookeeper', 'spark', 'hbase', 'hdfs', 'yarn', 'zeppelin', 'livy']
                        'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark']
                        'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez']
            hadoop_pillars:
                stable-1-0:
                    data:
                        unmanaged:
                            services: ['hdfs', 'yarn', 'mapreduce', 'tez', 'zookeeper', 'hbase', 'hive', 'sqoop', 'flume', 'spark', 'zeppelin', 'oozie']
                            topology:
                                subclusters: {}
                            s3:
                                endpoint_url: storage.cloud-preprod.yandex.net
                                region_name: ru-central1
                            agent:
                                manager_url: dataproc-manager.api.cloud-preprod.yandex.net:443
                                cert_server_name: dataproc-manager.api.cloud-preprod.yandex.net
                                ui_proxy_url: https://dataproc-ui.cloud-preprod.yandex.net/
                            monitoring:
                                hostname: monitoring.api.cloud-preprod.yandex.net
                            logging:
                                url: api.cloud-preprod.yandex.net:443
                                ingester_url: ingester.logging.cloud-preprod.yandex.net:443
            hadoop_ui_links:
                base_url: https://cluster-${CLUSTER_ID}.dataproc-ui.cloud-preprod.yandex.net/gateway/default-topology/
                knoxless_base_url: https://ui-${CLUSTER_ID}-${HOST}-${PORT}.dataproc-ui.cloud-preprod.yandex.net/
                services:
                    hdfs:
                        - code: hdfs
                          name: HDFS Namenode UI
                          port: 9870
                    yarn:
                        - code: yarn
                          name: YARN Resource Manager Web UI
                          port: 8088
                        - code: jobhistory
                          name: JobHistory Server Web UI
                          port: 8188
                    hive:
                        - code: hive
                          name: Hive
                          port: 10002
                    spark:
                        - code: sparkhistory
                          name: Spark History Server Web UI
                          port: 18080
                    zeppelin:
                        - code: zeppelin
                          name: Zeppelin Web UI
                          port: 8890
                    oozie:
                        - code: oozieui
                          name: Oozie Web Console
                          port: 11000
                    livy:
                        - code: livy
                          name: Livy Server
                          port: 8998
            dns_cache:
                "storage.cloud-preprod.yandex.net": 213.180.205.156
                "dataproc-manager.api.cloud-preprod.yandex.net": 130.193.32.122
                "monitoring.api.cloud-preprod.yandex.net": 87.250.250.10
                "dataproc-ui.cloud-preprod.yandex.net": 130.193.32.122
            resource_manager_config:
                url: https://resource-manager.api.cloud-preprod.yandex.net/resource-manager/v1
                ca_path: /etc/ssl/certs
                token: ''
            yc_identity:
                base_url: https://identity.private-api.cloud-preprod.yandex.net:14336
                connect_timeout: 1
                read_timeout: 10
                token: ''
                ca_path: /home/web-api/.postgresql/root.crt
            resource_manager_config_grpc:
                url: rm.private-api.cloud-preprod.yandex.net:4284
                cert_file: /opt/yandex/allCAs.pem
            dataproc_manager_config:
                url: dataproc-manager.private-api.cloud-preprod.yandex.net:443
                private_url: dataproc-manager.private-api.cloud-preprod.yandex.net:443
                cert_file: "/opt/yandex/allCAs.pem"
            dataproc_manager_public_config:
                manager_url: dataproc-manager.api.cloud-preprod.yandex.net:443
            dataproc_job_log:
                endpoint: storage.cloud-preprod.yandex.net
                region: ru-central1
            iam_token_config:
                url: ts.private-api.cloud-preprod.yandex.net:4282
                ca_path: /home/web-api/.postgresql/root.crt
                token: ''
            default_backup_schedule:
                clickhouse_cluster:
                    start:
                        hours: 22
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
                    sleep: 1800
                    retain_period: 7
                redis_cluster:
                    start:
                        hours: 22
                        minutes: 0
                        seconds: 0
                        nanos: 0
                    sleep: 1800
                postgresql_cluster:
                    start:
                        hours: 22
                        minutes: 0
                        seconds: 0
                        nanos: 0
                    sleep: 1800
                mysql_cluster:
                    start:
                        hours: 22
                        minutes: 0
                        seconds: 0
                        nanos: 0
                    sleep: 1800
        tls_key: |
            {{ salt.yav.get('ver-01dw7s65c8ksjrdanb3a6wtg0q[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01dw7s65c8ksjrdanb3a6wtg0q[cert]') | indent(12) }}
