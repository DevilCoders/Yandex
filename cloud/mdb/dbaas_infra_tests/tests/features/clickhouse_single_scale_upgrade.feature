@dependent-scenarios
Feature: Internal API Clickhouse single instance Cluster lifecycle, scale and upgrade

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
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster environment is "PRESTABLE"
    And all databases exist in cluster "test_cluster"
    And s3 has bucket for cluster
    And cluster has no pending changes

  Scenario: Upgrade ClickHouse version
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      version: "{{ versions[-2] }}"
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And following query return "1" on all hosts
    """
    SELECT version() LIKE '{{ versions[-2] }}.%';
    """
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      version: "{{ versions[-1] }}"
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And following query return "1" on all hosts
    """
    SELECT version() LIKE '{{ versions[-1] }}.%';
    """
    And cluster has no pending changes

  Scenario: Scale ClickHouse cluster volume size up
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        resources:
          diskSize: 21474836480
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  Scenario: Scale ClickHouse cluster volume size down
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        resources:
          diskSize: 10737418240
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  Scenario: Scale ClickHouse cluster up
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        resources:
          resourcePresetId: db1.small
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes

  Scenario: Scale ClickHouse cluster down
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      clickhouse:
        resources:
          resourcePresetId: db1.micro
    """
    Then response should have status 200
    And generated task is finished within "6 minutes"
    And cluster has no pending changes
