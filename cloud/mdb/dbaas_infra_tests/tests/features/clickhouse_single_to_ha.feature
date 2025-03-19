@dependent-scenarios
Feature: Internal API Clickhouse single instance Cluster updated to HA Cluster

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
    hostSpecs:
      - zoneId: vla
        type: CLICKHOUSE
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster environment is "PRESTABLE"
    And all databases exist in cluster "test_cluster"
    And s3 has bucket for cluster
    And cluster has no pending changes

  @shards
  Scenario: Add shard
    Given cluster "test_cluster" is up and running
    When we attempt to add shard in cluster
    """
    shardName: shard2
    hostSpecs:
      - zoneId: sas
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

  @shards
  Scenario: Add shard group
    Given cluster "test_cluster" is up and running
    When we attempt to create shard group in "ClickHouse" cluster
    """
    shard_group_name: "test_shard_group"
    description: "Test shard group"
    shard_names: ["shard1", "shard2"]
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes"
    And following query return "{{ cluster['id'] }}\ntest_shard_group" on all hosts
    """
    SELECT DISTINCT cluster FROM system.clusters ORDER BY cluster
    """
    When we attempt to delete shard group in "ClickHouse" cluster
    """
    shard_group_name: "test_shard_group"
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes"
    And following query return "{{ cluster['id'] }}" on all hosts
    """
    SELECT DISTINCT cluster FROM system.clusters ORDER BY cluster
    """

  @shards
  Scenario: Adding ZooKeeper hosts to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add ZooKeeper to cluster
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And ClickHouse is aware of ZooKeeper configuration
    And ClickHouse can access ZooKeeper

  @shards
  Scenario: Remove shard with one node
    Given cluster "test_cluster" is up and running
    When we execute the following query on all hosts
    """
    CREATE TABLE testdb.test_table
        (key UInt32, value UInt32)
        ENGINE ReplicatedMergeTree('/ch/tables/{shard}-test_table', '{replica}')
        ORDER BY key
    """
    Then ZooKeeper has ClickHouse node "/ch/tables/shard2-test_table"
    When we attempt to remove shard "shard2" in cluster
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And ZooKeeper has not ClickHouse node "/ch/tables/shard2-test_table"

