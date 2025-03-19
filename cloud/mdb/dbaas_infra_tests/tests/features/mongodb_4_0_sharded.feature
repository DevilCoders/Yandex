@dependent-scenarios
Feature: Internal API MongoDB Cluster lifecycle

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And feature flag "MDB_MONGODB_ALLOW_DEPRECATED_VERSIONS" is set
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard_replicaset_4_0 MongoDB cluster

  @setup
  Scenario: MongoDB cluster creation works
    When we try to create cluster "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And s3 has bucket for cluster
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" is synchronized
    And mongodb major version is "4.0"
    And mongodb featureCompatibilityVersion is "4.0"
    And initial backup task is done

  @sharding
  Scenario: Enable sharding of MongoDB cluster
    Given cluster "test_cluster" is up and running
    And feature flag "mongodb_sharding" is set
    When we attempt to enable sharding in cluster "test_cluster"
    """
    mongos:
      resources:
        resourcePresetId: db1.nano
        diskSize: 10737418240
        diskTypeId: local-ssd
    mongocfg:
      resources:
        resourcePresetId: db1.nano
        diskSize: 21474836480
        diskTypeId: local-ssd
    hostSpecs:
        - zoneId: man
          type: MONGOS
        - zoneId: sas
          type: MONGOS
        - zoneId: man
          type: MONGOCFG
        - zoneId: sas
          type: MONGOCFG
        - zoneId: vla
          type: MONGOCFG
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOCFG replset in cluster "test_cluster" has 3 members

  @sharding
  Scenario: Add shard
    Given cluster "test_cluster" is up and running
    When we attempt to add shard in cluster
    """
    shardName: shard2
    hostSpecs:
      - zoneId: sas
      - zoneId: vla
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOD "shard2" shard replset in cluster "test_cluster" has 2 members
    And mongodb MONGOCFG replset in cluster "test_cluster" has 3 members
    And mongodb major version is "4.0"
    And mongodb featureCompatibilityVersion is "4.0"

  Scenario: Downgrade MongoDB featureCompatibilityVersion
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      featureCompatibilityVersion: "3.6"
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And mongodb major version is "4.0"
    And mongodb featureCompatibilityVersion is "3.6"

  Scenario: Upgrade MongoDB featureCompatibilityVersion
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      featureCompatibilityVersion: "4.0"
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And mongodb major version is "4.0"
    And mongodb featureCompatibilityVersion is "4.0"

  @hosts
  Scenario: Add MongoDB mongod host to first shard
    Given cluster "test_cluster" is up and running
    When we attempt to add host in cluster
    """
    hostSpecs:
      - zoneId: sas
        shardName: rs01
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 4 members
    And mongodb MONGOD "shard2" shard replset in cluster "test_cluster" has 2 members
    And mongodb MONGOCFG replset in cluster "test_cluster" has 3 members

  @hosts
  Scenario: Add MongoDB mongod host to second shard
    Given cluster "test_cluster" is up and running
    When we attempt to add host in cluster
    """
    hostSpecs:
      - zoneId: sas
        shardName: shard2
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 4 members
    And mongodb MONGOD "shard2" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOCFG replset in cluster "test_cluster" has 3 members

  @hosts
  Scenario: Remove MongoDB mongod host from first shard
    Given cluster "test_cluster" is up and running
    When we attempt to remove "MONGOD" host in geo "man" from shard "rs01"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOD "shard2" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOCFG replset in cluster "test_cluster" has 3 members

  @hosts
  Scenario: Remove MongoDB mongod host from second shard
    Given cluster "test_cluster" is up and running
    When we attempt to remove "MONGOD" host in geo "vla" from shard "shard2"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOD "shard2" shard replset in cluster "test_cluster" has 2 members
    And mongodb MONGOCFG replset in cluster "test_cluster" has 3 members

  @hosts
  Scenario: Add MongoDB mongocfg host to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add host in cluster
    """
    hostSpecs:
      - zoneId: sas
        type: MONGOCFG
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOD "shard2" shard replset in cluster "test_cluster" has 2 members
    And mongodb MONGOCFG replset in cluster "test_cluster" has 4 members

  @hosts
  Scenario: Remove MongoDB mongocfg host from cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove "MONGOCFG" host in geo "man" from "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOD "shard2" shard replset in cluster "test_cluster" has 2 members
    And mongodb MONGOCFG replset in cluster "test_cluster" has 3 members

  @hosts
  Scenario: Add MongoDB mongos host to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add host in cluster
    """
    hostSpecs:
      - zoneId: sas
        type: MONGOS
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOD "shard2" shard replset in cluster "test_cluster" has 2 members
    And mongodb MONGOCFG replset in cluster "test_cluster" has 3 members

  @hosts
  Scenario: Remove MongoDB mongos host from cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove "MONGOS" host in geo "man" from "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOD "shard2" shard replset in cluster "test_cluster" has 2 members
    And mongodb MONGOCFG replset in cluster "test_cluster" has 3 members

  @roles
  Scenario: Sharding role works properly
    Given cluster "test_cluster" is up and running
    When sh_admin enables sharding of database "testsh_db"
    And sh_admin shards "testsh_db.test" with key "_id" and type "hashed"
    Then ns "testsh_db.test" has "2" chunks on shard "rs01"
    And ns "testsh_db.test" has "2" chunks on shard "shard2"
    When user_testsh_db inserts 100 test mongodb docs into "testsh_db.test" MONGOS

  @backups
  Scenario: MongoDB backups
    Given cluster "test_cluster" is up and running
    When we get MongoDB backups for "test_cluster"
    Then response should have status 200
    And message should have not empty "backups" list
    When we remember current response as "initial_backups"
    And another_test_user inserts 5 test mongodb docs into "testdb1.test" MONGOD "rs01"
    And user_testdb2 inserts 15 test mongodb docs into "testdb2.test" MONGOD "shard2"
    Then another_test_user has 5 test mongodb docs in "testdb1.test" MONGOD "rs01"
    And user_testdb2 has 15 test mongodb docs in "testdb2.test" MONGOD "shard2"
    When we create backup for MongoDB "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    When we get MongoDB backups for "test_cluster"
    Then response should have status 200
    And message with "backups" list is larger than in "initial_backups"
    When we remember current response as "backups"
    When we restore MongoDB using latest "backups" containing "shard2" and config
    """
    name: test_cluster_shard2_at_last_backup
    environment: PRESTABLE
    hostSpecs:
      - zoneId: man
      - zoneId: sas
      - zoneId: vla
    configSpec:
      mongodbSpec_4_0:
        mongod:
          config:
            net:
              maxIncomingConnections: 100
          resources:
            resourcePresetId: db1.nano
            diskSize: 20000000000
            diskTypeId: local-ssd
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    When we attempt to get cluster "test_cluster_shard2_at_last_backup"
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster_shard2_at_last_backup" is synchronized
    Then user_testdb2 has 15 test mongodb docs in "testdb2.test" MONGOD "rs01"
    And cluster has no pending changes

  @backups
  Scenario: Remove cluster with restored shard
    Given cluster "test_cluster_shard2_at_last_backup" is up and running
    When we attempt to remove cluster "test_cluster_shard2_at_last_backup"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And we are unable to find cluster "test_cluster_shard2_at_last_backup"

  @sharding
  Scenario: Remove shard
    Given cluster "test_cluster" is up and running
    When admin inserts 5 test mongodb docs into "testdb3.test" MONGOS
    Then admin has 5 test mongodb docs in "testdb3.test" MONGOS
    And sh_admin moves mongodb primary databases from "shard2" to "rs01"
    And sh_admin flushes MONGOS cache
    When we attempt to remove shard "shard2" in cluster
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members
    And mongodb MONGOCFG replset in cluster "test_cluster" has 3 members
    When admin inserts 5 test mongodb docs into "testdb3.test" MONGOS
    Then admin has 10 test mongodb docs in "testdb3.test" MONGOS

  @clean
  Scenario: Remove cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove cluster "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And in worker_queue exists "mongodb_cluster_delete_metadata" task
    And this task is done
    And autogenerated shard was deleted from Solomon
    And we are unable to find cluster "test_cluster"
    But s3 has bucket for cluster
    And in worker_queue exists "mongodb_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster
