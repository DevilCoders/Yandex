@dependent-scenarios
Feature: Internal API Redis 6.2 Cluster lifecycle

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
    And we are working with standard_62_tls Redis cluster

  @setup
  Scenario: Redis cluster creation works
    When we enable feature flag "MDB_REDIS_62"
    And we try to create cluster "test_redis_renamed"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And we are able to log in to "test_redis_renamed" with following parameters
    """
    password: passw0rd
    """
    And s3 has bucket for cluster
    And cluster has no pending changes
    And redis major version is "6.2"
    And tls mode is "on"
    And following query on master host succeeds
    """
    INFO replication
    """
    And query result is like
    """
    - connected_slaves:2
    """
    When we attempt to list hosts in "Redis" cluster [via gRPC]
    Then response body contains
    """
    {
        "hosts": [
            {
                "name": "ignore",
                "cluster_id": "ignore",
                "zone_id": "man",
                "resources": {
                    "resource_preset_id": "db1.nano",
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd"
                },
                "health": "ALIVE",
                "services": [{"health": "ALIVE", "type": "TYPE_UNSPECIFIED"}],
                "shard_name": "shard1",
                "replica_priority": "100",
                "subnet_id": "",
                "role": "ROLE_UNKNOWN",
                "assign_public_ip": false
            },
            {
                "name": "ignore",
                "cluster_id": "ignore",
                "zone_id": "sas",
                "resources": {
                    "resource_preset_id": "db1.nano",
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd"
                },
                "health": "ALIVE",
                "services": [{"health": "ALIVE", "type": "TYPE_UNSPECIFIED"}],
                "shard_name": "shard1",
                "replica_priority": "100",
                "subnet_id": "",
                "role": "ROLE_UNKNOWN",
                "assign_public_ip": false
            },
            {
                "name": "ignore",
                "cluster_id": "ignore",
                "zone_id": "vla",
                "resources": {
                    "resource_preset_id": "db1.nano",
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd"
                },
                "health": "ALIVE",
                "services": [{"health": "ALIVE", "type": "TYPE_UNSPECIFIED"}],
                "shard_name": "shard1",
                "replica_priority": "100",
                "subnet_id": "",
                "role": "ROLE_UNKNOWN",
                "assign_public_ip": false
            }
        ]
    }
    """

  @failover
  Scenario: Redis failover works [grpc]
    Given cluster "test_redis_renamed" is up and running
    When we choose one redis master and remember it
    And we attempt to start failover in "Redis" cluster [via gRPC]
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes"
    And we wait for "30 seconds"
    And redis master should be different from saved

  Scenario: Update Redis cluster name
    Given cluster "test_redis_renamed" is up and running
    When we attempt to modify cluster "test_redis_renamed" with following parameters
    """
    name: test_redis
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_redis" with following parameters
    """
    password: passw0rd
    """
    And tls mode is "on"
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
      redisConfig_6_2:
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
    And following query on master host succeeds
    """
    INFO replication
    """
    And query result is like
    """
    - connected_slaves:2
    """

  Scenario: Update Redis cluster config
    Given cluster "test_redis" is up and running
    When we attempt to modify cluster "test_redis" with following parameters
    """
    configSpec:
      redisConfig_6_2:
        timeout: 10
        maxmemoryPolicy: VOLATILE_LRU
        slowlogLogSlowerThan: 30000
        slowlogMaxLen: 4000
        notifyKeyspaceEvents: Elgtmd
    """
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_redis" with following parameters
    """
    password: new_passw0rd
    """
    And tls mode is "on"

  Scenario: Check restart preserves data without persistence
    Given cluster "test_redis" is up and running
    And we are able to log in to "test_redis" with following parameters
    """
    password: new_passw0rd
    """
    When we attempt to modify cluster "test_redis" with following parameters
    """
      persistenceMode: "OFF"
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster "test_redis" is up and running
    And cluster has no pending changes
    And aof missing on all hosts
    And following query contains "'aof_enabled:0'" on all redis hosts
    """
    INFO persistence
    """
    When we execute the following query on master host
    """
    SET test-key-pers secret-value-pers
    """
    Then we wait for "10 seconds"
    And following query exactly "['secret-value-pers']" on all redis hosts
    """
    GET test-key-pers
    """
    When we restart redis on all hosts
    Then cluster "test_redis" is up and running
    And we wait for "10 seconds"
    And following query exactly "['secret-value-pers']" on all redis hosts
    """
    GET test-key-pers
    """
    When we kill redis on master hosts
    Then cluster "test_redis" is up and running
    And we wait for "10 seconds"
    And following query exactly "['secret-value-pers']" on all redis hosts
    """
    GET test-key-pers
    """
    When we clear Redis data on all hosts
    And we kill redis on all hosts
    Then cluster "test_redis" is up and running
    And we wait for "10 seconds"
    And following query exactly "None" on all redis hosts
    """
    GET test-key-pers
    """
    When we attempt to modify cluster "test_redis" with following parameters
    """
      persistenceMode: "ON"
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster "test_redis" is up and running
    And cluster has no pending changes
    And aof exists on all hosts
    And following query contains "'aof_enabled:1'" on all redis hosts
    """
    INFO persistence
    """

  @backups
  Scenario: Redis backups (tls mode on to off)
    Given cluster "test_redis" is up and running
    When we attempt to list backups in "Redis" cluster by folderId [grpc]
    Then backups count is equal to "0"
    When we attempt to list clusterbackups in "Redis" cluster [via gRPC]
    Then backups count is equal to "0"
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
    And generated task is finished within "10 minutes"
    When we attempt to list backups in "Redis" cluster by folderId [grpc]
    Then backups count is greater than "0"
    When we attempt to backup in "Redis" cluster [via gRPC]
    Then response status is "OK" [grpc]
    And generated task is finished within "10 minutes"
    When we attempt to list clusterbackups in "Redis" cluster [via gRPC]
    Then backups count is greater than "1"
    And we remember current response as "backups"
    When we get latest Redis backup from "backups" [grpc]
    Then response body contains
    """
    {
        "created_at": "ignore",
        "folder_id": "rmi00000000000000001",
        "id": "ignore",
        "source_cluster_id": "ignore",
        "source_shard_names": ["shard1"],
        "started_at": "ignore"
    }
    """
    When we get Redis backups for "test_redis"
    Then response should have status 200
    And message with "backups" list is larger than in "initial_backups"
    When we remember current response as "backups"
    And we enable feature flag "MDB_REDIS_62"
    And we restore Redis using latest "backups" and config
    """
    name: restored_redis
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
    And redis major version is "6.2"

  @backups
  Scenario: Remove restored cluster
    Given cluster "restored_redis" is up and running
    When we attempt to remove cluster "restored_redis"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And we are unable to find cluster "restored_redis"

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
