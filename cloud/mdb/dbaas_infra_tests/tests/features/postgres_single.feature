@dependent-scenarios
Feature: Internal API PostgreSQL single instance Cluster lifecycle

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with single PostgreSQL cluster

  @setup
  Scenario: PostgreSQL cluster created successfully
    When we try to create cluster "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And we are able to log in to "test_cluster" with following parameters:
    """
    user: another_test_user
    password: mysupercooltestpassword11111
    """
    And s3 has bucket for cluster
    And cluster has no pending changes

  Scenario: PostgreSQL cluster task reject works
    Given cluster "test_cluster" is up and running
    When we lock cluster
    And we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      postgresqlConfig_14:
        sharedBuffers: 268435456
    """
    Then response should have status 200
    And generated task is failed within "15 minutes"
    When we unlock cluster
    Then cluster has no pending changes

  Scenario: Add PostgreSQL host to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add host in "test_cluster"
    """
    hostSpecs:
      - zoneId: sas
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And master in "test_cluster" has "1" replics
    And cluster has no pending changes

  Scenario: Add PostgreSQL cascade host to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add host in "vla" to "test_cluster" replication_source "sas"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And master in "test_cluster" has "1" replics
    # ensure that we have cascade replica streaming this replica
    And host in geo "sas" in "test_cluster" has "1" replics
    And cluster has no pending changes

  Scenario: Add PostgreSQL cascade host to cluster level 2
    Given cluster "test_cluster" is up and running
    When we attempt to add host in "iva" to "test_cluster" replication_source "vla"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And master in "test_cluster" has "1" replics
    And host in geo "vla" in "test_cluster" has "1" replics
    And cluster has no pending changes

  Scenario: Modify PostgreSQL host replication_source master
    Given cluster "test_cluster" is up and running
    When we attempt to modify host in "vla" in cluster "test_cluster" replication_source geo "man"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And master in "test_cluster" has "2" replics
    And cluster has no pending changes

  Scenario: Modify PostgreSQL host replication_source replica
    Given cluster "test_cluster" is up and running
    When we attempt to modify host in "vla" in cluster "test_cluster" replication_source geo "sas"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And master in "test_cluster" has "1" replics
    And host in geo "sas" in "test_cluster" has "1" replics
    And cluster has no pending changes

  Scenario: Modify PostgreSQL host config recoveryMinApplyDelay
    Given cluster "test_cluster" is up and running
    When we attempt to modify user "test_user" in "test_cluster"
    """
    grants:
     - mdb_admin
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    When we attempt to modify host in "vla" in cluster "test_cluster" with parameter "recoveryMinApplyDelay" and value "30000"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we wait for "60"
    And following SQL request on host in geo "sas" in "test_cluster" succeeds
    """
    SELECT replay_lag > interval '20 seconds' AS result FROM pg_stat_replication;
    """
    And query result is exactly
    """
    - result: True
    """

  Scenario: Change PostgreSQL host options
    Given cluster "test_cluster" is up and running
    When we attempt to modify host in "vla" in cluster "test_cluster" with parameter "logMinDurationStatement" and value "666"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we wait for "30"
    And following SQL request on host in geo "vla" in "test_cluster" succeeds
    """
    SELECT setting FROM pg_settings WHERE name = 'log_min_duration_statement'
    """
    And query result is like
    """
    - setting: '666'
    """

  Scenario: Modify PostgreSQL host reset replication_source
    Given cluster "test_cluster" is up and running
    When we attempt to modify host in "vla" in cluster "test_cluster" replication_source reset
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And master in "test_cluster" has "2" replics
    And cluster has no pending changes
    And following SQL request on host in geo "man" in "test_cluster" succeeds
    """
    SELECT replay_lag > interval '20 seconds' AS result FROM pg_stat_replication WHERE client_hostname ~ 'vla';
    """
    And query result is exactly
    """
    - result: False
    """
    And following SQL request on host in geo "vla" in "test_cluster" succeeds
    """
    SELECT setting FROM pg_settings WHERE name = 'log_min_duration_statement'
    """
    And query result is like
    """
    - setting: '1000'
    """

  Scenario: Modify PostgreSQL HA host replication_source replica
    Given cluster "test_cluster" is up and running
    When we attempt to modify host in "vla" in cluster "test_cluster" replication_source geo "sas"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And master in "test_cluster" has "1" replics
    And host in geo "sas" in "test_cluster" has "1" replics
    And cluster has no pending changes

  Scenario: Modify PostgreSQL master host replication_source replica
    Given cluster "test_cluster" is up and running
    When we attempt to modify host in "man" in cluster "test_cluster" replication_source geo "vla"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And master in "test_cluster" has "1" replics
    And host in geo "vla" in "test_cluster" has "2" replics
    And cluster has no pending changes

  Scenario: Remove PostgreSQL host from cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove host in geo "iva" from "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And master in "test_cluster" has "1" replics
    And host in geo "vla" in "test_cluster" has "1" replics
    And cluster "test_cluster" is up and running
    And cluster has no pending changes

  Scenario: Adding PostgreSQL host with priority
    Given cluster "test_cluster" is up and running
    When we attempt to add host in "test_cluster"
    """
    hostSpecs:
      - zoneId: myt
        priority: 42
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And we get hosts in "test_cluster"
    And host in "myt" in cluster "test_cluster" has priority "42" in zookeeper
    And cluster has no pending changes

  Scenario: Modifying PostgreSQL host priority
    Given cluster "test_cluster" is up and running
    When we attempt to modify host in "myt" in cluster "test_cluster" with priority "43"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    When we attempt to get hosts in "test_cluster"
    Then host in "myt" in cluster "test_cluster" has priority "43" in zookeeper
    And cluster has no pending changes

  Scenario: Execute maintenance task for PostgreSQL
    Given cluster "test_cluster" is up and running
    When we create maintenance task for PostgreSQL cluster "test_cluster"
    """
    restart: true
    """
    Then generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
