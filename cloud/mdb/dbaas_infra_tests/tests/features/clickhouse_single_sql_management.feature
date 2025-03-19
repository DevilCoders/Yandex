@dependent-scenarios
Feature: Internal API Clickhouse single instance Cluster with SQL Management

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
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And cluster "test_sql_management" is up and running
    """
    user: admin
    password: admin_password
    """
    And cluster environment is "PRESTABLE"
    And cluster has no pending changes

  @sql_management
  Scenario: Change admin user password
    Given cluster "test_sql_management" is up and running
    """
    user: admin
    password: admin_password
    """
    When we attempt to modify cluster "test_sql_management" with following parameters
    """
    configSpec:
      adminPassword: new_admin_password
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
    And cluster "test_sql_management" is up and running
    """
    user: admin
    password: new_admin_password
    """

  @sql_management
  Scenario: Create users and databases with sql queries
    Given cluster "test_sql_management" is up and running
    """
    user: admin
    password: new_admin_password
    """
    When we execute the following query on all hosts with credentials "admin" "new_admin_password"
    """
    CREATE USER test_user IDENTIFIED WITH plaintext_password BY 'password';
    """
    And we execute the following query on all hosts with credentials "admin" "new_admin_password"
    """
    GRANT CREATE ON test_database.* TO test_user;
    """
    And we execute the following query on all hosts with credentials "admin" "new_admin_password"
    """
    GRANT INSERT ON test_database.* TO test_user;
    """
    And we execute the following query on all hosts with credentials "admin" "new_admin_password"
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

  @backups @sql_management
  Scenario: Create cluster backup with access management
    Given cluster "test_sql_management" is up and running
    """
    user: admin
    password: new_admin_password
    """
    When we create backup for ClickHouse "test_sql_management"
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And we get ClickHouse backups for "test_sql_management"
    And response should have status 200
    And we remember current response as "backups"
    When we attempt to remove cluster "test_sql_management"
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And in worker_queue exists "clickhouse_cluster_delete_metadata" task
    And this task is done

  @backups @sql_management
  Scenario: Restore cluster backup with access management
    When we restore ClickHouse using latest "backups" and config
    """
    name: test_sql_management_restored
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
    And generated task is finished within "20 minutes"
    And we get ClickHouse backups for "test_sql_management_restored"
    And response should have status 200
    And message should have empty "backups" list
    And we get cluster "test_sql_management_restored"
    When we execute on all hosts with credentials "test_user" "password" and result is "10"
    """
    SELECT COUNT(*) FROM test_database.test_table
    """

  @sql_management
  Scenario: Check admin grantees for version 21.4+
    Given cluster "test_sql_management_restored" is up and running
    """
    user: admin
    password: new_admin_password
    """
    And version at least "21.4"
    Then user "admin" created as "CREATE USER admin IDENTIFIED WITH sha256_password GRANTEES ANY EXCEPT admin"
    When we broke grantees for user "admin"
    Then user "admin" created as "CREATE USER admin IDENTIFIED WITH sha256_password GRANTEES NONE"
    When we run highstate on all hosts in cluster
    Then user "admin" created as "CREATE USER admin IDENTIFIED WITH sha256_password GRANTEES ANY EXCEPT admin"
    Then following query executed with credentials "admin" "new_admin_password" on all hosts fails with "user `admin` is not allowed as grantee"
    """
    REVOKE CREATE ON *.* FROM admin
    """

