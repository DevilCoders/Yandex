data:
  dbaas_metadb:
    cluster_type_pillars:
      - type: clickhouse_cluster
        value:
          data:
            access:
              user_hosts:
              - fqdn: s3-private.mds.yandex.net
                port: 443
              - fqdn: s3.mds.yandex.net
                port: 80
              management_hosts:
              - fqdn: s3-private.mds.yandex.net
                port: 443
              - fqdn: s3.mds.yandex.net
                port: 80
              - fqdn: storage.yandexcloud.net
                port: 443
            logship:
              topics:
                clickhouse_logs: mdb/porto/prod/clickhouse-logs
            backup:
              start:
                hours: 23
                minutes: 0
                seconds: 0
                nanos: 0
              sleep: 7200
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
            monrun:
              unispace:
                exact_paths: /var/lib/clickhouse
            database_slice:
              enable: true
            testing_repos: true
      - type: mongodb_cluster
        value:
          data:
            salt_version: 3002.7+ds-1+yandex0
            salt_py_version: 3
            allow_salt_version_update: False
            logship:
              topics:
                mongodb_logs: mdb/porto/prod/mongodb-logs
                mongodb_audit_logs: mdb/porto/prod/mongodb-audit-logs
            perf_diag:
              tvm_secret:
                data: {{ salt.yav.get('ver-01e0wq4x7q22g87wvkdh1yre03[tvm_secret]') }}
                encryption_version: 1
              topics:
                mongodb-profiler: mdb/porto/prod/perf_diag/mongodb_profiler
              tvm_client_id: 2016999
              tvm_server_id: 2001059
            backup:
              start:
                hours: 22
                minutes: 0
                seconds: 0
                nanos: 0
              sleep: 7200
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
            mdb_metrics:
              enabled: true
              enable_userfault_broken_collector: false
              main:
                yasm_tags_cmd: /usr/local/yasmagent/mdb_mongodb_getter.py
                yasm_tags_db: mongodb
            database_slice:
              enable: true
            monrun:
              unispace:
                exact_paths: /var/lib/mongodb
            walg:
              enabled: true
              install: true
      - type: redis_cluster
        value:
          data:
            salt_version: 3002.7+ds-1+yandex0
            salt_py_version: 3
            allow_salt_version_update: False
            logship:
              topics:
                redis_logs: mdb/porto/prod/redis-logs
            backup:
              start:
                hours: 22
                minutes: 0
                seconds: 0
                nanos: 0
              sleep: 7200
            redis:
              config:
                maxmemory-policy: noeviction
                notify-keyspace-events: '""'
                slowlog-log-slower-than: 10000
                slowlog-max-len: 1000
                databases: 16
                timeout: 0
            use_yasmagent: true
            walg:
              install: true
              enabled: true
            database_slice:
              enable: true
            mdb_metrics:
              enable_userfault_broken_collector: false
              main:
                yasm_tags_cmd: /usr/local/yasmagent/redis_getter.py
                yasm_tags_db: redis
            monrun:
              unispace:
                exact_paths: /var/lib/redis
      - type: postgresql_cluster
        value:
          data:
            sox_audit: true
            odyssey:
              user: postgres
            logship:
              topics:
                postgresql_logs: mdb/porto/prod/postgresql-logs
                pgbouncer_logs: mdb/porto/prod/pgbouncer-logs
                odyssey_logs: mdb/porto/prod/odyssey-logs
            perf_diag:
              tvm_secret:
                data: {{ salt.yav.get('ver-01e0wq4x7q22g87wvkdh1yre03[tvm_secret]') }}
                encryption_version: 1
              topics:
                pg_stat_activity: mdb/porto/prod/perf_diag/pg_stat_activity
                pg_stat_statements: mdb/porto/prod/perf_diag/pg_stat_statements
                pg_stat_statements_query: mdb/porto/prod/perf_diag/pg_stat_statements_query
                grpc_pg_stat_activity: mdb/porto/prod_rt/perf_diag/pg_stat_activity
                grpc_pg_stat_statements: mdb/porto/prod_rt/perf_diag/pg_stat_statements
              tvm_client_id: 2016999
              tvm_server_id: 2001059
            backup:
              start:
                hours: 22
                minutes: 0
                seconds: 0
                nanos: 0
              sleep: 7200
            config:
              amcheck: True
              archive_timeout: 600
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
              bgwriter_delay: 10ms
              bgwriter_flush_after: 0
              bgwriter_lru_maxpages: 1000
              bgwriter_lru_multiplier: 10.0
              bytea_output: hex
              checkpoint_completion_target: 0.8
              checkpoint_flush_after: 0
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
              enable_mergejoin: false
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
              lock_timeout: 1s
              log_checkpoints: true
              log_connections: false
              log_disconnections: false
              log_duration: false
              log_error_verbosity: default
              log_keep_days: 1
              log_lock_waits: true
              log_min_duration_statement: -1
              log_min_error_statement: error
              log_min_messages: warning
              log_statement: ddl
              log_temp_files: 0
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
              pool_mode: transaction
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
            monrun:
              unispace:
                exact_paths: /var/lib/postgresql
            auto_resetup: true
            do_index_repack: true
            pgsync:
              min_failover_timeout: 1800
              failover_checks: 30
            use_barman: false
            use_pgsync: true
            use_postgis: true
            use_walg: true
            database_slice:
              enable: true
          mine_functions:
            grains.item: [id, role, ya, pg, virtual]
      - type: mysql_cluster
        value:
          data:
            logship:
              topics:
                mysql_audit_logs: mdb/porto/prod/mysql-audit-logs
                mysql_error_logs: mdb/porto/prod/mysql-error-logs
                mysql_general_logs: mdb/porto/prod/mysql-general-logs
                mysql_slow_logs: mdb/porto/prod/mysql-slow-logs
            perf_diag:
              tvm_secret:
                data: {{ salt.yav.get('ver-01e0wq4x7q22g87wvkdh1yre03[tvm_secret]') }}
                encryption_version: 1
              topics:
                my_sessions: mdb/porto/prod/perf_diag/my_sessions
                my_statements: mdb/porto/prod/perf_diag/my_statements
                grpc_my_sessions: mdb/porto/prod_rt/perf_diag/my_sessions
                grpc_my_statements: mdb/porto/prod_rt/perf_diag/my_statements
              tvm_client_id: 2016999
              tvm_server_id: 2001059
            backup:
              start:
                hours: 22
                minutes: 0
                seconds: 0
                nanos: 0
              sleep: 7200
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
            database_slice:
              enable: true
            monrun:
              unispace:
                exact_paths: /var/lib/mysql
      - type: elasticsearch_cluster
        value:
          data:
            logship:
              use_rt: false
              topics:
                elasticsearch_logs: mdb/porto/prod/elasticsearch-logs
                kibana_logs: mdb/porto/prod/kibana-logs
            use_yasmagent: true
            mdb_metrics:
              main:
                yasm_tags_cmd: /usr/local/yasmagent/mdb_elasticsearch_getter.py
            monrun:
              unispace:
                exact_paths: /var/lib/elasticsearch
      - type: kafka_cluster
        value:
          data:
            salt_version: 3002.7+ds-1+yandex0
            salt_py_version: 3
            allow_salt_version_update: True
            common:
              dh: {{ salt.yav.get('ver-01fc4as1z6y8s55nxq6b88tvnc[dhparam]') | tojson }}
            logship:
              use_rt: false
              topics:
                kafka_logs: mdb/porto/prod/kafka-logs
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
      - type: greenplum_cluster
        value:
          data:
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
            data_transfer:
              ya_project_id: '1951006720'
            greenplum:
              config:
                gp_resource_manager: queue
              master_config:
                max_connections: 250
                log_level: 5
              segment_config:
                max_connections: 750
            logship:
              topics:
                greenplum_logs: mdb/porto/prod/greenplum-logs
                odyssey_logs: mdb/porto/prod/greenplum-odyssey-logs
