@dependent-scenarios
Feature: Internal API Redis 6 Cluster lifecycle

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
    And we are working with standard_6_tls Redis cluster

  @setup
  Scenario: Redis cluster creation works
    When we enable feature flag "MDB_REDIS_ALLOW_DEPRECATED_6"
    And we try to create cluster "test_redis"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And we are able to log in to "test_redis" with following parameters
    """
    password: passw0rd
    """
    And s3 has bucket for cluster
    And cluster has no pending changes
    And redis major version is "6.0"
    And tls mode is "on"

  Scenario: Redis version downgrade fails
    Given cluster "test_redis" is up and running
    When we enable feature flag "MDB_REDIS_ALLOW_DEPRECATED_6"
    And we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      version: "5.0"
    """
    Then response should have status 422
    And redis major version is "6.0"

  Scenario: Scale Redis cluster volume size up
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

  Scenario: Scale Redis cluster volume size down
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

  Scenario: Scale Redis cluster up
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

  Scenario: Scale Redis cluster down
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
    And following query on master host succeeds
    """
    INFO replication
    """
    And query result is like
    """
    - connected_slaves:3
    """

  @hosts
  Scenario: Modify Redis host in cluster
    Given cluster "test_redis" is up and running
    When we attempt to modify host in "myt" in cluster "test_redis" with priority "250"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster "test_redis" is up and running
    And cluster has no pending changes
    And config on host in geo "myt" contains the following line
    """
    replica-priority 250
    """


  @hosts
  Scenario: Remove Redis host from cluster
    Given cluster "test_redis" is up and running
    When we attempt to remove host in geo "myt" from "test_redis"
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
    - connected_slaves:2
    """

  Scenario: Update Redis cluster password
    Given cluster "test_redis" is up and running
    When we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      redisConfig_6_0:
        password: new_passw0rd
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_redis" with following parameters
    """
    password: new_passw0rd
    """
    And tls mode is "on"


  Scenario: Update Redis cluster config
    Given cluster "test_redis" is up and running
    When we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      redisConfig_6_0:
        timeout: 10
        maxmemoryPolicy: VOLATILE_LRU
        slowlogLogSlowerThan: 30000
        slowlogMaxLen: 4000
        notifyKeyspaceEvents: Elgtm
    """
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_redis" with following parameters
    """
    password: new_passw0rd
    """
    And tls mode is "on"

  @backups
  Scenario: Redis backups (tls mode on to off)
    Given cluster "test_redis" is up and running
    When we get Redis backups for "test_redis"
    Then response should have status 200
    When we remember current response as "initial_backups"
    And we execute the following query on master host
    """
    SET test-key secret-value
    """
    Then we wait for "5 seconds"
    When we create backup for Redis "test_redis"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    When we get Redis backups for "test_redis"
    Then response should have status 200
    And message with "backups" list is larger than in "initial_backups"
    When we remember current response as "backups"
    And we enable feature flag "MDB_REDIS_ALLOW_DEPRECATED_6"
    And we restore Redis using latest "backups" and config
    """
    name: restored_redis
    environment: PRESTABLE
    hostSpecs:
      - zoneId: vla
    configSpec:
      redisConfig_6_0:
        maxmemoryPolicy: ALLKEYS_LFU
        password: secret-password
      resources:
        resourcePresetId: db1.nano
        diskSize: 20000000000
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And we are able to log in to "restored_redis" with following parameters
    """
    password: secret-password
    """
    And following query on one host succeeds
    """
    GET test-key
    """
    And query result is like
    """
    - secret-value
    """
    And cluster has no pending changes
    And tls mode is "off"
    And redis major version is "6.0"

  @backups
  Scenario: Redis backups (tls mode on to on, 6.0 to 6.2)
    When we enable feature flag "MDB_REDIS_62"
    And we get Redis backups for "test_redis"
    And we remember current response as "backups"
    And we restore Redis using latest "backups" and config
    """
    name: restored_redis_2
    tlsEnabled: true
    environment: PRESTABLE
    hostSpecs:
      - zoneId: vla
    configSpec:
      redisConfig_6_2:
        maxmemoryPolicy: ALLKEYS_LFU
        password: secret-password
      resources:
        resourcePresetId: db1.nano
        diskSize: 20000000000
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And we are able to log in to "restored_redis_2" with following parameters
    """
    password: secret-password
    """
    And following query on one host succeeds
    """
    GET test-key
    """
    And query result is like
    """
    - secret-value
    """
    And cluster has no pending changes
    And tls mode is "on"
    And redis major version is "6.2"

  @backups
  Scenario: Remove restored cluster
    Given cluster "restored_redis" is up and running
    When we attempt to remove cluster "restored_redis"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And we are unable to find cluster "restored_redis"

  Scenario: Upgrade Redis cluster to 6.2
    Given we are able to log in to "test_redis" with following parameters
    """
    password: new_passw0rd
    """
    When we enable feature flag "MDB_REDIS_62"
    And we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      version: "6.2"
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes
    And redis major version is "6.2"
    And following query on one host succeeds
    """
    INFO server
    """
    And tls mode is "on"

  Scenario: Remove Redis cluster
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
    But s3 has bucket for cluster
    And in worker_queue exists "redis_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster
