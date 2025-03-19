@sher
Feature: katan scheduler works

  Background: Empty databases
    Given empty databases
    And "2" postgresql clusters in metadb
    And mdb-katan-imp import them

  Scenario: Scheduler create rollouts for schedule
    Given all deploy shipments are "DONE"
    And all clusters in health are "Alive"
    And katan.schedule with maxSize=10 for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
    When I execute mdb-katan-scheduler
    Then katan.cluster_rollouts statistics is
    """
    pending: 2
    """
    And my schedule in "active" state

  @broken
  Scenario: Scheduler mark schedule as broken
    . and don't start new rollouts
    . Workarounds:
    . - In metadb/api recipes we have quota "mdb.clusters.count"=2.
    .   Run scheduler with MaxRolloutFails=2
    Given all deploy shipments are "FAILED"
    And all clusters in health are "Alive"
    And katan.schedule with maxSize=1 for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
    When I execute mdb-katan-scheduler
    Then katan.cluster_rollouts statistics is
    """
    pending: 1
    """
    When I execute mdb-katan
    Then katan.cluster_rollouts statistics is
    """
    failed: 1
    """
    And my schedule in "active" state
    When I execute mdb-katan-scheduler
    And I execute mdb-katan
    Then katan.cluster_rollouts statistics is
    """
    failed: 2
    """
    When I execute mdb-katan-scheduler with MaxRolloutFails=2
    Then katan.cluster_rollouts statistics is
    """
    failed: 2
    """
    And my schedule in "broken" state

  @skipped
  Scenario: Scheduler doesn't retry skipped and failed clusters within still_age
    . I'll just check that it doesn't launch new rollouts
    . if all the clusters are already skipped
    Given all deploy shipments are "OK"
    And all clusters in health are "Degraded"
    And katan.schedule with maxSize=1 for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
    When I execute mdb-katan-scheduler
    And I execute mdb-katan
    Then katan.cluster_rollouts statistics is
    """
    skipped: 1
    """
    When I execute mdb-katan-scheduler
    And I execute mdb-katan
    Then katan.cluster_rollouts statistics is
    """
    skipped: 2
    """
    When I execute mdb-katan-scheduler
    Then katan.cluster_rollouts statistics is
    """
    skipped: 2
    """

  @broken
  Scenario: Scheduler mark schedule as broken, but count only examined rollouts
    Given all deploy shipments are "FAILED"
    And all clusters in health are "Alive"
    And katan.schedule with maxSize=1 for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
    When I execute mdb-katan-scheduler
    And I execute mdb-katan
    And I execute mdb-katan-scheduler with MaxRolloutFails=1
    Then my schedule in "broken" state
    When I mark my schedule as "active"
    And I execute mdb-katan-scheduler
    Then my schedule in "active" state

  @retries
  Scenario: Scheduler pass rollout options to rollouts
    Given all deploy shipments are "DONE" after "FAILED"
    And all clusters in health are "Alive"
    And katan.schedule with options={"shipment_retries": 2} for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
    When I execute mdb-katan-scheduler
    And I execute mdb-katan
    Then katan.cluster_rollouts statistics is
    """
    succeeded: 2
    """

  @MDB-14877
  Scenario: Scheduler doesn't add rollouts on clusters imported early then importCooldown
    Given katan.schedule with maxSize=10 for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
    When I execute mdb-katan-scheduler with ImportCooldown=24h
    Then there are no rows in katan.rollouts
