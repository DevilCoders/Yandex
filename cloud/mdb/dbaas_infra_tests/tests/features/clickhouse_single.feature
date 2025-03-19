@dependent-scenarios
Feature: Internal API Clickhouse single instance Cluster lifecycle

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And Go Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with single ClickHouse cluster

  @setup
  Scenario: Create ClickHouse cluster
    When we try to create cluster "test_cluster"
    """
    configSpec:
      version: "{{ versions[-3] }}"
      clickhouse:
        resources:
          resourcePresetId: db1.micro
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And cluster "test_cluster" is up and running
    And cluster environment is "PRESTABLE"
    And all databases exist in cluster "test_cluster"
    And s3 has bucket for cluster
    And cluster has no pending changes

  @geobase
  Scenario: Geobase working
    Given cluster "test_cluster" is up and running
    Then following query on all hosts succeeds
    """
    SELECT regionToName(CAST(1 AS UInt32))
    """

  Scenario: ClickHouse cluster task reject works
    Given cluster "test_cluster" is up and running
    When we lock cluster
    And we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        config:
          logLevel: ERROR
    """
    Then response should have status 200
    And generated task is failed within "6 minutes"
    When we unlock cluster
    Then cluster has no pending changes

  Scenario: Create MergeTree table in ClickHouse cluster
    Given cluster "test_cluster" is up and running
    When we execute the following query on all hosts
    """
    CREATE TABLE testdb.mtree (
        date  Date,
        text  text,
        id    Int64
    )
    ENGINE MergeTree
    PARTITION BY date
    ORDER BY id;
    """
    And we execute the following query on all hosts
    """
    INSERT INTO testdb.mtree VALUES(today(), 'test', 1);
    """
    Then following query return "1" on all hosts
    """
    SELECT count() FROM testdb.mtree;
    """

  Scenario: Create external dictionary in ClickHouse cluster
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        config:
          dictionaries:
            - name: test_dict
              layout:
                type: FLAT
              httpSource:
                url: https://localhost:8443/?query=select%201%2C%27test%27
                format: TSV
              fixedLifetime: 300
              structure:
                id:
                  name: id
                attributes:
                  - name: text
                    type: String
                    nullValue: ""
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    Then following query return "test" on all hosts
    """
    SELECT dictGetString('test_dict', 'text', toUInt64(1));
    """

  Scenario: Modify ClickHouse cluster configuration
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        config:
          logLevel: ERROR
          mergeTree:
            replicatedDeduplicationWindow: 150
            replicatedDeduplicationWindowSeconds: 302400
          compression:
            - minPartSize: 10000000000
              minPartSizeRatio: 0.01
              method: ZSTD
          graphiteRollup:
            - name: test_rollup
              patterns:
                - function: any
                  retention:
                    - age: 60
                      precision: 1
          maxConnections: 30
          maxConcurrentQueries: 100
          keepAliveTimeout: 5
          uncompressedCacheSize: 83886080
          markCacheSize: 10737418240
          maxTableSizeToDrop: 214748364800
          maxPartitionSizeToDrop: 214748364800
          builtinDictionariesReloadInterval: 4200
          timezone: UTC
          queryLogRetentionSize: 1000000000
          queryLogRetentionTime: 864000000
          queryThreadLogEnabled: true
          queryThreadLogRetentionSize: 1000000001
          queryThreadLogRetentionTime: 864001000
          partLogRetentionSize: 1000000002
          partLogRetentionTime: 864002000
          metricLogEnabled: true
          metricLogRetentionSize: 1000000003
          metricLogRetentionTime: 864003000
          traceLogEnabled: true
          traceLogRetentionSize: 1000000004
          traceLogRetentionTime: 864004000
          textLogEnabled: true
          textLogRetentionSize: 1000000005
          textLogRetentionTime: 864005000
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And config on hosts contain the following parameters
    """
    logger:
      level: error
    merge_tree:
      replicated_deduplication_window: '150'
      replicated_deduplication_window_seconds: '302400'
    compression:
      case:
        method: zstd
        min_part_size: '10000000000'
        min_part_size_ratio: '0.01'
    max_connections: '30'
    max_concurrent_queries: '100'
    keep_alive_timeout: '5'
    uncompressed_cache_size: '83886080'
    mark_cache_size: '10737418240'
    max_table_size_to_drop: '214748364800'
    max_partition_size_to_drop: '214748364800'
    timezone: UTC
    test_rollup:
      default:
        function: any
        retention:
          age: '60'
          precision: '1'
    """

  Scenario: Create external PostgreSQL dictionary in ClickHouse cluster
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        config:
          dictionaries:
            - name: test_dict
              layout:
                type: FLAT
              postgresqlSource:
                db: test_db
                table: test_table
                hosts:
                  - host1
                  - host2
                  - host3
                user: test_user
                port: 6666
                password: test_password
                invalidateQuery: 'SELECT rand()'
                sslMode: DISABLE
              fixedLifetime: 300
              structure:
                id:
                  name: id
                attributes:
                  - name: text
                    type: String
                    nullValue: ''
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And config "/etc/clickhouse-server/dictionaries.d/test_dict.xml" on hosts contain the following parameters
    """
    dictionary:
      name: test_dict
      layout:
        flat: null
      source:
        odbc:
          table: test_table
          invalidate_query: 'SELECT rand()'
          connection_string: DSN=test_dict_dsn
      lifetime: '300'
      structure:
        id:
          name: id
        attribute:
          name: text
          type: String
          null_value: null
    """
    And config "/etc/odbc.ini" on hosts contain the following parameters
    """
    test_dict_dsn:
      Servername: host1,host2,host3
      Port: '6666'
      Database: test_db
      UserName: test_user
      Password: test_password
      Sslmode: disable
    """

  Scenario: Create ML model in ClickHouse cluster
    Given cluster "test_cluster" is up and running
    And s3 object "model.bin"
    When we attempt to create ML model in "ClickHouse" cluster
    """
    ml_model_name: test_model
    type: ML_MODEL_TYPE_CATBOOST
    uri: '{{ s3_object_url }}'
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And s3 object present "object_cache/ml_model/test_model"
    When we attempt to create ML model in "ClickHouse" cluster
    """
    ml_model_name: second_model
    type: ML_MODEL_TYPE_CATBOOST
    uri: '{{ s3_object_url }}'
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And s3 object present "object_cache/ml_model/second_model"

  Scenario: Create ML model with invalid URI rejects
    Given cluster "test_cluster" is up and running
    And s3 object "model.bin"
    When we attempt to create ML model in "ClickHouse" cluster
    """
    ml_model_name: test_model_invalid
    type: ML_MODEL_TYPE_CATBOOST
    uri: '{{ s3_object_url }}/invalid.bin'
    """
    Then response status is "OK" [grpc]
    And generated task is failed within "6 minutes" with error message "Unable to download ml_model: test_model_invalid"
    Then cluster has no pending changes
    And s3 object absent "object_cache/ml_model/test_model_invalid"

  Scenario: Modify ML model in ClickHouse cluster with invalid URI rejects
    Given cluster "test_cluster" is up and running
    And s3 object "model.bin"
    When we attempt to update ML model in "ClickHouse" cluster
    """
    ml_model_name: test_model
    uri: '{{ s3_object_url }}/invalid.bin'
    """
    Then response status is "OK" [grpc]
    And generated task is failed within "6 minutes" with error message "Unable to download ml_model: test_model"
    And cluster has no pending changes
    And s3 object present "object_cache/ml_model/test_model"

  Scenario: Modify ML model in ClickHouse cluster
    Given cluster "test_cluster" is up and running
    And s3 object "new_model.bin"
    When we attempt to update ML model in "ClickHouse" cluster
    """
    ml_model_name: test_model
    uri: '{{ s3_object_url }}'
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And s3 object present "object_cache/ml_model/test_model"

  Scenario: Delete ML model in ClickHouse cluster
    Given cluster "test_cluster" is up and running
    When we attempt to delete ML model in "ClickHouse" cluster
    """
    ml_model_name: test_model
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And s3 object absent "object_cache/ml_model/test_model"

  Scenario: Configure custom geobase
    Given cluster "test_cluster" is up and running
    And "geobase.tar" archive in s3
    """
    regions_hierarchy.txt: ''
    regions_names_en.txt: ''
    regions_names_ru.txt: ''
    """
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        config:
          geobaseUri: '{{ s3_object_url }}'
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And s3 object present "object_cache/geobase/custom_clickhouse_geobase"

  Scenario: Configure custom geobase with invalid URI rejects
    Given cluster "test_cluster" is up and running
    And "geobase.tar" archive in s3
    """
    regions_hierarchy.txt: ''
    regions_names_en.txt: ''
    regions_names_ru.txt: ''
    """
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        config:
          geobaseUri: '{{ s3_object_url }}/invalid.tar'
    """
    Then response should have status 200
    And generated task is failed within "6 minutes" with error message "Unable to download geobase: custom_clickhouse_geobase"
    And cluster has no pending changes
    And s3 object present "object_cache/geobase/custom_clickhouse_geobase"

  Scenario: Create format schema in ClickHouse cluster
    Given cluster "test_cluster" is up and running
    And s3 object "schema.proto" with data
    """
    // test schema
    """
    When we attempt to create format schema in "ClickHouse" cluster
    """
    format_schema_name: test_schema
    type: FORMAT_SCHEMA_TYPE_PROTOBUF
    uri: '{{ s3_object_url }}'
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And s3 object present "object_cache/format_schema/test_schema" with data
    """
    // test schema
    """

  Scenario: Create format schema with invalid URI rejects
    Given cluster "test_cluster" is up and running
    And s3 object "schema.proto"
    When we attempt to create format schema in "ClickHouse" cluster
    """
    format_schema_name: test_schema_invalid
    type: FORMAT_SCHEMA_TYPE_PROTOBUF
    uri: '{{ s3_object_url }}/invalid.proto'
    """
    Then response status is "OK" [grpc]
    And generated task is failed within "6 minutes" with error message "Unable to download format_schema: test_schema_invalid"
    Then cluster has no pending changes
    And s3 object absent "object_cache/format_schema/test_schema_invalid"

  Scenario: Modify format schema in ClickHouse cluster with invalid link
    Given cluster "test_cluster" is up and running
    And s3 object "schema.proto"
    When we attempt to update format schema in "ClickHouse" cluster
    """
    format_schema_name: test_schema
    uri: '{{ s3_object_url }}/invalid.proto'
    """
    Then response status is "OK" [grpc]
    And generated task is failed within "6 minutes" with error message "Unable to download format_schema: test_schema"
    And cluster has no pending changes
    And s3 object present "object_cache/format_schema/test_schema" with data
    """
    // test schema
    """

  Scenario: Modify format schema in ClickHouse cluster
    Given cluster "test_cluster" is up and running
    And s3 object "new_schema.proto" with data
    """
    // new schema
    """
    When we attempt to update format schema in "ClickHouse" cluster
    """
    format_schema_name: test_schema
    uri: '{{ s3_object_url }}'
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And s3 object present "object_cache/format_schema/test_schema" with data
    """
    // new schema
    """

  Scenario: Create format schema in ClickHouse cluster with сyrillic symbols
    Given cluster "test_cluster" is up and running
    And s3 object "new_schema_rus.proto" with data
    """
    // Русский текст в файле
    """
    When we attempt to create format schema in "ClickHouse" cluster
    """
    format_schema_name: rus_schema
    type: FORMAT_SCHEMA_TYPE_PROTOBUF
    uri: '{{ s3_object_url }}'
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And s3 object present "object_cache/format_schema/rus_schema"
    """
    // Русский текст в файле
    """

  Scenario: Delete format schema in ClickHouse cluster
    Given cluster "test_cluster" is up and running
    When we attempt to delete format schema in "ClickHouse" cluster
    """
    format_schema_name: test_schema
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And s3 object absent "object_cache/format_schema/test_schema"

  @users
  Scenario: Add user to ClickHouse cluster gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to create user in "ClickHouse" cluster
    """
    user_spec:
      name: petya
      password: password-password-password-password
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "10 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters
    """
    user: petya
    password: password-password-password-password
    """

  @users
  Scenario: Add original user to ClickHouse cluster
    Given cluster "test_cluster" is up and running
    When we attempt to create user in "ClickHouse" cluster
    """
    user_spec:
      name: pet-ya-
      password: password-password-password-пароль
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "10 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters
    """
    user: pet-ya-
    password: password-password-password-пароль
    """

  @users
  Scenario: Remove ClickHouse user gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to delete user in "ClickHouse" cluster
    """
    user_name: petya
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "10 minutes"
    And cluster has no pending changes
    And we get users in cluster
    And we are unable to log in to "test_cluster" with following parameters
    """
    user: petya
    password: password_changed-password_changed-password_changed
    """

  @users
  @quotas
  Scenario: Test user quotas grpc
    Given cluster "test_cluster" is up and running
    When we attempt to create user in "ClickHouse" cluster
    """
    user_spec:
      name: quota_user
      password: password-password
      quotas:
        - interval_duration: 3600000
          queries: 42
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "10 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters
    """
    user: quota_user
    password: password-password
    """
    Then quota "max_queries" with duration "3600" for user "quota_user" is "42"

  @users
  @settings
  Scenario: Test user settings grpc
    Given cluster "test_cluster" is up and running
    When we attempt to create user in "ClickHouse" cluster
    """
    user_spec:
      name: settings_user
      password: password-password
      settings:
        maxMemoryUsage: 12345678
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "10 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters
    """
    user: settings_user
    password: password-password
    """
    When we execute on all hosts with credentials "settings_user" "password-password" and result is "12345678"
    """
    SELECT value FROM system.settings WHERE name='max_memory_usage';
    """

  @databases
  Scenario: Add ClickHouse database to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add database in "test_cluster"
    """
    databaseSpec:
      name: testdb_3
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  @databases
  Scenario: Add ClickHouse database to cluster [gRPC]
    Given cluster "test_cluster" is up and running
    When we attempt to create database in "ClickHouse" cluster "test_cluster" with data [gRPC]
    """
    database_spec:
      name: testdb_4
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  @databases
  Scenario: Grant permissions to created database
    Given cluster "test_cluster" is up and running
    When we attempt to grant permission to user in "ClickHouse" cluster
    """
    user_name: test_user
    permission:
      database_name: testdb_3
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  @users
  @quotas
  Scenario: Test user quotas after database added
    Given cluster "test_cluster" is up and running
    When we attempt to grant permission to user in "ClickHouse" cluster
    """
    user_name: quota_user
    permission:
      database_name: testdb_3
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters
    """
    user: quota_user
    password: password-password
    """
    Then quota "max_queries" with duration "3600" for user "quota_user" is "42"

  @users
  @settings
  Scenario: Test user settings after database added
    Given cluster "test_cluster" is up and running
    When we attempt to grant permission to user in "ClickHouse" cluster
    """
    user_name: settings_user
    permission:
      database_name: testdb_3
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters
    """
    user: settings_user
    password: password-password
    """
    When we execute on all hosts with credentials "settings_user" "password-password" and result is "12345678"
    """
    SELECT value FROM system.settings WHERE name='max_memory_usage';
    """

  @databases
  Scenario: Remove ClickHouse database from cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove database "testdb_3" in cluster
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  @databases
  Scenario: Remove ClickHouse database from cluster [gRPC]
    Given cluster "test_cluster" is up and running
    When we attempt to delete database in "ClickHouse" cluster "test_cluster" with data [gRPC]
    """
    database_name: testdb_4
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  @databases
  Scenario: Remove big ClickHouse database from cluster
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        config:
          maxTableSizeToDrop: 100
          maxPartitionSizeToDrop: 100
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    Then following query on all hosts succeeds
    """
    CREATE TABLE testdb_big.test_table (
        EventDate DateTime,
        CounterID UInt32,
        UserID UInt32
    )
    ENGINE MergeTree
    PARTITION BY toYYYYMM(EventDate)
    ORDER BY (CounterID, EventDate, intHash32(UserID))
    SAMPLE BY intHash32(UserID);
    """
    And following query on all hosts succeeds
    """
    INSERT INTO testdb_big.test_table SELECT now(), number, rand() FROM system.numbers LIMIT 1000000
    """
    When we attempt to remove database "testdb_big" in cluster
    Then response should have status 200
    And generated task is finished within "6 minutes"
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        config:
          maxTableSizeToDrop: 53687091200
          maxPartitionSizeToDrop: 53687091200
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  @backups
  Scenario: Create cluster backup
    Given cluster "test_cluster" is up and running
    When we get ClickHouse backups for "test_cluster"
    Then we remember current response as "initial_backups"
    When we execute the following query on all hosts
    """
    CREATE TABLE testdb.logs (
        date     Date,
        message  text,
        id       Int64
    )
    ENGINE MergeTree
    PARTITION BY date
    ORDER BY id;
    """
    And we execute the following query on all hosts
    """
    INSERT INTO testdb.logs (date, message, id)
    VALUES (now(), 'Test', 1),
           (now(), 'Important!', 2),
           (now(), 'Urgnet!!!', 3);
    """
    And we create backup for ClickHouse "test_cluster"
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And we get ClickHouse backups for "test_cluster"
    And response should have status 200
    And message with "backups" list is larger than in "initial_backups"
    And we remember current response as "backups"
    And following query on all hosts succeeds
    """
    DROP TABLE testdb.logs NO DELAY;
    """

  @backups
  Scenario: Create cluster backup [gRPC]
    Given cluster "test_cluster" is up and running
    When we get ClickHouse backups for "test_cluster"
    Then we remember current response as "initial_backups"
    When we execute the following query on all hosts
    """
    CREATE TABLE testdb.logs (
        date     Date,
        message  text,
        id       Int64
    )
    ENGINE MergeTree
    PARTITION BY date
    ORDER BY id;
    """
    And we execute the following query on all hosts
    """
    INSERT INTO testdb.logs (date, message, id)
    VALUES (now(), 'Test', 1),
           (now(), 'Important!', 2),
           (now(), 'Urgnet!!!', 3);
    """
    And we create backup for ClickHouse "test_cluster" [gRPC]
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And we get ClickHouse backups for "test_cluster"
    And response should have status 200
    And message with "backups" list is larger than in "initial_backups"
    And we remember current response as "backups"
    And following query on all hosts succeeds
    """
    DROP TABLE testdb.logs NO DELAY;
    """

  @backups
  Scenario: Restore cluster from backup
    Given cluster "test_cluster" is up and running
    And s3 object "new_schema_rus.proto" with data
    """
    // Другой текст
    """
    When we restore ClickHouse using latest "backups" and config
    """
    name: test_cluster_at_last_backup
    environment: PRESTABLE
    hostSpecs:
      - zoneId: man
        type: CLICKHOUSE
    configSpec:
      clickhouse:
        resources:
          resourcePresetId: db1.micro
          diskTypeId: local-ssd
          diskSize: 20000000000
    """
    Then response should have status 200
    And generated task is finished within "30 minutes"
    And we get ClickHouse backups for "test_cluster_at_last_backup"
    And response should have status 200
    And message should have empty "backups" list
    And we get cluster "test_cluster_at_last_backup"
    And following query return "2" on all hosts
    """
    SELECT COUNT(*)
      FROM testdb.logs
     WHERE message IN ('Important!', 'Urgnet!!!');
    """
    And cluster has no pending changes
    And s3 object present "object_cache/geobase/custom_clickhouse_geobase"
    And s3 object present "object_cache/ml_model/second_model"
    And s3 object present "object_cache/format_schema/rus_schema"
    """
    // Русский текст в файле
    """

  @backups
  Scenario: Remove restored cluster
    Given cluster "test_cluster_at_last_backup" is up and running
    When we attempt to remove cluster "test_cluster_at_last_backup"
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And we are unable to find cluster "test_cluster_at_last_backup"

  @shards
  Scenario: Add shard
    Given cluster "test_cluster" is up and running
    When we attempt to add shard in cluster
    """
    shardName: shard2
    hostSpecs:
      - zoneId: sas
        type: CLICKHOUSE
    copySchema: true
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And following query return "2" on all hosts
    """
    SELECT count() FROM (SELECT DISTINCT shard_num FROM system.clusters)
    """
    And following query on all hosts succeeds
    """
    SELECT COUNT(*) FROM testdb.mtree
    """

  @shard_groups
  Scenario: Add shard group
    Given cluster "test_cluster" is up and running
    When we attempt to create shard group in "ClickHouse" cluster
    """
    shard_group_name: "test_shard_group"
    description: "Test shard group"
    shard_names: ["shard1", "shard2"]
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "10 minutes"
    And following query return "{{ cluster['id'] }}\ntest_shard_group" on all hosts
    """
    SELECT DISTINCT cluster FROM system.clusters ORDER BY cluster
    """
    When we attempt to delete shard group in "ClickHouse" cluster
    """
    shard_group_name: "test_shard_group"
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "10 minutes"
    And following query return "{{ cluster['id'] }}" on all hosts
    """
    SELECT DISTINCT cluster FROM system.clusters ORDER BY cluster
    """

  @shards
  Scenario: Create backup of sharded cluster
    Given cluster "test_cluster" is up and running
    When we get ClickHouse backups for "test_cluster"
    Then we remember current response as "initial_backups"
    When we execute the following query on all hosts of "shard1"
    """
    CREATE TABLE testdb.logs (
        date     Date,
        message  text,
        id       Int64
    )
    ENGINE MergeTree
    PARTITION BY date
    ORDER BY id;
    """
    And we execute the following query on all hosts of "shard1"
    """
    INSERT INTO testdb.logs (date, message, id)
    VALUES (now(), 'Test', 1),
           (now(), 'Important!', 2),
           (now(), 'Urgnet!!!', 3);
    """
    And we execute the following query on all hosts of "shard2"
    """
    CREATE TABLE testdb.logs2 (
        date     Date,
        message  text,
        id       Int64
    )
    ENGINE MergeTree
    PARTITION BY date
    ORDER BY id;
    """
    And we execute the following query on all hosts of "shard2"
    """
    INSERT INTO testdb.logs2 (date, message, id)
    VALUES (now(), 'Test', 1),
           (now(), 'Important!', 2),
           (now(), 'Urgnet!!!', 3);
    """
    And we create backup for ClickHouse "test_cluster"
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And we get ClickHouse backups for "test_cluster"
    And message with "backups" list is larger than in "initial_backups"
    And we remember current response as "backups"
    And following query on all hosts of "shard1" succeeds
    """
    DROP TABLE testdb.logs NO DELAY;
    """
    And following query on all hosts of "shard2" succeeds
    """
    DROP TABLE testdb.logs2 NO DELAY;
    """

  @shards
  Scenario: Create backup of sharded cluster [gRPC]
    Given cluster "test_cluster" is up and running
    When we get ClickHouse backups for "test_cluster"
    Then we remember current response as "initial_backups"
    When we execute the following query on all hosts of "shard1"
    """
    CREATE TABLE testdb.logs (
        date     Date,
        message  text,
        id       Int64
    )
    ENGINE MergeTree
    PARTITION BY date
    ORDER BY id;
    """
    And we execute the following query on all hosts of "shard1"
    """
    INSERT INTO testdb.logs (date, message, id)
    VALUES (now(), 'Test', 1),
           (now(), 'Important!', 2),
           (now(), 'Urgnet!!!', 3);
    """
    And we execute the following query on all hosts of "shard2"
    """
    CREATE TABLE testdb.logs2 (
        date     Date,
        message  text,
        id       Int64
    )
    ENGINE MergeTree
    PARTITION BY date
    ORDER BY id;
    """
    And we execute the following query on all hosts of "shard2"
    """
    INSERT INTO testdb.logs2 (date, message, id)
    VALUES (now(), 'Test', 1),
           (now(), 'Important!', 2),
           (now(), 'Urgnet!!!', 3);
    """
    And we create backup for ClickHouse "test_cluster" [gRPC]
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And we get ClickHouse backups for "test_cluster"
    And message with "backups" list is larger than in "initial_backups"
    And we remember current response as "backups"
    And following query on all hosts of "shard1" succeeds
    """
    DROP TABLE testdb.logs NO DELAY;
    """
    And following query on all hosts of "shard2" succeeds
    """
    DROP TABLE testdb.logs2 NO DELAY;
    """

  @shards
  Scenario: Restore shard from backup
    Given cluster "test_cluster" is up and running
    When we restore ClickHouse using latest "backups" containing "shard2" and config
    """
    name: test_cluster_at_last_backup
    environment: PRESTABLE
    hostSpecs:
      - zoneId: man
        type: CLICKHOUSE
    configSpec:
      clickhouse:
        resources:
          resourcePresetId: db1.micro
          diskTypeId: local-ssd
          diskSize: 10737418240
    """
    Then response should have status 200
    And generated task is finished within "30 minutes"
    And we get ClickHouse backups for "test_cluster_at_last_backup"
    And response should have status 200
    And message should have empty "backups" list
    And we get cluster "test_cluster_at_last_backup"
    And following query return "2" on all hosts
    """
    SELECT COUNT(*)
      FROM testdb.logs2
     WHERE message IN ('Important!', 'Urgnet!!!');
    """
    And cluster has no pending changes

  @shards
  Scenario: Remove cluster with restored shard
    Given cluster "test_cluster_at_last_backup" is up and running
    When we attempt to remove cluster "test_cluster_at_last_backup"
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And we are unable to find cluster "test_cluster_at_last_backup"

  @shards
  Scenario: Modify shard
    Given cluster "test_cluster" is up and running
    When we attempt to modify shard "shard2" in cluster
    """
    configSpec:
      clickhouse:
        resources:
          resourcePresetId: db1.micro
          diskSize: 21474836480
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes

  @shards
  Scenario: Remove shard
    Given cluster "test_cluster" is up and running
    When we attempt to remove shard "shard2" in cluster
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes

  Scenario: Remove ClickHouse cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove cluster "test_cluster"
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And in worker_queue exists "clickhouse_cluster_delete_metadata" task
    And this task is done
    And autogenerated shard was deleted from Solomon
    And we are unable to find cluster "test_cluster"
    But s3 has bucket for cluster
    And in worker_queue exists "clickhouse_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster
