@dependent-scenarios
Feature: Go Internal API Clickhouse Cluster lifecycle

  Background: Wait until Go internal api is ready
    Given Go Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard ClickHouse cluster

  @setup
  Scenario: Create ClickHouse cluster gRPC
    When we create "ClickHouse" cluster "test_cluster" [grpc]
    """
    configSpec:
      version: "{{ versions[-2] }}"
      clickhouse:
        resources:
          resourcePresetId: db1.micro

    hostSpecs:
      - zoneId: vla
        type: CLICKHOUSE
      - zoneId: man
        type: CLICKHOUSE

      - zoneId: vla
        type: ZOOKEEPER
      - zoneId: man
        type: ZOOKEEPER
      - zoneId: iva
        type: ZOOKEEPER
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "30 minutes"
    And cluster "test_cluster" is up and running
    And cluster environment is "PRESTABLE"
    And all databases exist in cluster "test_cluster"
    And s3 has bucket for cluster
    And cluster has no pending changes

  Scenario: Create external dictionary with complex key in ClickHouse cluster gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to create external dictionary in "ClickHouse" cluster
    """
    external_dictionary:
      name: test_dict
      layout:
        type: FLAT
      http_source:
        url: https://localhost:8443/?query=select%201%2C%27test%27
        format: TSV
      fixedLifetime: 300
      structure:
        id:
          name: id
        attributes:
          - name: text
            type: String
            null_value: ""
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    Then following query return "test" on all hosts
    """
    SELECT dictGetString('test_dict', 'text', toUInt64(1));
    """

  Scenario: Modify External dictionary in ClickHouse cluster gRPC
    Given cluster "test_cluster" is up and running
    And s3 object "new_model.bin"
    When we attempt to update external dictionary in "ClickHouse" cluster
    """
    external_dictionary:
      name: test_dict
      layout:
        type: COMPLEX_KEY_HASHED
      http_source:
        url: https://localhost:8443/?query=select%201%2C1%2C%27test%27
        format: TSV
      fixedLifetime: 300
      structure:
        key:
          attributes:
            - name: key1
              type: UInt8
            - name: key2
              type: UInt8
        attributes:
          - name: text
            type: String
            null_value: ""
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    Then following query return "test" on all hosts
    """
    SELECT dictGetString('test_dict', 'text', (1, 1));
    """

  Scenario: Delete External dictionary in ClickHouse cluster gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to delete external dictionary in "ClickHouse" cluster
    """
    external_dictionary_name: test_dict
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  Scenario: Create ML model in ClickHouse cluster gRPC
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

  Scenario: Modify ML model in ClickHouse cluster gRPC
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

  Scenario: Delete ML model in ClickHouse cluster gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to delete ML model in "ClickHouse" cluster
    """
    ml_model_name: test_model
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  Scenario: Create format schema in ClickHouse cluster gRPC
    Given cluster "test_cluster" is up and running
    And s3 object "schema.proto"
    When we attempt to create format schema in "ClickHouse" cluster
    """
    format_schema_name: test_schema
    type: FORMAT_SCHEMA_TYPE_PROTOBUF
    uri: '{{ s3_object_url }}'
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  Scenario: Modify format schema in ClickHouse cluster gRPC
    Given cluster "test_cluster" is up and running
    And s3 object "new_schema.proto"
    When we attempt to update format schema in "ClickHouse" cluster
    """
    format_schema_name: test_schema
    uri: '{{ s3_object_url }}'
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  Scenario: Delete format schema in ClickHouse cluster gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to delete format schema in "ClickHouse" cluster
    """
    format_schema_name: test_schema
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

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
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters
    """
    user: petya
    password: password-password-password-password
    """

  @users
  Scenario: Change password of ClickHouse user gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to update user in "ClickHouse" cluster
    """
    user_name: petya
    password: password_changed-password_changed-password_changed
    update_mask:
      paths:
        - password
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters
    """
    user: petya
    password: password_changed-password_changed-password_changed
    """

  @users
  Scenario: Remove ClickHouse user gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to delete user in "ClickHouse" cluster
    """
    user_name: petya
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And we get users in cluster
    And we are unable to log in to "test_cluster" with following parameters
    """
    user: petya
    password: password_changed-password_changed-password_changed
    """

  @databases
  Scenario: Add ClickHouse database to cluster gRPC
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
  Scenario: Grant permissions to created database gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to grant permission to user in "ClickHouse" cluster
    """
    user_name: test_user
    permission:
      database_name: testdb_4
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  @databases
  Scenario: Remove ClickHouse database from cluster gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to delete database in "ClickHouse" cluster "test_cluster" with data [gRPC]
    """
    database_name: testdb_4
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  @hosts
  Scenario: Add ClickHouse host to cluster gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to add hosts in "ClickHouse" cluster
    """
    host_specs:
      - zone_id: sas
        type: CLICKHOUSE
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And following query return "3" on all hosts
    """
    SELECT count() FROM (SELECT host_name FROM system.clusters)
    """

  @hosts
  Scenario: Remove ClickHouse host from cluster gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to delete "CLICKHOUSE" host in "sas" from "ClickHouse" cluster
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And following query return "2" on all hosts
    """
    SELECT count() FROM (SELECT host_name FROM system.clusters)
    """

  @backups
  Scenario: Create ClickHouse backup gRPC
    Given cluster "test_cluster" is up and running
    When we get ClickHouse backups for "test_cluster" [gRPC]
    Then we remember current response as "initial_backups"
    When we execute the following query on all hosts
    """
    CREATE TABLE testdb.logs (
        date     Date,
        message  text,
        id       Int64
    )
    ENGINE ReplicatedMergeTree('/tables/logs', '{replica}')
    PARTITION BY date
    ORDER BY id;
    """
    And we execute the following query on one host
    """
    INSERT INTO testdb.logs (date, message, id)
    VALUES (now(), 'Test', 1),
           (now(), 'Important!', 2),
           (now(), 'Urgnet!!!', 3);
    """
    And we attempt to backup "ClickHouse" cluster "test_cluster" [gRPC]
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And we get ClickHouse backups for "test_cluster" [gRPC]
    And new backups amount is larger than in "initial_backups"
    And we remember current response as "backups"
    And following query on all hosts succeeds
    """
    DROP TABLE testdb.logs NO DELAY;
    """

  @backups
  Scenario: Restore cluster from backup gRPC
    Given cluster "test_cluster" is up and running
    When we restore ClickHouse using latest "backups" and config [gRPC]
    """
    name: test_cluster_at_last_backup
    environment: PRESTABLE
    hostSpecs:
      - zoneId: man
        type: CLICKHOUSE
      - zoneId: vla
        type: CLICKHOUSE
    configSpec:
      clickhouse:
        resources:
          resourcePresetId: db1.micro
          diskSize: 20000000000
          diskTypeId: local-ssd
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "20 minutes"
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

  @backups
  Scenario: Remove restored cluster gRPC
    Given cluster "test_cluster_at_last_backup" is up and running
    When we attempt to delete "ClickHouse" cluster "test_cluster_at_last_backup" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And we are unable to find cluster "test_cluster_at_last_backup"

  @shards
  Scenario: Add shard gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to add shard in "ClickHouse" cluster
    """
    shardName: shard2
    hostSpecs:
      - zoneId: sas
        type: CLICKHOUSE
      - zoneId: vla
        type: CLICKHOUSE
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And following query return "2" on all hosts
    """
    SELECT count() FROM (SELECT DISTINCT shard_num FROM system.clusters)
    """
    And following query on one host succeeds
    """
    CREATE TABLE testdb.rmtree2 ON CLUSTER '{cluster}' (
        date  Date,
        text  text,
        id    Int64
    )
    ENGINE ReplicatedMergeTree('/tables/rmtree2', '{replica}')
    PARTITION BY date
    ORDER BY id;
    """
    And following query return "1" on all hosts
    """
    SELECT count() FROM system.tables WHERE database = 'testdb' AND name = 'rmtree2';
    """

  @shards
  Scenario: Create backup of sharded cluster gRPC
    Given cluster "test_cluster" is up and running
    When we get ClickHouse backups for "test_cluster" [gRPC]
    Then we remember current response as "initial_backups"
    When we execute the following query on all hosts of "shard1"
    """
    CREATE TABLE testdb.logs (
        date     Date,
        message  text,
        id       Int64
    )
    ENGINE ReplicatedMergeTree('/tables/logs2', '{replica}')
    PARTITION BY date
    ORDER BY id;
    """
    And we execute the following query on one host of "shard1"
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
    ENGINE ReplicatedMergeTree('/tables/logs2', '{replica}')
    PARTITION BY date
    ORDER BY id;
    """
    And we execute the following query on one host of "shard2"
    """
    INSERT INTO testdb.logs2 (date, message, id)
    VALUES (now(), 'Test', 1),
           (now(), 'Important!', 2),
           (now(), 'Urgnet!!!', 3);
    """
    And we attempt to backup "ClickHouse" cluster "test_cluster" [gRPC]
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And we get ClickHouse backups for "test_cluster" [gRPC]
    And new backups amount is larger than in "initial_backups"
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
  Scenario: Restore shard from backup gRPC
    Given cluster "test_cluster" is up and running
    When we restore ClickHouse using latest "backups" containing "shard2" and config [gRPC]
    """
    name: test_cluster_at_last_backup
    environment: PRESTABLE
    hostSpecs:
      - zoneId: man
        type: CLICKHOUSE
      - zoneId: vla
        type: CLICKHOUSE
    configSpec:
      clickhouse:
        resources:
          resourcePresetId: db1.micro
          diskTypeId: local-ssd
          diskSize: 10737418240
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "20 minutes"
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
  Scenario: Remove cluster with restored shard gRPC
    Given cluster "test_cluster_at_last_backup" is up and running
    When we attempt to delete "ClickHouse" cluster "test_cluster_at_last_backup" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And we are unable to find cluster "test_cluster_at_last_backup"

  @shards
  Scenario: Restore sharded cluster from backup gRPC
    Given cluster "test_cluster" is up and running
    When we restore ClickHouse shards [shard1, shard2] with config [gRPC]
    """
    name: test_restore_sharded_cluster
    environment: PRESTABLE
    hostSpecs:
      - zoneId: man
        type: CLICKHOUSE
      - zoneId: vla
        type: CLICKHOUSE
    configSpec:
      clickhouse:
        resources:
          resourcePresetId: db1.micro
          diskTypeId: local-ssd
          diskSize: 10737418240
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "40 minutes"
    And we get ClickHouse backups for "test_restore_sharded_cluster"
    And response should have status 200
    And message should have empty "backups" list
    And we get cluster "test_restore_sharded_cluster"
    And cluster has no pending changes
    And following query return "2" on all hosts
    """
    SELECT uniq(shard_num) FROM system.clusters
    """

  @shards
  Scenario: Remove cluster with restored shards gRPC
    Given cluster "test_restore_sharded_cluster" is up and running
    When we attempt to delete "ClickHouse" cluster "test_restore_sharded_cluster" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And we are unable to find cluster "test_restore_sharded_cluster"

  @shards
  Scenario: Add ClickHouse host to shard gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to add hosts in "ClickHouse" cluster
    """
    host_specs:
      - zone_id: sas
        type: CLICKHOUSE
        shard_name: shard2
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes

  @shards
  Scenario: Remove shard gRPC
    Given cluster "test_cluster" is up and running
    When we execute the following query on one host of "shard2"
    """
    CREATE TABLE testdb.test_table ON CLUSTER '{cluster}'
        (key UInt32, value UInt32)
        ENGINE ReplicatedMergeTree('/ch/tables/{shard}-test_table', '{replica}')
        ORDER BY key
    """
    Then ZooKeeper has ClickHouse node "/ch/tables/shard2-test_table"
    When we attempt to delete shard in "ClickHouse" cluster with data [gRPC]
    """
        shard_name: shard2
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And ZooKeeper has not ClickHouse node "/ch/tables/shard2-test_table"

  @zookeeper
  Scenario Outline: Adding ZooKeeper hosts to cluster gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to add hosts in "ClickHouse" cluster
    """
    host_specs:
      - zone_id: <zone_id>
        type: ZOOKEEPER
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And ZooKeeper consists of following nodes
    """
    <nodes>
    """
    And ClickHouse is aware of ZooKeeper configuration
    And ClickHouse can access ZooKeeper

    Examples:
      | zone_id | nodes                     |
      | sas     | [vla, man, iva, sas]      |
      | myt     | [vla, man, iva, sas, myt] |

  @zookeeper
  Scenario Outline: Deleting ZooKeeper hosts from cluster gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to delete "ZOOKEEPER" host in "<zone_id>" from "ClickHouse" cluster
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And ZooKeeper consists of following nodes
    """
    <nodes>
    """
    And ClickHouse is aware of ZooKeeper configuration
    And ClickHouse can access ZooKeeper

    Examples:
      | zone_id | nodes                |
      | sas     | [vla, man, iva, myt] |
      | myt     | [vla, man, iva]      |

  @zookeeper
  Scenario Outline: Completely update ZooKeeper configuration gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to add hosts in "ClickHouse" cluster
    """
    host_specs:
      - zone_id: <zone_id_add>
        type: ZOOKEEPER
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And ClickHouse can access ZooKeeper

    When we attempt to delete "ZOOKEEPER" host in "<zone_id_remove>" from "ClickHouse" cluster
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And cluster "test_cluster" is up and running
    And ClickHouse can access ZooKeeper

    Examples:
      | zone_id_add | zone_id_remove |
      | myt         | vla            |
      | vla         | man            |
      | man         | iva            |

  Scenario: Remove ClickHouse cluster gRPC
    Given cluster "test_cluster" is up and running
    When we attempt to delete "ClickHouse" cluster "test_cluster" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "6 minutes"
    And in worker_queue exists "clickhouse_cluster_delete_metadata" task
    And this task is done
    And autogenerated shard was deleted from Solomon
    And we are unable to find cluster "test_cluster"
    But s3 has bucket for cluster
    And in worker_queue exists "clickhouse_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster
