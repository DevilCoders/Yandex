data:
    dbaas_metadb:
        default_alert:
            - template_id: 'managed-postgresql-master-wal-size-percent'
              critical_threshold: 30
              warning_threshold: 20
              cluster_type: 'postgresql_cluster'
              template_version: '9'
              name: 'Master WAL size'
              description: 'Checks Write Ahead Log (WAL) disk usage percentage. PostgreSQL keeps WAL needed for logical replication even after max_wal_size limit exceeded. Unit: percent, 0-100'
              mandatory: false

            - template_id: 'managed-postgresql-log-errors'
              critical_threshold: 100000
              warning_threshold: 50000
              cluster_type: 'postgresql_cluster'
              template_version: '7'
              name: 'Errors'
              description: 'Checks for count of messages with severity "ERROR" in logs. Unit: count'
              mandatory: false

            - template_id: 'managed-postgresql-log-fatals'
              critical_threshold: 100000
              warning_threshold: 50000
              cluster_type: 'postgresql_cluster'
              template_version: '4'
              name: 'Fatal errors'
              description: 'Checks for count of messages with severity "FATALS" in logs. Unit: count'
              mandatory: false

            - template_id: 'managed-postgresql-master-alive'
              critical_threshold: 0
              warning_threshold: 0
              cluster_type: 'postgresql_cluster'
              template_version: '9'
              name: 'Master is alive'
              description: 'Checks if cluster has active primary node. Unit: bool, 0/1'
              mandatory: true

            - template_id: 'managed-postgresql-average-query-time'
              critical_threshold: 100000
              warning_threshold: 50000
              cluster_type: 'postgresql_cluster'
              template_version: '7'
              name: 'Average query execution time'
              description: 'Checks average query execution time. Unit: ms'
              mandatory: false

            - template_id: 'managed-postgresql-disk-free-bytes-percent'
              critical_threshold: 10
              warning_threshold: 20
              cluster_type: 'postgresql_cluster'
              template_version: '18'
              name: 'Database free space'
              description: 'Checks database free disk space percentage. Unit: percent, 0-100'
              mandatory: true

            - template_id: 'managed-postgresql-net-usage'
              critical_threshold: 90
              warning_threshold: 80
              cluster_type: 'postgresql_cluster'
              template_version: '8'
              name: 'Network usage'
              description: 'Checks network usage percentage per host. Unit: percent, 0-100'
              mandatory: true

            - template_id: 'managed-postgresql-cpu-usage'
              critical_threshold: 90
              warning_threshold: 80
              cluster_type: 'postgresql_cluster'
              template_version: '14'
              name: 'CPU usage'
              description: 'Checks CPU usage percentage per host. Unit: percent, 0-100'
              mandatory: true

            - template_id: 'managed-postgresql-io-usage'
              critical_threshold: 90
              warning_threshold: 80
              cluster_type: 'postgresql_cluster'
              template_version: '8'
              name: 'Disk IO usage'
              description: 'Checks disk IO usage percentage per host. Unit: percent, 0-100'
              mandatory: true

            - template_id: 'managed-postgresql-memory-usage'
              critical_threshold: 90
              warning_threshold: 80
              cluster_type: 'postgresql_cluster'
              template_version: '8'
              name: 'Memory usage'
              description: 'Checks memory usage percentage per host. Unit: percent, 0-100'
              mandatory: true

            - template_id: 'managed-postgresql-query-0.9'
              critical_threshold: 10000
              warning_threshold: 1000
              cluster_type: 'postgresql_cluster'
              template_version: '5'
              name: '0.9 quantile of query execution time'
              description: 'Checks 0.9 quantile of query execution time, aggregated across all databases and users. Unit: ms'
              mandatory: false

            - template_id: 'managed-postgresql-transaction-0.9'
              critical_threshold: 10000
              warning_threshold: 1000
              cluster_type: 'postgresql_cluster'
              template_version: '6'
              name: '0.9 quantile of transaction execution time'
              description: 'Checks 0.9 quantile of transaction execution time, aggregated across all databases and users. Unit: ms'
              mandatory: false

            - template_id: 'managed-postgresql-used-connections'
              critical_threshold: 90
              warning_threshold: 80
              cluster_type: 'postgresql_cluster'
              template_version: '8'
              name: 'Connections usage'
              description: 'Checks percentage of used connections, meaning connections established from connection pooler (Odyssey) to PostgreSQL. Especially recommended for session pooling mode in connection pooler. Unit: percent, 0-100'
              mandatory: false

            - template_id: 'managed-postgresql-oldest-transaction'
              critical_threshold: 3600
              warning_threshold: 1800
              cluster_type: 'postgresql_cluster'
              template_version: '2'
              name: 'Oldest transaction execution time'
              description: 'Checks oldest transaction execution time, aggregated across all databases and users. Unit: s'
              mandatory: true
