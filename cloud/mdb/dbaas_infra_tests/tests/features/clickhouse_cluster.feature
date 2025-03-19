@dependent-scenarios
Feature: Internal API Clickhouse Cluster lifecycle

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard ClickHouse cluster

  @setup
  Scenario: Create ClickHouse cluster
    When we try to create cluster "test_cluster"
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
    Then response should have status 200
    And generated task is finished within "15 minutes"
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

  Scenario: Change ClickHouse cluster options with restart
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        config:
          graphiteRollup:
            - name: test
              patterns:
                - function: any
                  retention:
                    - age: 60
                      precision: 1
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And following query on one host succeeds
    """
    CREATE TABLE testdb.graphite
    (
        Path String,
        Value Float64,
        Time UInt32,
        Date Date,
        Timestamp UInt32
    )
    ENGINE GraphiteMergeTree('test')
    PARTITION BY Date
    ORDER BY (Path, Time);
    """

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

  Scenario: Create ReplicatedMergeTree table in ClickHouse cluster
    Given cluster "test_cluster" is up and running
    And ClickHouse client options
    """
    insert_quorum: 2
    """
    When we execute the following query on all hosts
    """
    CREATE TABLE testdb.rmtree (
        date  Date,
        text  text,
        id    Int64
    )
    ENGINE ReplicatedMergeTree('/tables/rmtree', '{replica}')
    PARTITION BY date
    ORDER BY id;
    """
    And we execute the following query on one host
    """
    INSERT INTO testdb.rmtree VALUES(today(), 'test', 1);
    """
    Then following query return "1" on all hosts
    """
    SELECT count() FROM testdb.rmtree;
    """

  Scenario: Create external dictionary with complex key in ClickHouse cluster
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        config:
          dictionaries:
            - name: test_dict
              layout:
                type: COMPLEX_KEY_HASHED
              httpSource:
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
                    nullValue: ""
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    Then following query return "test" on all hosts
    """
    SELECT dictGetString('test_dict', 'text', (1, 1));
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

  Scenario: Restore removed MDB tables
    Given cluster "test_cluster" is up and running
    Then following query return "1" on all hosts
    """
    SELECT count() FROM system.tables
      WHERE database='_system' AND name='write_sli_part';
    """
    When we remove table "write_sli_part" in database "_system" on all hosts
    Then following query return "0" on all hosts
    """
    SELECT count() FROM system.tables
      WHERE database='_system' AND name='write_sli_part';
    """
    When we run highstate on all hosts in cluster
    Then following query return "1" on all hosts
    """
    SELECT count() FROM system.tables
      WHERE database='_system' AND name='write_sli_part';
    """
    And cluster has no pending changes
    When we dirty remove table "write_sli_part" in database "_system" on all hosts
    Then following query return "0" on all hosts
    """
    SELECT count() FROM system.tables
      WHERE database='_system' AND name='write_sli_part';
    """
    When we run highstate on all hosts in cluster
    Then following query return "1" on all hosts
    """
    SELECT count() FROM system.tables
      WHERE database='_system' AND name='write_sli_part';
    """
    And cluster has no pending changes

  @databases
  Scenario: Add ClickHouse database to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add database in cluster
    """
    databaseSpec:
      name: testdb_3
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  @databases
  Scenario: Remove ClickHouse database from cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove database "testdb_3" in cluster
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  @backups
  Scenario: ClickHouse backups
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
  Scenario: Restore cluster from backup
    Given cluster "test_cluster" is up and running
    When we restore ClickHouse using latest "backups" and config
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
    Then response should have status 200
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
  Scenario: Remove restored cluster
    Given cluster "test_cluster_at_last_backup" is up and running
    When we attempt to remove cluster "test_cluster_at_last_backup"
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And we are unable to find cluster "test_cluster_at_last_backup"

  @backups
  Scenario: Restore cluster from backup to earlier version
    Given cluster "test_cluster" is up and running
    When we restore ClickHouse using latest "backups" and config
    """
    name: test_cluster_at_last_backup_to_earlier_version
    environment: PRESTABLE
    hostSpecs:
      - zoneId: man
        type: CLICKHOUSE
      - zoneId: vla
        type: CLICKHOUSE
    configSpec:
      version: "{{ versions[-3] }}"
      clickhouse:
        resources:
          resourcePresetId: db1.micro
          diskSize: 20000000000
          diskTypeId: local-ssd
    """
    Then response should have status 422 and body contains "Restoring on earlier version is deprecated"

  @shards
  Scenario: Add shard
    Given cluster "test_cluster" is up and running
    When we attempt to add shard in cluster
    """
    shardName: shard2
    hostSpecs:
      - zoneId: sas
        type: CLICKHOUSE
      - zoneId: vla
        type: CLICKHOUSE
    """
    Then response should have status 200
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
  Scenario: Restore shard from backup
    Given cluster "test_cluster" is up and running
    When we restore ClickHouse using latest "backups" containing "shard2" and config
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
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And we get cluster "test_cluster_at_last_backup"
    And following query return "2" on all hosts
    """
    SELECT COUNT(*)
      FROM testdb.logs2
     WHERE message IN ('Important!', 'Urgnet!!!');
    """
    And we get ClickHouse backups for "test_cluster_at_last_backup"
    And response should have status 200
    And message should have empty "backups" list
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
          resourcePresetId: db1.small
          diskSize: 21474836480
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes

  @shards
  Scenario: Remove shard
    Given cluster "test_cluster" is up and running
    When we execute the following query on one host of "shard2"
    """
    CREATE TABLE testdb.test_table ON CLUSTER '{cluster}'
        (key UInt32, value UInt32)
        ENGINE ReplicatedMergeTree('/ch/tables/{shard}-test_table', '{replica}')
        ORDER BY key
    """
    Then ZooKeeper has ClickHouse node "/ch/tables/shard2-test_table"
    When we attempt to remove shard "shard2" in cluster
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And ZooKeeper has not ClickHouse node "/ch/tables/shard2-test_table"

#  TODO: uncomment and switch to worker tasks for host resetup, https://st.yandex-team.ru/MDB-17369
#  @resetup
#  Scenario: ClickHouse resetup when ZK metadata exist
#    Given cluster "test_cluster" is up and running
#    When we execute the following query on all hosts
#    """
#    CREATE TABLE testdb.logs (
#        date     Date,
#        message  text,
#        id       Int64
#    ) engine=ReplicatedMergeTree('/clickhouse/tables/{shard}/logs', '{replica}', date, id, 8192);
#    """
#    And we execute the following query on all hosts
#    """
#    INSERT INTO testdb.logs (date, message, id)
#    VALUES (now(), 'Test', 1),
#           (now(), 'Important!', 2),
#           (now(), 'Urgnet!!!', 3);
#    """
#    When we dirty remove all databases on hosts from zone "man"
#    Then hosts from zone "man" are dead
#    When we run highstate on all hosts from zone "man"
#    Then cluster "test_cluster" is up and running
#    Then following query return "3" on all hosts
#    """
#      SELECT count(*) FROM testdb.logs;
#    """
#
#  @resetup
#  Scenario: ClickHouse resetup when ZK metadata doesn't exist
#    Given cluster "test_cluster" is up and running
#    When we dirty remove all databases on hosts from zone "man"
#    And we remove ZK metadata for hosts from zone "man"
#    Then hosts from zone "man" are dead
#    When we run highstate on all hosts from zone "man"
#    Then cluster "test_cluster" is up and running
#    Then following query return "3" on all hosts
#    """
#      SELECT count(*) FROM testdb.logs;
#    """
#    And following query on all hosts succeeds
#    """
#      DROP TABLE testdb.logs NO DELAY;
#    """

  Scenario: Rename ClickHouse cluster
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    name: test_cluster_renamed
    """
    Then response should have status 200
    And generated task is finished within "2 minutes"
    And generated task has description "Update ClickHouse cluster metadata"
    And cluster has no pending changes
    When we execute command on host in geo "man"
    """
    grep -q "test_cluster_renamed" /etc/dbaas.conf
    """
    Then command retcode should be "0"

  Scenario: Remove ClickHouse cluster
    Given cluster "test_cluster_renamed" is up and running
    When we attempt to remove cluster "test_cluster_renamed"
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And in worker_queue exists "clickhouse_cluster_delete_metadata" task
    And this task is done
    And autogenerated shard was deleted from Solomon
    And we are unable to find cluster "test_cluster_renamed"
    But s3 has bucket for cluster
    And in worker_queue exists "clickhouse_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster

