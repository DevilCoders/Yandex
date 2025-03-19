@dependent-scenarios
Feature: Clickhouse Backup Service integration

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

  Scenario: ClickHouse usual backups
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


  Scenario: ClickHouse backup service enabled
    Given cluster "test_cluster" is up and running
    When we get ClickHouse backups for "test_cluster"
    Then response should have status 200
    And message should have not empty "backups" list
    When we remember current response as "initial_backups_storage"
    And we enable backup service
    And we import cluster backups to backup service with args "--dry-run=false --skip-sched-date-dups=true"

    When we get ClickHouse backups for "test_cluster"
    Then response should have status 200
    And message should have not empty "backups" list
    Then message with "backups" list length is equal to "initial_backups_storage"


  Scenario: ClickHouse backup service backups
    Given cluster "test_cluster" is up and running
    When we get ClickHouse backups for "test_cluster"
    Then response should have status 200
    And message should have not empty "backups" list
    When we remember current response as "initial_backups"
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
    VALUES (now(), 'New', 4),
           (now(), 'New!', 5),
           (now(), 'New!!!', 6);
    """
    And we create backup for ClickHouse "test_cluster"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    When we get ClickHouse backups for "test_cluster"
    Then response should have status 200
    And message with "backups" list is larger than in "initial_backups"
    When we remember current response as "backups"
    And we restore ClickHouse using latest "backups" and config
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
    And we get cluster "test_cluster_at_last_backup"
    And following query return "3" on all hosts
    """
    SELECT COUNT(*)
      FROM testdb.logs;
    """
    And cluster has no pending changes
