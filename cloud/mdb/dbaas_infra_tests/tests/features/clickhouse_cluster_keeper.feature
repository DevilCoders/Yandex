@dependent-scenarios
Feature: Clickhouse Keeper

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
    And feature flag "MDB_CLICKHOUSE_KEEPER" is set

  Scenario: Create ClickHouse cluster with enabled Keeper in HA mode
    When we try to create cluster "test_cluster"
    """
    configSpec:
      version: "{{ versions[-1] }}"
      embeddedKeeper: true
      clickhouse:
        resources:
          resourcePresetId: db1.micro
    hostSpecs:
      - zoneId: vla
        type: CLICKHOUSE
      - zoneId: man
        type: CLICKHOUSE
      - zoneId: sas
        type: CLICKHOUSE
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster environment is "PRESTABLE"
    And all databases exist in cluster "test_cluster"
    And s3 has bucket for cluster
    And cluster has no pending changes
    And ClickHouse can access ZooKeeper
    And ClickHouse Keeper is HA

  Scenario: Create ClickHouse cluster with enabled Keeper and 2 nodes
    When we try to create cluster "test_cluster"
    """
    configSpec:
      version: "{{ versions[-1] }}"
      embeddedKeeper: true
      clickhouse:
        resources:
          resourcePresetId: db1.micro
    hostSpecs:
      - zoneId: vla
        type: CLICKHOUSE
      - zoneId: man
        type: CLICKHOUSE
    """
    Then response should have status 422 and body contains "You cannot use 2-node setup with enabled Embedded Keeper"
