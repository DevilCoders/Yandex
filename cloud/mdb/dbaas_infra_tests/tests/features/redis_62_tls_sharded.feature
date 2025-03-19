@dependent-scenarios
Feature: Internal API Redis 6.2 Sharded Cluster lifecycle

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
    And we are working with sharded_62_tls Redis cluster

  @setup
  Scenario: Redis sharded cluster creation works
    When we enable feature flag "MDB_REDIS_SHARDING"
    And we enable feature flag "MDB_REDIS_62"
    And we try to create cluster "sharded_redis_renamed"
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And s3 has bucket for cluster
    And cluster has no pending changes
    And we are able to log in to "sharded_redis_renamed" with following parameters
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
    And redis major version is "6.2"
    And tls mode is "on"

  Scenario: Update Redis cluster name
    Given cluster "sharded_redis_renamed" is up and running
    When we attempt to modify cluster "sharded_redis_renamed" with following parameters
    """
    name: sharded_redis
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes
    And we are able to log in to "sharded_redis" with following parameters
    """
    password: passw0rd
    """
    And tls mode is "on"
    When we execute the following query on master host of "shard3"
    """
    INFO replication
    """
    Then query result is like
    """
    - connected_slaves:1
    """

  @modify
  Scenario: Scale cluster instance type down
    Given cluster "sharded_redis" is up and running
    Then following query contains "'maxmemory:1610612736'" on all redis hosts sharded
    """
    INFO memory
    """
    When we insert 20000 keys into shard "shard1"
    And we attempt to modify cluster "sharded_redis" with following parameters
    """
    configSpec:
      redisConfig_6_2:
        maxmemoryPolicy: NOEVICTION
      resources:
        resourcePresetId: db1.dnische
    """
    Then response should have status 200
    And generated task is failed within "10 minutes"
    And cluster has no pending changes
    And following query contains "'maxmemory:1610612736'" on all redis hosts sharded
    """
    INFO memory
    """
    When we attempt to modify cluster "sharded_redis" with following parameters
    """
    configSpec:
      redisConfig_6_2:
        maxmemoryPolicy: ALLKEYS_LFU
      resources:
        resourcePresetId: db1.dnische2
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And cluster has no pending changes
    And following query contains "'maxmemory:240000000'" on all redis hosts sharded
    """
    INFO memory
    """
    And following query less than "20000" on all redis hosts sharded
    """
    DBSIZE
    """
    When we execute the following query on all redis hosts sharded
    """
    FLUSHALL
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

  @shards @grpc
  Scenario: Rebalance Redis cluster slots
    Given cluster "sharded_redis" is up and running
    When we attempt to rebalance in "Redis" cluster [via gRPC]
    Then response status is "OK" [grpc]
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
  Scenario: Redis backups (tls mode on to off)
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
        clientOutputBufferLimitNormal:
            hardLimit: 26777216
            hardLimitUnit: BYTES
            softLimit: 2388608
            softLimitUnit: BYTES
            softSeconds: 20
        clientOutputBufferLimitPubsub:
            hardLimit: 36777216
            hardLimitUnit: BYTES
            softLimit: 3388608
            softLimitUnit: BYTES
            softSeconds: 30
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
  Scenario: Change Redis cluster password
    Given we are able to log in to "sharded_redis" with following parameters
    """
    password: passw0rd
    """
    And cluster "sharded_redis" is up and running
    When we attempt to modify cluster "sharded_redis" with following parameters
    """
    configSpec:
      redisConfig_6_2:
        password: newpassword
    """
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And we are able to log in to "sharded_redis" with following parameters
    """
    password: newpassword
    """
    And cluster has no pending changes
    And tls mode is "on"

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
      redisConfig_6_2:
        timeout: 10
        maxmemoryPolicy: VOLATILE_LRU
        slowlogLogSlowerThan: 30000
        slowlogMaxLen: 4000
        notifyKeyspaceEvents: Elgtmd
        databases: 10
        clientOutputBufferLimitNormal:
            hardLimit: 26777216
            hardLimitUnit: BYTES
            softLimit: 2388608
            softLimitUnit: BYTES
            softSeconds: 20
        clientOutputBufferLimitPubsub:
            hardLimit: 36777216
            hardLimitUnit: BYTES
            softLimit: 3388608
            softLimitUnit: BYTES
            softSeconds: 30
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And tls mode is "on"

  Scenario: Check restart preserves data without persistence
    Given cluster "sharded_redis" is up and running
    And we are able to log in to "sharded_redis" with following parameters
    """
    password: newpassword
    """
    When we attempt to modify cluster "sharded_redis" with following parameters
    """
      persistenceMode: "OFF"
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster "sharded_redis" is up and running
    And cluster has no pending changes
    And aof missing on all hosts
    And following query contains "'aof_enabled:0'" on all redis hosts sharded
    """
    INFO persistence
    """
    When we execute the following query on all redis hosts sharded
    """
    SET test-key-pers secret-value-pers
    """
    Then we wait for "10 seconds"
    And following query exactly "['secret-value-pers']" on all redis hosts sharded
    """
    GET test-key-pers
    """
    When we restart redis on all hosts
    Then cluster "sharded_redis" is up and running
    And we wait for "10 seconds"
    And following query exactly "['secret-value-pers']" on all redis hosts sharded
    """
    GET test-key-pers
    """
    When we kill redis on master hosts
    Then cluster "sharded_redis" is up and running
    And we wait for "10 seconds"
    And following query exactly "['secret-value-pers']" on all redis hosts sharded
    """
    GET test-key-pers
    """
    When we clear Redis data on all hosts
    And we kill redis on all hosts
    Then cluster "sharded_redis" is up and running
    And we wait for "10 seconds"
    And following query exactly "None" on all redis hosts sharded
    """
    GET test-key-pers
    """
    When we attempt to modify cluster "sharded_redis" with following parameters
    """
      persistenceMode: "ON"
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster "sharded_redis" is up and running
    And cluster has no pending changes
    And aof exists on all hosts
    And following query contains "'aof_enabled:1'" on all redis hosts sharded
    """
    INFO persistence
    """

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
