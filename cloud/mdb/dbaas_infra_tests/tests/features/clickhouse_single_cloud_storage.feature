@dependent-scenarios
Feature: Internal API Clickhouse single instance Cluster with Cloud Storade lifecycle

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

  @setup @cloud_storage
  Scenario: Create ClickHouse cluster with cloud storage
    When we try to create cluster "cloud_storage_cluster"
    """
    configSpec:
      version: "{{ versions[-1] }}"
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    When we attempt to modify cluster "cloud_storage_cluster" with following parameters
    """
    configSpec:
      cloudStorage:
        enabled: true
        moveFactor: 0.02
        dataCacheEnabled: true
        dataCacheMaxSize: 2147483648
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster "cloud_storage_cluster" is up and running
    And cluster environment is "PRESTABLE"
    And all databases exist in cluster "cloud_storage_cluster"
    And s3 has bucket for cluster
    And s3 has "cloud storage" bucket for cluster
    And cluster has no pending changes

  @cloud_storage
  Scenario: Check cloud storage settings
    Given cluster "cloud_storage_cluster" is up and running
    Then following query return "0.02" on all hosts
    """
    SELECT move_factor FROM system.storage_policies WHERE policy_name='default' AND volume_name='default'
    """
    And command succeeds on all hosts
    """
    grep '<data_cache_enabled>true</data_cache_enabled>' /etc/clickhouse-server/config.d/storage_policy.xml
    """
    And command succeeds on all hosts
    """
    grep '<data_cache_max_size>2147483648</data_cache_max_size>' /etc/clickhouse-server/config.d/storage_policy.xml
    """

  @cloud_storage
  Scenario: Move partitions to S3 disk on ClickHouse with cloud storage
    Given cluster "cloud_storage_cluster" is up and running
    When we execute the following query on all hosts
    """
    CREATE TABLE testdb.logs (
        dt     Date,
        message  text,
        id       Int64
    )
    ENGINE MergeTree
    PARTITION BY dt
    ORDER BY (dt, id);
    """
    And we execute the following query on all hosts
    """
    INSERT INTO testdb.logs (dt, message, id)
    VALUES ('2020-01-03', 'Test', 1),
           ('2020-01-04', 'Important!', 2),
           ('2020-01-05', 'Urgnet!!!', 3);
    """
    And we execute the following query on all hosts
    """
    ALTER TABLE testdb.logs MOVE PARTITION '2020-01-04' TO DISK 'object_storage'
    """
    Then following query return "1" on all hosts
    """
    SELECT count(*) FROM system.parts where database = 'testdb' and table = 'logs' and disk_name = 'object_storage';
    """
    And following query return "6" on all hosts
    """
    SELECT sum(id) FROM testdb.logs;
    """

  @backup @cloud_storage
  Scenario: ClickHouse backup with cloud storage
    Given cluster "cloud_storage_cluster" is up and running
    When we get ClickHouse backups for "cloud_storage_cluster"
    Then we remember current response as "initial_backups"
    And following query return "6" on all hosts
    """
    SELECT sum(id) FROM testdb.logs;
    """
    When we create backup for ClickHouse "cloud_storage_cluster"
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And we get ClickHouse backups for "cloud_storage_cluster"
    And response should have status 200
    And message with "backups" list is larger than in "initial_backups"
    And we remember current response as "backups"


  @backup @cloud_storage
  Scenario: Restore ClickHouse from backup with cloud storage
    Given cluster "cloud_storage_cluster" is up and running
    And version at least "22.3"
    When we restore ClickHouse using latest "backups" and config
    """
    name: cloud_storage_cluster_at_last_backup
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
      cloudStorage:
        enabled: true
    """
    Then response should have status 200
    And generated task is finished within "30 minutes"
    And we get ClickHouse backups for "cloud_storage_cluster_at_last_backup"
    And response should have status 200
    And message should have empty "backups" list
    And we get cluster "cloud_storage_cluster_at_last_backup"
    And following query return "6" on all hosts
    """
    SELECT sum(id) FROM testdb.logs;
    """
    And cluster has no pending changes


  @backup @cloud_storage
  Scenario: Remove restored cluster
    Given cluster "cloud_storage_cluster_at_last_backup" is up and running
    When we attempt to remove cluster "cloud_storage_cluster_at_last_backup"
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And we are unable to find cluster "cloud_storage_cluster_at_last_backup"


  @cloud_storage
  Scenario: Remove ClickHouse cluster
    Given cluster "cloud_storage_cluster" is up and running
    When we attempt to remove cluster "cloud_storage_cluster"
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And in worker_queue exists "clickhouse_cluster_delete_metadata" task
    And this task is done
