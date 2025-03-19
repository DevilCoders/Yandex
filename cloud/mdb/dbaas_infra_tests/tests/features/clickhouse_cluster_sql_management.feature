@dependent-scenarios
Feature: Internal API Clickhouse Cluster with SQL Management

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

  @setup @sql_management
  Scenario: Create ClickHouse cluster with sql user and database management
    When we try to create cluster "test_sql_management"
    """
    configSpec:
     version: "{{ versions[-1] }}"
     sqlUserManagement: true
     sqlDatabaseManagement: true
     adminPassword: admin_password

    databaseSpecs: []
    userSpecs: []

    hostSpecs:
      - zoneId: vla
        type: CLICKHOUSE

      - zoneId: vla
        type: ZOOKEEPER
      - zoneId: man
        type: ZOOKEEPER
      - zoneId: iva
        type: ZOOKEEPER
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And cluster "test_sql_management" is up and running
    """
    user: admin
    password: admin_password
    """
    And cluster environment is "PRESTABLE"
    And s3 has bucket for cluster
    And cluster has no pending changes

  @sql_management
  Scenario: Create users and databases with sql queries
    Given cluster "test_sql_management" is up and running
    """
    user: admin
    password: admin_password
    """
    When we execute the following query on all hosts with credentials "admin" "admin_password"
    """
    CREATE USER test_user IDENTIFIED WITH plaintext_password BY 'password';
    """
    And we execute the following query on all hosts with credentials "admin" "admin_password"
    """
    GRANT CREATE ON test_database.* TO test_user;
    """
    And we execute the following query on all hosts with credentials "admin" "admin_password"
    """
    GRANT DROP ON test_database.* TO test_user;
    """
    And we execute the following query on all hosts with credentials "admin" "admin_password"
    """
    GRANT INSERT ON test_database.* TO test_user;
    """
    And we execute the following query on all hosts with credentials "admin" "admin_password"
    """
    GRANT SELECT ON *.* TO test_user;
    """
    And we execute the following query on all hosts with credentials "test_user" "password"
    """
    CREATE DATABASE test_database
    """
    And we execute the following query on all hosts with credentials "test_user" "password"
    """
    CREATE TABLE test_database.test_table (
        EventDate DateTime,
        CounterID UInt32,
        UserID UInt32
    )
    ENGINE MergeTree
    PARTITION BY toYYYYMM(EventDate)
    ORDER BY (CounterID, EventDate, intHash32(UserID))
    SAMPLE BY intHash32(UserID);
    """
    And we execute the following query on all hosts with credentials "test_user" "password"
    """
    INSERT INTO test_database.test_table SELECT now(), number, rand() FROM system.numbers LIMIT 10
    """

  @hosts @sql_management
  Scenario: Copy access objects and schema on new hosts
    Given cluster "test_sql_management" is up and running
    """
    user: admin
    password: admin_password
    """
    When we attempt to add hosts in "ClickHouse" cluster
    """
    host_specs:
      - zone_id: man
        type: CLICKHOUSE
    copy_schema: true
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes"
    And cluster "test_sql_management" is up and running
    """
    user: admin
    password: admin_password
    """
    And cluster has no pending changes
    When we execute the following query on all hosts with credentials "test_user" "password"
    """
    SELECT COUNT(*) FROM test_database.test_table
    """

  @shards @sql_management
  Scenario: Copy access and schema on new shard.
    Given cluster "test_sql_management" is up and running
    """
    user: admin
    password: admin_password
    """
    When we attempt to add shard in cluster
    """
    shardName: shard2
    hostSpecs:
      - zoneId: sas
        type: CLICKHOUSE
      - zoneId: vla
        type: CLICKHOUSE
    copySchema: true
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_sql_management" is up and running
    """
    user: admin
    password: admin_password
    """
    And cluster has no pending changes
    When we execute the following query on all hosts with credentials "test_user" "password"
    """
    SELECT COUNT(*) FROM test_database.test_table
    """
    And we execute the following query on all hosts with credentials "test_user" "password"
    """
    DROP DATABASE test_database;
    """
    And we attempt to remove cluster "test_sql_management"
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And in worker_queue exists "clickhouse_cluster_delete_metadata" task
    And this task is done
