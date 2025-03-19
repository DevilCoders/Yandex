data:
  dbaas_metadb:
    cluster_type_pillars:
      - type: clickhouse_cluster
        value:
          data:
            salt_version: 3002.7+ds-1+yandex0
            salt_py_version: 3
            allow_salt_version_update: False
            access:
              user_hosts:
              - fqdn: s3-private.mds.yandex.net
                port: 443
              - fqdn: s3.mds.yandex.net
                port: 80
              - fqdn: storage.cloud-preprod.yandex.net
                port: 443
              management_hosts:
              - fqdn: s3-private.mds.yandex.net
                port: 443
              - fqdn: s3.mds.yandex.net
                port: 80
              - fqdn: storage.yandexcloud.net
                port: 443
              - fqdn: storage.yandexcloud.net
                port: 80
              - fqdn: storage.cloud-preprod.yandex.net
                port: 443
            logship:
              topics:
                clickhouse_logs: aoe9shbqc2v314v7fp3d/dataplane/clickhouse-logs
            backup:
              start:
                hours: 22
                minutes: 0
                seconds: 0
                nanos: 0
              sleep: 1800
            clickhouse:
              config:
                log_level: debug
                builtin_dictionaries_reload_interval: 3600
                max_connections: 4096
                max_concurrent_queries: 500
                keep_alive_timeout: 3
                uncompressed_cache_size: 8589934592
                mark_cache_size: 5368709120
                max_table_size_to_drop: 53687091200
                max_partition_size_to_drop: 53687091200
                timezone: Europe/Moscow
                query_log_retention_size: 1073741824
                query_log_retention_time: 2592000
                query_thread_log_enabled: true
                query_thread_log_retention_size: 536870912
                query_thread_log_retention_time: 2592000
                part_log_retention_size: 536870912
                part_log_retention_time: 2592000
                metric_log_enabled: true
                metric_log_retention_size: 536870912
                metric_log_retention_time: 2592000
                trace_log_enabled: true
                trace_log_retention_size: 536870912
                trace_log_retention_time: 2592000
                text_log_enabled: false
                text_log_retention_size: 536870912
                text_log_retention_time: 2592000
                text_log_level: trace
                compression: []
                merge_tree:
                  replicated_deduplication_window: 100
                  replicated_deduplication_window_seconds: 604800
                  parts_to_delay_insert: 150
                  parts_to_throw_insert: 300
                  max_replicated_merges_in_queue: 16
                  number_of_free_entries_in_pool_to_lower_max_size_of_merge: 8
                  max_bytes_to_merge_at_min_space_in_pool: 1048576
                  enable_mixed_granularity_parts: true
            oslogin:
              enabled: true
              breakglass:
                ca_public_keys:
                  - ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC/kScoku920oGZ8vGPy9aNV+aQtihyzyEZy40Gbr9c18fGFh9Nc8SdFba/e0JKH8eOJq7TLB4mcxM2GybdPVFzoW+SkNJZEgN++YHMED1PmHMPCDB9JmijJOIiOvnkOrKIIgn7/6g841qBCXL+hn+VySHPLSooAlVJnZXSdads6nv9kXZeyU4rHzOT7BkkK4Bcbay+04srmvShno87KB47hYH0Gb6nJH8VHY7RBuqhdyDzMQ2gq1n3wYF6aLgHphboP01LTsonjIVoBJhYT+Eyie95bsg5gt7XcQP02iBvmGh/7CtkG18QpABv8GLFF5R3YL/B4LG7AMyyOTyZc5Wz yubikey
                  - ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAFxDHbZHZCx72sztvWsFSJVUhW+fngANgR5iiIWy9gicOdjYOy0pxpb1U8PqWmJYJYOWLwtfm/f9qnSV3Q/wAX/igCAwdYuwsWtx/0eBp2G3ECIUJzEQRme+TrtECpJHyLFCJonE/Cg24JlQ8N6mVjrQZwLiVjCdVwRFGoXxb3F1N4SHg== bastion
                  - ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIF6dz6foYXUqqxfqJpI/ZHabt1OiMINrrLD/LKyZJ5N3 bastion
                authorized_principals:
                  - alex-burmak
                  - iantonspb
                  - vadim-volodin
                  - pervakovg
                  - dstaroff
                  - estrizhnev
                  - moukavoztchik
                  - earhipov
                  - d0uble
                  - egor-medvedev
          firewall:
            ACCEPT:
            - int: eth1
              net: ::/0
              ports: ['22']
              type: addr6
            - int: eth0
              net: ::/0
              ports_function: mdb_clickhouse.open_ports
              type: addr6
            - int: eth0
              net: 0.0.0.0/0
              ports_function: mdb_clickhouse.open_ports
              type: addr4
            - int: eth0
              net: data:dbaas:cluster_hosts
              ports: ['2181', '2281', '2888', '3888', '9009', '9010']
              type: pillar_fqdn
            policy: DROP
            ports:
              external:
                - 8443
                - 9440
          testing_repos: true
      - type: mongodb_cluster
        value:
          data:
            logship:
              topics:
                mongodb_logs: aoe9shbqc2v314v7fp3d/dataplane/mongodb-logs
                mongodb_audit_logs: aoe9shbqc2v314v7fp3d/dataplane/mongodb-audit-logs
            perf_diag:
              use_cloud_logbroker: true
              topics:
                mongodb-profiler: aoe9shbqc2v314v7fp3d/dataplane/perf_diag/mongodb_profiler
              logbroker_port: 2135
              logbroker_host: lb.cc8035oc71oh9um52mv3.ydb.mdb.cloud-preprod.yandex.net
              database: /pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3
              iam_endpoint: iam.api.cloud-preprod.yandex.net
              lb_producer_key: {{ salt.yav.get('ver-01ewg0wgrbers3c7gz4vmf307w[lb_producer_key]') | tojson }}
            backup:
              start:
                hours: 22
                minutes: 0
                seconds: 0
                nanos: 0
              sleep: 1800
            mongodb:
              config:
                mongocfg:
                  net:
                    maxIncomingConnections: 1024
                  operationProfiling:
                    mode: slowOp
                    slowOpThresholdMs: 300
                mongod:
                  net:
                    maxIncomingConnections: 1024
                  operationProfiling:
                    mode: slowOp
                    slowOpThresholdMs: 300
                  storage:
                    journal:
                      commitInterval: 100
                      enabled: true
                    wiredTiger:
                      collectionConfig:
                        blockCompressor: snappy
                mongos:
                  net:
                    maxIncomingConnections: 1024
            walg:
              install: true
              enabled: true
            oslogin:
              enabled: true
              breakglass:
                ca_public_keys:
                  - ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC/kScoku920oGZ8vGPy9aNV+aQtihyzyEZy40Gbr9c18fGFh9Nc8SdFba/e0JKH8eOJq7TLB4mcxM2GybdPVFzoW+SkNJZEgN++YHMED1PmHMPCDB9JmijJOIiOvnkOrKIIgn7/6g841qBCXL+hn+VySHPLSooAlVJnZXSdads6nv9kXZeyU4rHzOT7BkkK4Bcbay+04srmvShno87KB47hYH0Gb6nJH8VHY7RBuqhdyDzMQ2gq1n3wYF6aLgHphboP01LTsonjIVoBJhYT+Eyie95bsg5gt7XcQP02iBvmGh/7CtkG18QpABv8GLFF5R3YL/B4LG7AMyyOTyZc5Wz yubikey
                  - ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAFxDHbZHZCx72sztvWsFSJVUhW+fngANgR5iiIWy9gicOdjYOy0pxpb1U8PqWmJYJYOWLwtfm/f9qnSV3Q/wAX/igCAwdYuwsWtx/0eBp2G3ECIUJzEQRme+TrtECpJHyLFCJonE/Cg24JlQ8N6mVjrQZwLiVjCdVwRFGoXxb3F1N4SHg== bastion
                  - ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIF6dz6foYXUqqxfqJpI/ZHabt1OiMINrrLD/LKyZJ5N3 bastion
                authorized_principals:
                  - vgoshev
                  - roman-chulkov
                  - dkhurtin
                  - ayasensky
                  - arhipov
                  - d0uble
            mdb_metrics:
              enabled: true
              enable_userfault_broken_collector: false
              main:
                yasm_tags_cmd: /usr/local/yasmagent/mdb_mongodb_getter.py
                yasm_tags_db: mongodb
          firewall:
            ACCEPT:
            - int: eth1
              net: ::/0
              ports: ['22']
              type: addr6
            - int: eth0
              net: ::/0
              ports: ['27017', '27018']
              type: addr6
            - int: eth0
              net: 0.0.0.0/0
              ports: ['27017', '27018']
              type: addr4
            - int: eth0
              net: data:dbaas:cluster_hosts
              ports: ['27019']
              type: pillar_fqdn
            policy: DROP
            ports:
              external:
                - 27018
      - type: redis_cluster
        value:
          data:
            logship:
              topics:
                redis_logs: aoe9shbqc2v314v7fp3d/dataplane/redis-logs
            backup:
              start:
                hours: 22
                minutes: 0
                seconds: 0
                nanos: 0
              sleep: 1800
            redis:
              config:
                maxmemory-policy: noeviction
                notify-keyspace-events: '""'
                slowlog-log-slower-than: 10000
                slowlog-max-len: 1000
                databases: 16
                timeout: 0
            walg:
              enabled: true
              install: true
            oslogin:
              enabled: true
              breakglass:
                ca_public_keys:
                  - ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC/kScoku920oGZ8vGPy9aNV+aQtihyzyEZy40Gbr9c18fGFh9Nc8SdFba/e0JKH8eOJq7TLB4mcxM2GybdPVFzoW+SkNJZEgN++YHMED1PmHMPCDB9JmijJOIiOvnkOrKIIgn7/6g841qBCXL+hn+VySHPLSooAlVJnZXSdads6nv9kXZeyU4rHzOT7BkkK4Bcbay+04srmvShno87KB47hYH0Gb6nJH8VHY7RBuqhdyDzMQ2gq1n3wYF6aLgHphboP01LTsonjIVoBJhYT+Eyie95bsg5gt7XcQP02iBvmGh/7CtkG18QpABv8GLFF5R3YL/B4LG7AMyyOTyZc5Wz yubikey
                  - ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAFxDHbZHZCx72sztvWsFSJVUhW+fngANgR5iiIWy9gicOdjYOy0pxpb1U8PqWmJYJYOWLwtfm/f9qnSV3Q/wAX/igCAwdYuwsWtx/0eBp2G3ECIUJzEQRme+TrtECpJHyLFCJonE/Cg24JlQ8N6mVjrQZwLiVjCdVwRFGoXxb3F1N4SHg== bastion
                  - ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIF6dz6foYXUqqxfqJpI/ZHabt1OiMINrrLD/LKyZJ5N3 bastion
                authorized_principals:
                  - vgoshev
                  - pperekalov
                  - ayasensky
                  - litleleprikon
                  - arhipov
                  - d0uble
            mdb_metrics:
              enable_userfault_broken_collector: false
              main:
                yasm_tags_cmd: /usr/local/yasmagent/redis_getter.py
                yasm_tags_db: redis
          firewall:
            ACCEPT:
            - int: eth1
              net: ::/0
              ports: ['22']
              type: addr6
            - int: eth0
              net: ::/0
              ports: ['6379', '16379', '26379', '6380', '16380', '26380']
              type: addr6
            - int: eth0
              net: 0.0.0.0/0
              ports: ['6379', '16379', '26379', '6380', '16380', '26380']
              type: addr4
            policy: DROP
            ports:
              external:
                - 6379
                - 6380
                - 26379
                - 26380
      - type: postgresql_cluster
        value:
          data:
            odyssey:
              user: postgres
            logship:
              topics:
                postgresql_logs: aoe9shbqc2v314v7fp3d/dataplane/postgresql-logs
                pgbouncer_logs: aoe9shbqc2v314v7fp3d/dataplane/pgbouncer-logs
                odyssey_logs: aoe9shbqc2v314v7fp3d/dataplane/odyssey-logs
            perf_diag:
              tvm_secret:
                data: {{ salt.yav.get('ver-01eezf4seyemw1eqv9vmy6rd5x[secret]') }}
                encryption_version: 1
              topics:
                pg_stat_activity: mdb/compute/preprod/perf_diag/pg_stat_activity
                pg_stat_statements: mdb/compute/preprod/perf_diag/pg_stat_statements
                pg_stat_statements_query: mdb/compute/preprod/perf_diag/pg_stat_statements_query
                grpc_pg_stat_activity: aoe9shbqc2v314v7fp3d/dataplane_rt/perf_diag/pg_stat_activity
                grpc_pg_stat_statements: aoe9shbqc2v314v7fp3d/dataplane_rt/perf_diag/pg_stat_statements
              tvm_client_id: 2021712
              tvm_server_id: 2001059
            backup:
              start:
                hours: 22
                minutes: 0
                seconds: 0
                nanos: 0
              sleep: 1800
            config:
              archive_timeout: 30s
              array_nulls: true
              auto_explain_log_analyze: false
              auto_explain_log_buffers: false
              auto_explain_log_min_duration: -1
              auto_explain_log_nested_statements: false
              auto_explain_log_timing: false
              auto_explain_log_triggers: false
              auto_explain_log_verbose: false
              auto_explain_sample_rate: 1
              auto_kill_timeout: 12 hours
              autovacuum_analyze_scale_factor: 0.0001
              autovacuum_naptime: 15s
              autovacuum_vacuum_scale_factor: 0.00001
              autovacuum_work_mem: -1
              backend_flush_after: 0
              backslash_quote: safe_encoding
              bgwriter_delay: 200ms
              bgwriter_flush_after: 512kB
              bgwriter_lru_maxpages: 100
              bgwriter_lru_multiplier: 2.0
              bytea_output: hex
              checkpoint_completion_target: 0.5
              checkpoint_flush_after: 256kB
              checkpoint_timeout: 5min
              client_min_messages: notice
              constraint_exclusion: partition
              cursor_tuple_fraction: 0.1
              deadlock_timeout: 1s
              default_statistics_target: 1000
              default_transaction_isolation: read committed
              default_transaction_read_only: false
              default_with_oids: false
              effective_cache_size: "100GB"
              effective_io_concurrency: 1
              enable_bitmapscan: true
              enable_hashagg: true
              enable_hashjoin: true
              enable_indexonlyscan: true
              enable_indexscan: true
              enable_material: true
              enable_mergejoin: true
              enable_nestloop: true
              enable_parallel_append: true
              enable_parallel_hash: true
              enable_partition_pruning: true
              enable_partitionwise_aggregate: false
              enable_partitionwise_join: false
              enable_seqscan: true
              enable_sort: true
              enable_tidscan: true
              escape_string_warning: true
              exit_on_error: false
              force_parallel_mode: 'off'
              from_collapse_limit: 8
              gin_pending_list_limit: 4MB
              idle_in_transaction_session_timeout: 0
              join_collapse_limit: 8
              lo_compat_privileges: false
              lock_timeout: 0
              log_checkpoints: false
              log_connections: false
              log_disconnections: false
              log_duration: false
              log_error_verbosity: default
              log_keep_days: 1
              log_lock_waits: false
              log_min_duration_statement: -1
              log_min_error_statement: error
              log_min_messages: warning
              log_statement: none
              log_temp_files: -1
              maintenance_work_mem: 64MB
              max_client_pool_conn: 8000
              max_locks_per_transaction: 64
              max_logical_replication_workers: 4
              max_parallel_maintenance_workers: 2
              max_parallel_workers: 8
              max_parallel_workers_per_gather: 2
              max_pred_locks_per_transaction: 64
              max_prepared_transactions: 0
              max_replication_slots: 20
              max_slot_wal_keep_size: -1
              max_standby_streaming_delay: 30s
              max_wal_senders: 20
              max_worker_processes: 8
              old_snapshot_threshold: -1
              online_analyze_enable: true
              operator_precedence_warning: false
              parallel_leader_participation: true
              pg_hint_plan_debug_print: false
              pg_hint_plan_enable_hint: true
              pg_hint_plan_enable_hint_table: false
              pg_hint_plan_message_level: 'info'
              plantuner_fix_empty_table: true
              pool_mode: session
              quote_all_identifiers: false
              random_page_cost: 1.0
              replacement_sort_tuples: 150000
              row_security: true
              search_path: '"$user", public'
              seq_page_cost: 1.0
              server_reset_query_always: 1
              shared_preload_libraries: 'pg_stat_statements,pg_stat_kcache,repl_mon'
              sql_inheritance: true
              standard_conforming_strings: true
              statement_timeout: 0
              stats_temp_directory: /dev/shm/pg_stat_tmp
              synchronize_seqscans: true
              synchronous_commit: 'on'
              temp_buffers: 8MB
              temp_file_limit: -1
              timezone: 'Europe/Moscow'
              track_activity_query_size: 1024
              transform_null_equals: false
              vacuum_cleanup_index_scale_factor: 0.1
              vacuum_cost_delay: 0
              vacuum_cost_limit: 200
              vacuum_cost_page_dirty: 20
              vacuum_cost_page_hit: 1
              vacuum_cost_page_miss: 10
              wal_level: logical
              wal_log_hints: 'on'
              work_mem: 16MB
              xmlbinary: base64
              xmloption: content
            auto_resetup: true
            do_index_repack: false
            pgsync:
              failover_checks: 30
            use_barman: false
            use_pgsync: true
            use_postgis: true
            use_walg: true
            oslogin:
              enabled: true
              breakglass:
                ca_public_keys:
                  - ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC/kScoku920oGZ8vGPy9aNV+aQtihyzyEZy40Gbr9c18fGFh9Nc8SdFba/e0JKH8eOJq7TLB4mcxM2GybdPVFzoW+SkNJZEgN++YHMED1PmHMPCDB9JmijJOIiOvnkOrKIIgn7/6g841qBCXL+hn+VySHPLSooAlVJnZXSdads6nv9kXZeyU4rHzOT7BkkK4Bcbay+04srmvShno87KB47hYH0Gb6nJH8VHY7RBuqhdyDzMQ2gq1n3wYF6aLgHphboP01LTsonjIVoBJhYT+Eyie95bsg5gt7XcQP02iBvmGh/7CtkG18QpABv8GLFF5R3YL/B4LG7AMyyOTyZc5Wz yubikey
                  - ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAFxDHbZHZCx72sztvWsFSJVUhW+fngANgR5iiIWy9gicOdjYOy0pxpb1U8PqWmJYJYOWLwtfm/f9qnSV3Q/wAX/igCAwdYuwsWtx/0eBp2G3ECIUJzEQRme+TrtECpJHyLFCJonE/Cg24JlQ8N6mVjrQZwLiVjCdVwRFGoXxb3F1N4SHg== bastion
                  - ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIF6dz6foYXUqqxfqJpI/ZHabt1OiMINrrLD/LKyZJ5N3 bastion
                authorized_principals:
                  - efimkin
                  - kashinav
                  - annkpx
                  - waleriya
                  - fedusia
                  - x4mmm
                  - denchick
                  - ein-krebs
                  - munakoiso
                  - usernamedt
                  - reshke
                  - secwall
                  - dsarafan
                  - d0uble
          firewall:
            ACCEPT:
            - int: eth1
              net: ::/0
              ports: ['22']
              type: addr6
            - int: eth0
              net: ::/0
              ports: ['6432']
              type: addr6
            - int: eth0
              net: 0.0.0.0/0
              ports: ['6432']
              type: addr4
            - int: eth0
              net: data:dbaas:shard_hosts
              ports: ['5432']
              type: pillar_fqdn
            policy: DROP
            ports:
              external:
                - 6432
          mine_functions:
            grains.item: [id, role, pg, virtual]
      - type: mysql_cluster
        value:
          data:
            logship:
              topics:
                mysql_audit_logs: aoe9shbqc2v314v7fp3d/dataplane/mysql-audit-logs
                mysql_error_logs: aoe9shbqc2v314v7fp3d/dataplane/mysql-error-logs
                mysql_general_logs: aoe9shbqc2v314v7fp3d/dataplane/mysql-general-logs
                mysql_slow_logs: aoe9shbqc2v314v7fp3d/dataplane/mysql-slow-logs
            perf_diag:
              tvm_secret:
                data: {{ salt.yav.get('ver-01eezf4seyemw1eqv9vmy6rd5x[secret]') }}
                encryption_version: 1
              topics:
                my_sessions: mdb/compute/preprod/perf_diag/my_sessions
                my_statements: mdb/compute/preprod/perf_diag/my_statements
                grpc_my_sessions: aoe9shbqc2v314v7fp3d/dataplane_rt/perf_diag/my_sessions
                grpc_my_statements: aoe9shbqc2v314v7fp3d/dataplane_rt/perf_diag/my_statements
              tvm_client_id: 2021712
              tvm_server_id: 2001059
            backup:
              start:
                hours: 22
                minutes: 0
                seconds: 0
                nanos: 0
              sleep: 1800
            mysql:
              config:
                  long_query_time: 10.0
                  audit_log: false
                  general_log: false
                  max_allowed_packet: 16777216
                  default_time_zone: Europe/Moscow
                  group_concat_max_len: 1024
                  query_cache_type: 0
                  query_cache_size: 0
                  query_cache_limit: 1048576
                  net_read_timeout: 30
                  net_write_timeout: 60
                  tmp_table_size: 16777216
                  max_heap_table_size: 16777216
                  innodb_flush_log_at_trx_commit: 1
                  innodb_lock_wait_timeout: 50
                  transaction_isolation: REPEATABLE-READ
                  sql_mode:
                    - ONLY_FULL_GROUP_BY
                    - STRICT_TRANS_TABLES
                    - NO_ZERO_IN_DATE
                    - NO_ZERO_DATE
                    - ERROR_FOR_DIVISION_BY_ZERO
                    - NO_ENGINE_SUBSTITUTION
                  innodb_print_all_deadlocks: false
                  innodb_adaptive_hash_index: true
                  innodb_numa_interleave: false
                  innodb_log_buffer_size: 16777216
                  innodb_log_file_size: 268435456
                  innodb_io_capacity: 200
                  innodb_io_capacity_max: 2000
                  innodb_read_io_threads: 4
                  innodb_write_io_threads: 4
                  innodb_purge_threads: 4
                  innodb_thread_concurrency: 0
                  thread_cache_size: 10
                  thread_stack: 196608
                  join_buffer_size: 262144
                  sort_buffer_size: 262144
                  table_definition_cache: 2000
                  table_open_cache: 4000
                  table_open_cache_instances: 16
                  explicit_defaults_for_timestamp: true
                  auto_increment_increment: 1
                  auto_increment_offset: 1
                  sync_binlog: 1
                  binlog_cache_size: 32768
                  binlog_group_commit_sync_delay: 0
                  binlog_row_image: FULL
                  binlog_rows_query_log_events: false
                  mdb_preserve_binlog_bytes: 1073741824
                  rpl_semi_sync_master_wait_for_slave_count: 1
                  slave_parallel_type: LOGICAL_CLOCK
                  slave_parallel_workers: 8
                  interactive_timeout: 28800
                  wait_timeout: 28800
                  mdb_offline_mode_enable_lag: 86400
                  mdb_offline_mode_disable_lag: 300
                  range_optimizer_max_mem_size: 8388608
                  innodb_status_output: false
                  innodb_strict_mode: true
                  log_error_verbosity: 3
                  max_digest_length: 1024
                  slow_query_log: false
                  log_slow_sp_statements: false
                  slow_query_log_always_write_time: 10
                  log_slow_rate_limit: 1
                  log_slow_rate_type: 'query'
                  innodb_online_alter_log_max_size: 134217728
                  innodb_ft_min_token_size: 3
                  innodb_ft_max_token_size: 84
                  lower_case_table_names: 0
                  innodb_page_size: 16384
                  mdb_priority_choice_max_lag: 60
                  max_sp_recursion_depth: 0
                  innodb_compression_level: 6
            mdb_metrics:
              main:
                yasm_tags_cmd: /usr/local/mysql_getter.py
            oslogin:
              enabled: true
              breakglass:
                ca_public_keys:
                  - ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC/kScoku920oGZ8vGPy9aNV+aQtihyzyEZy40Gbr9c18fGFh9Nc8SdFba/e0JKH8eOJq7TLB4mcxM2GybdPVFzoW+SkNJZEgN++YHMED1PmHMPCDB9JmijJOIiOvnkOrKIIgn7/6g841qBCXL+hn+VySHPLSooAlVJnZXSdads6nv9kXZeyU4rHzOT7BkkK4Bcbay+04srmvShno87KB47hYH0Gb6nJH8VHY7RBuqhdyDzMQ2gq1n3wYF6aLgHphboP01LTsonjIVoBJhYT+Eyie95bsg5gt7XcQP02iBvmGh/7CtkG18QpABv8GLFF5R3YL/B4LG7AMyyOTyZc5Wz yubikey
                  - ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAFxDHbZHZCx72sztvWsFSJVUhW+fngANgR5iiIWy9gicOdjYOy0pxpb1U8PqWmJYJYOWLwtfm/f9qnSV3Q/wAX/igCAwdYuwsWtx/0eBp2G3ECIUJzEQRme+TrtECpJHyLFCJonE/Cg24JlQ8N6mVjrQZwLiVjCdVwRFGoXxb3F1N4SHg== bastion
                  - ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIF6dz6foYXUqqxfqJpI/ZHabt1OiMINrrLD/LKyZJ5N3 bastion
                authorized_principals:
                  - mialinx
                  - teem0n
                  - dsarafan
                  - d0uble
          firewall:
            ACCEPT:
            - int: eth1
              net: ::/0
              ports: ['22']
              type: addr6
            - int: eth0
              net: data:dbaas:cluster_hosts
              ports: ['22', '3307']
              type: pillar_fqdn
            - int: eth0
              net: ::/0
              ports: ['3306']
              type: addr6
            - int: eth0
              net: 0.0.0.0/0
              ports: ['3306']
              type: addr4
            policy: DROP
            ports:
              external:
                - 3306
      - type: hadoop_cluster
        value:
          data:
            unmanaged:
              dataproc:
                version: testing
              agent:
                manager_url: dataproc-manager.api.cloud-preprod.yandex.net:443
                cert_server_name: dataproc-manager.api.cloud-preprod.yandex.net
                ui_proxy_url: https://dataproc-ui.cloud-preprod.yandex.net/
              monitoring:
                hostname: monitoring.api.cloud-preprod.yandex.net
      - type: sqlserver_cluster
        value:
          data:
            backup:
              start:
                hours: 22
                minutes: 0
                seconds: 0
                nanos: 0
              sleep: 1800
            sqlserver:
              config:
                max_degree_of_parallelism: 0
                cost_threshold_for_parallelism: 5
                sqlcollation: SQL_Latin1_General_CP1_CI_AS
                audit_level: 0
                fill_factor_percent: 0
                in_doubt_xact_resolution: 0
                optimize_for_ad_hoc_workloads: false
                cross_db_ownership_chaining: false
            wsus:
              address: http://mdb-wsus-preprod01k.cloud-preprod.yandex.net:8530
            external_s3:
              endpoint: https://storage.cloud-preprod.yandex.net
      - type: kafka_cluster
        value:
          data:
            salt_version: 3002.7+ds-1+yandex0
            salt_py_version: 3
            allow_salt_version_update: True
            common:
              dh: {{ salt.yav.get('ver-01fc3t7z8vm9xppvsc3r2e59wf[dhparam]') | tojson }}
            logship:
              use_rt: false
              topics:
                kafka_logs: aoe9shbqc2v314v7fp3d/dataplane/kafka-logs
            kafka:
              zk_connect: localhost:2181
            zk:
              version: 3.5.5-1+yandex19-3067ff6
              config:
                dataDir: /data/zookeper
                snapCount: 1000000
                autopurge.snapRetainCount: 20  # Snapshot count
                autopurge.purgeInterval: 1  # Purge hourly
                reconfigEnabled: 'false'
            mdb_metrics:
              main:
                yasm_tags_cmd: /usr/local/yasmagent/kafka_getter.py
          firewall:
            ACCEPT:
            - int: eth1
              net: ::/0
              ports: ['22']
              type: addr6
            - int: eth0
              net: data:dbaas:cluster_hosts
              ports: ['2181', '2888', '3888', '8083', '8081', '9000', '9091', '9092']
              type: pillar_fqdn
            - int: eth0
              net: ::/0
              ports: ['9000', '9091', '9092', '443']
              type: addr6
            - int: eth0
              net: 0.0.0.0/0
              ports: ['9000', '9091', '9092', '443']
              type: addr4
            policy: DROP
      - type: elasticsearch_cluster
        value:
          data:
            elasticsearch:
              license:
                platinum:
                  data: {{ salt.yav.get('ver-01g0hhj4rh4nnrh2he2nvvw2h4[platinum]') }}
                  encryption_version: 1
                gold:
                  data: {{ salt.yav.get('ver-01g0hhj4rh4nnrh2he2nvvw2h4[gold]') }}
                  encryption_version: 1
            logship:
              use_rt: false
              topics:
                elasticsearch_logs: aoe9shbqc2v314v7fp3d/dataplane/elasticsearch-logs
                kibana_logs: aoe9shbqc2v314v7fp3d/dataplane/kibana-logs
            mdb_metrics:
                main:
                    yasm_tags_cmd: /usr/local/yasmagent/mdb_elasticsearch_getter.py
            monrun:
              unispace:
                exact_paths: /var/lib/elasticsearch
            oslogin:
              enabled: true
              breakglass:
                ca_public_keys:
                  - ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC/kScoku920oGZ8vGPy9aNV+aQtihyzyEZy40Gbr9c18fGFh9Nc8SdFba/e0JKH8eOJq7TLB4mcxM2GybdPVFzoW+SkNJZEgN++YHMED1PmHMPCDB9JmijJOIiOvnkOrKIIgn7/6g841qBCXL+hn+VySHPLSooAlVJnZXSdads6nv9kXZeyU4rHzOT7BkkK4Bcbay+04srmvShno87KB47hYH0Gb6nJH8VHY7RBuqhdyDzMQ2gq1n3wYF6aLgHphboP01LTsonjIVoBJhYT+Eyie95bsg5gt7XcQP02iBvmGh/7CtkG18QpABv8GLFF5R3YL/B4LG7AMyyOTyZc5Wz yubikey
                  - ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAFxDHbZHZCx72sztvWsFSJVUhW+fngANgR5iiIWy9gicOdjYOy0pxpb1U8PqWmJYJYOWLwtfm/f9qnSV3Q/wAX/igCAwdYuwsWtx/0eBp2G3ECIUJzEQRme+TrtECpJHyLFCJonE/Cg24JlQ8N6mVjrQZwLiVjCdVwRFGoXxb3F1N4SHg== bastion
                  - ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIF6dz6foYXUqqxfqJpI/ZHabt1OiMINrrLD/LKyZJ5N3 bastion
                authorized_principals:
                  - irioth
                  - mariadyuldina
                  - arhipov
                  - d0uble
          firewall:
            ACCEPT:
            - int: eth1
              net: ::/0
              ports: ['22']
              type: addr6
            - int: eth0
              net: ::/0
              ports: ['9200', '443']
              type: addr6
            - int: eth0
              net: 0.0.0.0/0
              ports: ['9200', '443']
              type: addr4
            - int: eth0
              net: data:dbaas:cluster_hosts
              ports: ['9300', '9201']
              type: pillar_fqdn
            policy: DROP

      - type: greenplum_cluster
        value:
          data:
            dbaas_compute:
              read_ahead_kb: 8192
            odyssey:
              user: gpadmin
            solomon_cloud:
              service: 'managed-greenplum'
            gp_data_folders:
              - /var/lib/greenplum/data1
            gp_master_directory: /var/lib/greenplum/data1
            fscreate: false
            use_monrun: false
            ssh:
              max_sessions: 200
              max_startups: '100:60:200'
            greenplum:
              config:
                gp_interconnect_type: udpifc
              master_config:
                max_connections: 250
                log_level: 5
              segment_config:
                max_connections: 750
            logship:
              topics:
                greenplum_logs: aoe9shbqc2v314v7fp3d/dataplane/greenplum-logs
                odyssey_logs: aoe9shbqc2v314v7fp3d/dataplane/greenplum-odyssey-logs
            data_transfer:
              ya_project_id: '301727744'
            oslogin:
              enabled: true
              breakglass:
                ca_public_keys:
                  - ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC/kScoku920oGZ8vGPy9aNV+aQtihyzyEZy40Gbr9c18fGFh9Nc8SdFba/e0JKH8eOJq7TLB4mcxM2GybdPVFzoW+SkNJZEgN++YHMED1PmHMPCDB9JmijJOIiOvnkOrKIIgn7/6g841qBCXL+hn+VySHPLSooAlVJnZXSdads6nv9kXZeyU4rHzOT7BkkK4Bcbay+04srmvShno87KB47hYH0Gb6nJH8VHY7RBuqhdyDzMQ2gq1n3wYF6aLgHphboP01LTsonjIVoBJhYT+Eyie95bsg5gt7XcQP02iBvmGh/7CtkG18QpABv8GLFF5R3YL/B4LG7AMyyOTyZc5Wz yubikey
                  - ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAFxDHbZHZCx72sztvWsFSJVUhW+fngANgR5iiIWy9gicOdjYOy0pxpb1U8PqWmJYJYOWLwtfm/f9qnSV3Q/wAX/igCAwdYuwsWtx/0eBp2G3ECIUJzEQRme+TrtECpJHyLFCJonE/Cg24JlQ8N6mVjrQZwLiVjCdVwRFGoXxb3F1N4SHg== bastion
                  - ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIF6dz6foYXUqqxfqJpI/ZHabt1OiMINrrLD/LKyZJ5N3 bastion
                authorized_principals:
                  - efimkin
                  - kashinav
                  - annkpx
                  - waleriya
                  - fedusia
                  - x4mmm
                  - denchick
                  - ein-krebs
                  - munakoiso
                  - usernamedt
                  - reshke
                  - secwall
                  - dsarafan
                  - d0uble
          firewall:
            ACCEPT:
            - int: eth0
              net: data:dbaas:cluster_hosts
              type: pillar_fqdn
              proto: ['tcp', 'udp']
            policy: DROP
            ports:
              external:
                - 5432
                - 6432
