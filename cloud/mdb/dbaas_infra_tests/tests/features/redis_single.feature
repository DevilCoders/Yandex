@dependent-scenarios
Feature: Internal API Redis Single lifecycle

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
    And we are working with single_instance Redis cluster

  @setup
  Scenario: Redis single instance creation works
    When we enable feature flag "MDB_REDIS_ALLOW_DEPRECATED_5"
    And we try to create cluster "test_redis"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And we are able to log in to "test_redis" with following parameters
    """
    password: passw0rd
    """
    And s3 has bucket for cluster
    And cluster has no pending changes
    And redis major version is "5.0"

  Scenario: Redis single instance task reject works
    Given cluster "test_redis" is up and running
    When we lock cluster
    And we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      redisConfig_5_0:
        password: rejected_password
    """
    Then response should have status 200
    And generated task is failed within "5 minutes"
    When we unlock cluster
    Then cluster has no pending changes

  Scenario: Scale Redis instance volume size up
    Given cluster "test_redis" is up and running
    When we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      resources:
        diskSize: 21474836480
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes

  Scenario: Scale Redis instance volume size down
    Given cluster "test_redis" is up and running
    When we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      resources:
        diskSize: 17179869184
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes

  Scenario: Scale Redis instance up
    Given cluster "test_redis" is up and running
    When we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      resources:
        resourcePresetId: db1.micro
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes

  Scenario: Scale Redis instance down
    Given cluster "test_redis" is up and running
    When we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      resources:
        resourcePresetId: db1.nano
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes

  Scenario: Update Redis instance password
    Given cluster "test_redis" is up and running
    When we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      redisConfig_5_0:
        password: new_passw0rd
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_redis" with following parameters
    """
    password: new_passw0rd
    """

  Scenario: Update Redis instance config
    Given cluster "test_redis" is up and running
    When we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      redisConfig_5_0:
        timeout: 10
        maxmemoryPolicy: VOLATILE_LRU
        slowlogLogSlowerThan: 30000
        slowlogMaxLen: 4000
        notifyKeyspaceEvents: Elgt
        databases: 10
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_redis" with following parameters
    """
    password: new_passw0rd
    """

  @hosts
  Scenario: Add Redis host to cluster
    Given cluster "test_redis" is up and running
    When we attempt to add host in cluster
    """
    hostSpecs:
      - zoneId: myt
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster "test_redis" is up and running
    And cluster has no pending changes
    And following query on one host in "man" succeeds
    """
    INFO replication
    """
    And query result is like
    """
    - connected_slaves:1
    """

  @failover
  Scenario: Redis failover works
    Given cluster "test_redis" is up and running
    When we choose one redis master and remember it
    And we attempt to failover redis cluster "test_redis" with saved master
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And we wait for "30 seconds"
    And redis master should be different from saved

  @hosts
  Scenario: Remove Redis host from cluster
    Given cluster "test_redis" is up and running
    When we attempt to remove host in geo "man" from "test_redis"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster "test_redis" is up and running
    And cluster has no pending changes
    And following query on master host succeeds
    """
    INFO replication
    """
    And query result is like
    """
    - connected_slaves:0
    """

  @backups
  Scenario: Redis backups
    Given cluster "test_redis" is up and running
    When we get Redis backups for "test_redis"
    Then response should have status 200
    When we remember current response as "initial_backups"
    And we create backup for Redis "test_redis"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    When we get Redis backups for "test_redis"
    Then response should have status 200
    And message with "backups" list is larger than in "initial_backups"
    When we remember current response as "backups"

  Scenario: Upgrade Redis single instance to 6.0
    Given we are able to log in to "test_redis" with following parameters
    """
    password: new_passw0rd
    """
    When we enable feature flag "MDB_REDIS_ALLOW_DEPRECATED_6"
    And we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      version: "6.0"
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes
    And redis major version is "6.0"
    And following query on one host succeeds
    """
    INFO server
    """

  Scenario: Remove Redis single instance
    Given we are able to log in to "test_redis" with following parameters
    """
    password: new_passw0rd
    """
    When we attempt to remove cluster "test_redis"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And in worker_queue exists "redis_cluster_delete_metadata" task
    And this task is done
    And autogenerated shard was deleted from Solomon
    And we are unable to find cluster "test_redis"
