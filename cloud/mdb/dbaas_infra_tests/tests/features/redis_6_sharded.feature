@dependent-scenarios
Feature: Internal API Redis 6 Sharded Cluster lifecycle

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
    And we are working with sharded_6 Redis cluster

  @setup
  Scenario: Redis sharded cluster creation works
    When we enable feature flag "MDB_REDIS_SHARDING"
    And we enable feature flag "MDB_REDIS_ALLOW_DEPRECATED_6"
    And we try to create cluster "sharded_redis"
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And s3 has bucket for cluster
    And cluster has no pending changes
    And we are able to log in to "sharded_redis" with following parameters
    """
    password: passw0rd
    """
    And following query on one host succeeds
    """
    CLUSTER info
    """
    And query result is like
    """
    - cluster_state:ok
    - cluster_size:3
    - cluster_known_nodes:6
    - cluster_slots_fail:0
    - cluster_slots_pfail:0
    - cluster_slots_assigned:16384
    - cluster_slots_ok:16384
    """
    And redis major version is "6.0"
    And tls mode is "off"

  @failover
  Scenario: Redis failover works
    Given cluster "sharded_redis" is up and running
    When we choose one redis master and remember it
    And we attempt to failover redis cluster "sharded_redis" with saved master
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And redis master should be different from saved

  @hosts
  Scenario: Add Redis host to sharded cluster
    Given cluster "sharded_redis" is up and running
    When we attempt to add host in cluster
    """
    hostSpecs:
      - zoneId: sas
        shardName: shard1
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "sharded_redis" is up and running
    And cluster has no pending changes
    And following query on one host succeeds
    """
    CLUSTER info
    """
    And query result is like
    """
    - cluster_state:ok
    - cluster_size:3
    - cluster_known_nodes:7
    """
    When we execute the following query on master host of "shard1"
    """
    INFO replication
    """
    Then query result is like
    """
    - connected_slaves:2
    """

  @hosts
  Scenario: Remove Redis host from sharded cluster
    Given cluster "sharded_redis" is up and running
    When we attempt to remove host in geo "myt" from shard "shard1"
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And cluster "sharded_redis" is up and running
    And cluster has no pending changes
    And following query on one host succeeds
    """
    CLUSTER info
    """
    And query result is like
    """
    - cluster_state:ok
    - cluster_size:3
    - cluster_known_nodes:6
    """
    When we execute the following query on master host of "shard1"
    """
    INFO replication
    """
    Then query result is like
    """
    - connected_slaves:1
    """

  @shards
  Scenario: Add Redis shard to cluster
    Given cluster "sharded_redis" is up and running
    When we attempt to add shard in cluster
    """
    shardName: shard4
    hostSpecs:
      - zoneId: sas
      - zoneId: vla
      - zoneId: man
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "sharded_redis" is up and running
    And cluster has no pending changes
    And following query on one host succeeds
    """
    CLUSTER info
    """
    And query result is like
    """
    - cluster_state:ok
    - cluster_size:3
    - cluster_known_nodes:9
    """
    When we execute the following query on master host of "shard4"
    """
    INFO replication
    """
    Then query result is like
    """
    - connected_slaves:2
    """

  @shards
  Scenario: Rebalance Redis cluster slots
    Given cluster "sharded_redis" is up and running
    When we attempt to rebalance cluster
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And cluster "sharded_redis" is up and running
    And cluster has no pending changes
    And following query on one host succeeds
    """
    CLUSTER info
    """
    And query result is like
    """
    - cluster_state:ok
    - cluster_size:4
    - cluster_known_nodes:9
    """

  @shards
  Scenario: Remove Redis shard from cluster
    Given cluster "sharded_redis" is up and running
    When we attempt to remove shard "shard1" in cluster
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And cluster "sharded_redis" is up and running
    And cluster has no pending changes
    And following query on one host succeeds
    """
    CLUSTER info
    """
    And query result is like
    """
    - cluster_state:ok
    - cluster_size:3
    - cluster_known_nodes:7
    """

  @backups
  Scenario: Redis backups (tls mode off to off, 6.0 to 6.2)
    Given cluster "sharded_redis" is up and running
    When we get Redis backups for "sharded_redis"
    Then response should have status 200
    When we remember current response as "initial_backups"
    And we insert 10 keys into shard "shard2"
    When we create backup for Redis "sharded_redis"
    Then response should have status 200
    And generated task is finished within "7 minutes"
    When we get Redis backups for "sharded_redis"
    Then response should have status 200
    And message with "backups" list is larger than in "initial_backups"
    When we remember current response as "backups"
    And we enable feature flag "MDB_REDIS_62"
    And we restore Redis using latest "backups" containing "shard2" and config
    """
    name: restored_shard
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
    And generated task is finished within "20 minutes"
    And we are able to log in to "restored_shard" with following parameters
    """
    password: secret-password
    """
    And following query on one host succeeds
    """
    INFO keyspace
    """
    And query result is like
    """
    - db0:keys=10,expires=0,avg_ttl=0
    """
    And cluster has no pending changes
    And tls mode is "off"
    And redis major version is "6.2"

  @backups
  Scenario: Redis backups (tls mode off to on)
    When we get Redis backups for "sharded_redis"
    And we remember current response as "backups"
    And we enable feature flag "MDB_REDIS_ALLOW_DEPRECATED_6"
    And we restore Redis using latest "backups" containing "shard2" and config
    """
    name: restored_shard_2
    tlsEnabled: true
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
    And generated task is finished within "20 minutes"
    And we are able to log in to "restored_shard_2" with following parameters
    """
    password: secret-password
    """
    And following query on one host succeeds
    """
    INFO keyspace
    """
    And query result is like
    """
    - db0:keys=10,expires=0,avg_ttl=0
    """
    And cluster has no pending changes
    And tls mode is "on"
    And redis major version is "6.0"

  @backups
  Scenario: Remove cluster with restored shard
    Given we are able to log in to "restored_shard" with following parameters
    """
    password: secret-password
    """
    When we attempt to remove cluster "restored_shard"
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And we are unable to find cluster "restored_shard"

  @modify
  Scenario: Scale Redis cluster volume size up
    Given we are able to log in to "sharded_redis" with following parameters
    """
    password: passw0rd
    """
    When we attempt to modify cluster "sharded_redis" with following parameters
    """
    configSpec:
      resources:
        diskSize: 34359738368
    """
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And cluster has no pending changes

  @modify
  Scenario: Scale Redis cluster volume size down
    Given cluster "sharded_redis" is up and running
    When we attempt to modify cluster "sharded_redis" with following parameters
    """
    configSpec:
      resources:
        diskSize: 17179869184
    """
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And cluster has no pending changes

  @modify
  Scenario: Scale cluster instance type up
    Given cluster "sharded_redis" is up and running
    When we attempt to modify cluster "sharded_redis" with following parameters
    """
    configSpec:
      resources:
        resourcePresetId: db1.micro
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  @modify
  Scenario: Scale cluster instance type down
    Given cluster "sharded_redis" is up and running
    When we attempt to modify cluster "sharded_redis" with following parameters
    """
    configSpec:
      resources:
        resourcePresetId: db1.nano
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  @modify
  Scenario: Change Redis cluster password
    Given cluster "sharded_redis" is up and running
    When we attempt to modify cluster "sharded_redis" with following parameters
    """
    configSpec:
      redisConfig_6_0:
        password: newpassword
    """
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And we are able to log in to "sharded_redis" with following parameters
    """
    password: newpassword
    """
    And cluster has no pending changes
    And tls mode is "off"

  @modify
  Scenario: Change Redis cluster options with restart
    Given we are able to log in to "sharded_redis" with following parameters
    """
    password: newpassword
    """
    And cluster "sharded_redis" is up and running
    When we attempt to modify cluster "sharded_redis" with following parameters
    """
    configSpec:
      redisConfig_6_0:
        timeout: 10
        maxmemoryPolicy: VOLATILE_LRU
        slowlogLogSlowerThan: 30000
        slowlogMaxLen: 4000
        notifyKeyspaceEvents: Elgtm
        databases: 10
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And tls mode is "off"

  @clean
  Scenario: Remove cluster
    Given we are able to log in to "sharded_redis" with following parameters
    """
    password: newpassword
    """
    When we attempt to remove cluster "sharded_redis"
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And in worker_queue exists "redis_cluster_delete_metadata" task
    And this task is done
    And autogenerated shard was deleted from Solomon
    And we are unable to find cluster "sharded_redis"
    But s3 has bucket for cluster
    And in worker_queue exists "redis_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster
