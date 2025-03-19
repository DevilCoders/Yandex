@dependent-scenarios
Feature: Clickhouse Cluster TLS

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And we are working with standard ClickHouse cluster

  @setup
  Scenario: Create ClickHouse cluster
    When we try to create cluster "test_cluster"
    """
    configSpec:
      version: "{{ versions[-1] }}"
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
    And ACL is "on"
    And cluster has no pending changes

  Scenario: Turn TLS+ACL off
    Given cluster "test_cluster" is up and running
    And TLS disabled in cluster "test_cluster"
    When we run highstate on all hosts in cluster
    And we restart zookeeper on all hosts
    And we restart clickhouse on all hosts
    Then cluster "test_cluster" is up and running
    And ACL is "off"

  Scenario: Execute maintenance task for ACL
    Given cluster "test_cluster" is up and running
    And ACL enabled in cluster "test_cluster"
    When we create maintenance task for ClickHouse cluster "test_cluster"
    """
    deploy_zk_tls: true
    """
    Then generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And ACL is "on"
