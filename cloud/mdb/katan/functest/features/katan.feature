@katan
Feature: katan works

  Background: Empty databases
    Given empty databases
    And "2" postgresql clusters in metadb
    And mdb-katan-imp import them

  @happy
  Scenario: Happy path - all green
    Given all deploy shipments are "DONE"
    And all clusters in health are "Alive"
    When I add rollout for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
    And I execute mdb-katan
    Then my rollout is finished
    And katan.cluster_rollouts statistics is
    """
    succeeded: 2
    """
    And there are "6" katan.rollout_shipments

  Scenario: All clusters are dead
    . katan should not touch them
    Given all deploy shipments are "DONE"
    And all clusters in health are "Dead"
    When I add rollout for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
    And I execute mdb-katan
    Then my rollout is finished
    And katan.cluster_rollouts statistics is
    """
    skipped: 2
    """
    And there are "0" katan.rollout_shipments

  Scenario: All clusters are alive but shipments fails
    . katan should stop on first broken cluster
    Given all deploy shipments are "FAILED"
    And all clusters in health are "Alive"
    When I add rollout for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
    And I execute mdb-katan
    Then my rollout is finished
    And katan.cluster_rollouts statistics is
    """
    failed: 1
    skipped: 1
    """
    # 1 shipment + 2 its retries
    And there are "3" katan.rollout_shipments

  Scenario: All clusters are alive and all shipments are OK, but them break cluster
    . katan should stop on first broken cluster
    Given all deploy shipments are "DONE"
    And all clusters in health are "Alive" before rollout
    And all clusters in health are "Degraded" after rollout
    When I add rollout for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
    And I execute mdb-katan
    Then my rollout is finished
    And katan.cluster_rollouts statistics is
    """
    failed: 1
    skipped: 1
    """
    And there is one katan.rollout_shipments

  Scenario: All clusters belongs to personal salt-master
    . katan should skip them
    Given all deploy minions in "personal" group
    When I add rollout for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
    And I execute mdb-katan
    Then my rollout is finished
    And katan.cluster_rollouts statistics is
    """
    skipped: 2
    """
