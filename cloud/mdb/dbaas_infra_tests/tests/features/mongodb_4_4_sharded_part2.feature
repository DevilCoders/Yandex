@dependent-scenarios
Feature: Internal API MongoDB Cluster lifecycle

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard_replicaset_4_4 MongoDB cluster

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
    And mongodb major version is "4.4"
    And mongodb featureCompatibilityVersion is "4.4"
    And initial backup task is done
    When admin inserts 5 test mongodb docs into "testdb1.dbtest"
    And admin inserts 5 test mongodb docs into "testdb2.dbtest"
    And admin inserts 5 test mongodb docs into "testdb3.dbtest"

  @sharding
  Scenario: Enable sharding of MongoDB cluster
    Given cluster "test_cluster" is up and running
    And feature flag "mongodb_sharding" is set
    When we attempt to enable sharding in cluster "test_cluster"
    """
    mongoinfra:
      resources:
        resourcePresetId: db1.nano
        diskSize: 10737418240
        diskTypeId: local-ssd
    hostSpecs:
        - zoneId: man
          type: MONGOINFRA
        - zoneId: sas
          type: MONGOINFRA
        - zoneId: vla
          type: MONGOINFRA
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members

  @databases
  Scenario: Add MongoDB database to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add database in cluster
    """
    databaseSpec:
      name: testdb4
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    When admin inserts 5 test mongodb docs into "testdb4.dbtest"
    Then mongodb databases list is equal to following list
    """
    - admin
    - config
    - local
    - mdb_internal
    - testdb1
    - testdb2
    - testdb3
    - testdb4
    """

  @databases
  Scenario: Remove MongoDB database from cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove database "testdb3" in cluster
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And mongodb databases list is equal to following list
    """
    - admin
    - config
    - local
    - mdb_internal
    - testdb1
    - testdb2
    - testdb4
    """

  @users
  Scenario: Add user to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add user in cluster
    """
    userSpec:
      name: petya
      password: password-password-password-password
      permissions:
        - databaseName: testdb4
          roles: ['read']
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters
    """
    name: petya
    password: password-password-password-password
    dbname: testdb4
    """

  @users
  Scenario: Add original user to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add user in cluster
    """
    userSpec:
      name: pet-ya-
      password: password-password-password-пароль
      permissions:
        - databaseName: testdb4
          roles: ['read']
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters
    """
    name: pet-ya-
    password: password-password-password-пароль
    dbname: testdb4
    """

  @users
  Scenario: Grant permission to user
    Given cluster "test_cluster" is up and running
    When we attempt to grant permission to user "test_user" in cluster
    """
    permission:
      databaseName: testdb4
      roles: ['readWrite']
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters
    """
    name: test_user
    password: mysupercooltestpassword11111
    dbname: testdb4
    """

  @users
  Scenario: Revoke permission from user
    Given cluster "test_cluster" is up and running
    When we attempt to revoke permission from user "test_user" in cluster
    """
    databaseName: testdb4
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are unable to log in to "test_cluster" with following parameters
    """
    name: test_user
    password: mysupercooltestpassword11111
    dbname: testdb4
    """
    And we are able to log in to "test_cluster" with following parameters
    """
    name: test_user
    password: mysupercooltestpassword11111
    dbname: testdb1
    """

  @users
  Scenario: Change user password
    Given cluster "test_cluster" is up and running
    When we attempt to modify user "petya" in cluster
    """
    password: password_changed-password_changed-password_changed
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are unable to log in to "test_cluster" with following parameters
    """
    name: petya
    password: password-password-password-password
    dbname: testdb4
    """
    And we are able to log in to "test_cluster" with following parameters
    """
    name: petya
    password: password_changed-password_changed-password_changed
    dbname: testdb4
    """

  @users
  Scenario: Change user permissions
    Given cluster "test_cluster" is up and running
    When we attempt to modify user "petya" in cluster
    """
    permissions:
      - databaseName: testdb1
        roles: ['readWrite']
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are unable to log in to "test_cluster" with following parameters
    """
    name: petya
    password: password_changed-password_changed-password_changed
    dbname: testdb4
    """
    And we are able to log in to "test_cluster" with following parameters
    """
    name: petya
    password: password_changed-password_changed-password_changed
    dbname: testdb1
    """

  @users
  Scenario: Remove user
    Given cluster "test_cluster" is up and running
    When we attempt to remove user "petya" in cluster
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are unable to log in to "test_cluster" with following parameters
    """
    name: petya
    password: password_changed-password_changed-password_changed
    dbname: testdb1
    """

  @modify
  Scenario: Scale MongoDB cluster volume size up and down
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      mongodbSpec_4_4:
        mongod:
          resources:
            diskSize: 21474836480
        mongoinfra:
          resources:
            diskSize: 32212254720
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  @modify
  Scenario: Scale MongoDB cluster volume size down
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      mongodbSpec_4_4:
        mongod:
          resources:
            diskSize: 10737418240
        mongoinfra:
          resources:
            diskSize: 10737418240
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  @modify
  Scenario: Scale cluster instanceType up
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      mongodbSpec_4_4:
        mongod:
          resources:
            resourcePresetId: db1.micro
        mongoinfra:
          resources:
            resourcePresetId: db1.small
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And mongodb "test_cluster" options has entry
    """
    MONGOD:
      storage:
          wiredTiger:
              engineConfig:
                  cacheSizeGB: 1.0
    """

  @modify
  Scenario: Scale cluster instanceType down
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      mongodbSpec_4_4:
        mongod:
          resources:
            resourcePresetId: db1.nano
        mongoinfra:
          resources:
            resourcePresetId: db1.micro
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And mongodb "test_cluster" options has entry
    """
    MONGOD:
      storage:
          wiredTiger:
              engineConfig:
                  cacheSizeGB: 0.5
    """

  @modify
  Scenario: Change MongoDB cluster options with restart
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      mongodbSpec_4_4:
        mongod:
          config:
            storage:
              wiredTiger:
                engineConfig:
                  cacheSizeGB: 0.25
        mongos:
          config:
            net:
              maxIncomingConnections: 256
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And cluster has no pending changes
    And mongodb "test_cluster" options has entry
    """
    MONGOD:
      storage:
        wiredTiger:
          engineConfig:
            cacheSizeGB: 0.25
    MONGOINFRA:
      net:
        maxIncomingConnections: 256
    """

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

  @hosts
  Scenario: Remove MongoDB mongos host from cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove "MONGOS" host in geo "sas" from "test_cluster"
    Then response should have status 200 
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes

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
