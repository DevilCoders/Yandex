@dependent-scenarios
Feature: Internal API MongoDB Cluster lifecycle

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
    And we are working with standard_replicaset_4_4 MongoDB cluster
    And feature flag "MDB_MONGODB_RS_PITR" is set

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

  Scenario: Downgrade MongoDB featureCompatibilityVersion
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      featureCompatibilityVersion: "4.2"
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And mongodb major version is "4.4"
    And mongodb featureCompatibilityVersion is "4.2"

  Scenario: Upgrade MongoDB featureCompatibilityVersion
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      featureCompatibilityVersion: "4.4"
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And mongodb major version is "4.4"
    And mongodb featureCompatibilityVersion is "4.4"

  Scenario: Scale MongoDB cluster volume size up
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      mongodbSpec_4_4:
        mongod:
          resources:
            diskSize: 21474836480
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  Scenario: Scale MongoDB cluster volume size down
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      mongodbSpec_4_4:
        mongod:
          resources:
            diskSize: 10737418240
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  Scenario: Scale cluster instanceType up
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      mongodbSpec_4_4:
        mongod:
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
                  cacheSizeGB: 1.0
    """

  Scenario: Scale cluster instanceType down
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      mongodbSpec_4_4:
        mongod:
          resources:
            resourcePresetId: db1.nano
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
    """

  Scenario: Change cluster options
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      mongodbSpec_4_4:
        mongod:
          config:
            net:
              maxIncomingConnections: 256
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

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

  @hosts
  Scenario: Add MongoDB host to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add host in cluster
    """
    hostSpecs:
      - zoneId: sas
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 4 members

  @hosts
  Scenario: Remove MongoDB host from cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove host in geo "man" from "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster" has 3 members

  @backups
  Scenario: MongoDB backups
    Given cluster "test_cluster" is up and running
    When we get MongoDB backups for "test_cluster"
    Then response should have status 200
    And message should have not empty "backups" list
    When we remember current response as "initial_backups"
    And another_test_user inserts 5 test mongodb docs into "testdb1.test"
    And we create backup for MongoDB "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    When we get MongoDB backups for "test_cluster"
    Then response should have status 200
    And message with "backups" list is larger than in "initial_backups"
    When we remember current response as "backups"
    And another_test_user inserts 10 test mongodb docs into "testdb1.test"
    When we save shard "rs01" last oplog timestamp to "after_second_load"
    And another_test_user inserts 100 test mongodb docs into "testdb1.test"
    Then another_test_user has 115 test mongodb docs in "testdb1.test"
    When we wait until shard "rs01" timestamp next to "after_second_load" appears in storage
    And we restore MongoDB using latest "backups" timestamp "after_second_load" and config
    """
    name: test_cluster_at_last_backup
    environment: PRESTABLE
    hostSpecs:
      - zoneId: man
      - zoneId: sas
      - zoneId: vla
    configSpec:
      mongodbSpec_4_4:
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
    When we attempt to get cluster "test_cluster_at_last_backup"
    And mongodb MONGOD "rs01" shard replset in cluster "test_cluster_at_last_backup" is synchronized
    Then another_test_user has 15 test mongodb docs in "testdb1.test"
    And cluster has no pending changes

  @backups
  Scenario: Remove restored cluster
    Given cluster "test_cluster_at_last_backup" is up and running
    When we attempt to remove cluster "test_cluster_at_last_backup"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And we are unable to find cluster "test_cluster_at_last_backup"

  Scenario: Resetup hosts works
    Given cluster "test_cluster" is up and running
    When we attempt to resetup mongod host in geo "sas" in "test_cluster"
    Then generated task is finished within "20 minutes"
    And cluster "test_cluster" is up and running

  Scenario: Stepdown hosts works
    Given cluster "test_cluster" is up and running
    When we attempt to stepdown mongod host in geo "sas" in "test_cluster"
    Then generated task is finished within "20 minutes"
    And cluster "test_cluster" is up and running

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
