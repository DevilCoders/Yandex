@katan @zombies
Feature: katan handle zombie rollouts

  Background: Empty database
    Given empty databases

  Scenario: Katan handle zombie rollouts
    . A bit of a bloated setup since:
    . - there is no simple way to emulate unfinished rollout
    .   (Katan always tries to finish it).
    .   That is why we `start rollout` by hand.
    . - after cleanup,  Katan will wait 'forever' for the new rollout,
    .   That is why we need two rollouts.
    Given postgresql cluster in metadb
    And mdb-katan-imp import it
    And katan.schedule with maxSize=1 for
    """
    {
      "meta": {
        "type": "postgresql_cluster"
      }
    }
    """
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
    pending: 2
    """
    And I start one pending rollout
    Then katan.cluster_rollouts statistics is
    """
    pending: 1
    running: 1
    """
    And I execute mdb-katan
    Then katan.cluster_rollouts statistics is
    """
    cancelled: 1
    succeeded: 1
    """
