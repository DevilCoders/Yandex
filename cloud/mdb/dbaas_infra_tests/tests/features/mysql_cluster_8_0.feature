@dependent-scenarios
Feature: Internal API MySQL Cluster lifecycle

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard 8 MySQL cluster

  @setup
  Scenario: MySQL cluster creation works
    Given feature flag "MDB_MYSQL_8_0" is set
    When we try to create cluster "test_mysql"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And all hosts in mysql "test_mysql" are online
    And cluster "test_mysql" is up and running
    And s3 has bucket for cluster
    And initial backup task is done
    And cluster has no pending changes

  Scenario: Replication works just after MySQL cluster creation
    Given cluster "test_mysql" is up and running
    When we execute SQL on master in "test_mysql"
    """
    CREATE TABLE test_table_1(id INT)
    """
    Then following SQL request in "test_mysql" succeeds within "3 minutes"
    """
    SELECT * FROM test_table_1
    """

  Scenario: Failover without target works
    Given cluster "test_mysql" is up and running
    When we attempt to failover cluster "test_mysql"
    Then response should have status 200
    And generated task is finished within "2 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_mysql" succeeds
    """
    SELECT @@hostname LIKE 'man%' AS ok
    """
    And query result is exactly
    """
    - ok: 0
    """

  Scenario: Failover with target works
    Given cluster "test_mysql" is up and running
    When we attempt to failover cluster "test_mysql" to host in geo "man"
    Then response should have status 200
    And generated task is finished within "2 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_mysql" succeeds
    """
    SELECT @@hostname LIKE 'man%' AS ok
    """
    And query result is exactly
    """
    - ok: 1
    """

  Scenario: Scale MySQL cluster volume size up
    Given cluster "test_mysql" is up and running
    When we attempt to modify cluster "test_mysql" with following parameters
    """
    configSpec:
      resources:
        diskSize: 21474836480
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And all hosts in mysql "test_mysql" are online
    And cluster has no pending changes

  Scenario: Scale MySQL cluster volume size down
    Given cluster "test_mysql" is up and running
    When we attempt to modify cluster "test_mysql" with following parameters
    """
    configSpec:
      resources:
        diskSize: 10737418240
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  @scale-up
  Scenario: Scale MySQL cluster instanceType up
    Given cluster "test_mysql" is up and running
    When we attempt to modify cluster "test_mysql" with following parameters
    """
    configSpec:
      resources:
        resourcePresetId: db1.small
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And all hosts in mysql "test_mysql" are online
    And cluster has no pending changes
    And following SQL request on master in "test_mysql" succeeds
    """
    SELECT @@max_connections AS max_connections
    """
    And query result is like
    """
     - max_connections: 256
    """

  @scale-down
  Scenario: Scale MySQL cluster instanceType down
    Given cluster "test_mysql" is up and running
    When we attempt to modify cluster "test_mysql" with following parameters
    """
    configSpec:
      resources:
        resourcePresetId: db1.nano
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And all hosts in mysql "test_mysql" are online
    And cluster has no pending changes
    And following SQL request on master in "test_mysql" succeeds
    """
    SELECT @@max_connections AS max_connections
    """
    And query result is like
    """
     - max_connections: 100
    """

  Scenario: Change MySQL cluster options without restart
    Given cluster "test_mysql" is up and running
    When we attempt to modify cluster "test_mysql" with following parameters
    """
    configSpec:
      mysqlConfig_8_0:
        maxConnections: 30
        innodbBufferPoolSize: 536870912
        slowQueryLog: ON
        longQueryTime: 1.5
        generalLog: ON
        maxAllowedPacket: 2097152
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And following SQL request in "test_mysql" succeeds
    """
    SELECT 
    @@max_connections AS max_connections, 
    @@global.innodb_buffer_pool_size AS innodb_buffer_pool_size, 
    @@global.long_query_time as long_query_time, 
    @@global.general_log as general_log, 
    @@global.max_allowed_packet as max_allowed_packet,
    @@global.slow_query_log as slow_query_log
    """
    And query result is like
    """
     - max_connections: 30
       innodb_buffer_pool_size: 536870912
       slow_query_log: 1
       long_query_time: 1.5
       general_log: ON
       max_allowed_packet: 2097152
    """

  Scenario: Remove MySQL host
    Given cluster "test_mysql" is up and running
    When we attempt to remove host in geo "vla" from "test_mysql"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    # NOTE: we don't check SLAVE HOSTS here as it
    # detect slave disconnect with huge delay
    And cluster "test_mysql" is up and running
    And cluster has no pending changes

  Scenario: Add MySQL host to cluster
    Given cluster "test_mysql" is up and running
    When we attempt to add host in "test_mysql"
    """
    hostSpecs:
      - zoneId: myt
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And all hosts in mysql "test_mysql" are online
    And cluster "test_mysql" is up and running
    And cluster has no pending changes

  Scenario: Replication works after adding new host to MySQL cluster
    Given cluster "test_mysql" is up and running
    Then following SQL request on host in geo "myt" in "test_mysql" succeeds within "1 minutes"
    """
    SELECT * FROM test_table_1
    """
    When we execute SQL on master in "test_mysql"
    """
    CREATE TABLE test_table_2(id INT)
    """
    Then all changes in mysql "test_mysql" are replicated within "30" seconds

  Scenario: Add MySQL cascade host to cluster
    Given cluster "test_mysql" is up and running
    When we attempt to add host in "vla" to "test_mysql" replication_source "sas"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And all hosts in mysql "test_mysql" are online
    And host in geo "vla" in "test_mysql" is replica of host in geo "sas" within "2min"
    And cluster has no pending changes

  Scenario: Replication works after adding cascade host to MySQL cluster
    Given cluster "test_mysql" is up and running
    Then following SQL request on host in geo "vla" in "test_mysql" succeeds within "1 minutes"
    """
    SELECT * FROM test_table_2
    """
    When we execute SQL on master in "test_mysql"
    """
    CREATE TABLE test_table_3(id INT)
    """
    Then all changes in mysql "test_mysql" are replicated within "30" seconds

  Scenario: Add MySQL cascade level-2 host to cluster
    Given cluster "test_mysql" is up and running
    When we attempt to add host in "iva" to "test_mysql" replication_source "vla"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And host in geo "iva" in "test_mysql" is replica of host in geo "vla" within "2min"
    And host in geo "vla" in "test_mysql" is replica of host in geo "sas" within "2min"
    And cluster has no pending changes

  Scenario: Replication works after adding cascade level-2 host to MySQL cluster
    Given cluster "test_mysql" is up and running
    Then following SQL request on host in geo "iva" in "test_mysql" succeeds within "1 minutes"
    """
    SELECT * FROM test_table_1
    """
    When we execute SQL on master in "test_mysql"
    """
    CREATE TABLE test_table_cascade_level2(id INT)
    """
    Then all changes in mysql "test_mysql" are replicated within "30" seconds

  Scenario: Remove cascade level-2 MySQL host
    Given cluster "test_mysql" is up and running
    When we attempt to remove host in geo "iva" from "test_mysql"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster "test_mysql" is up and running
    And host in geo "vla" in "test_mysql" is replica of host in geo "sas" within "2min"
    And cluster has no pending changes

  Scenario: Modify MySQL host replication_source
    Given cluster "test_mysql" is up and running
    When we attempt to modify host in "vla" in cluster "test_mysql" replication_source geo "myt"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And all hosts in mysql "test_mysql" are online
    And host in geo "vla" in "test_mysql" is replica of host in geo "myt" within "2min"
    And cluster has no pending changes

  Scenario: Replication works after changing replication source of host in MySQL cluster
    Given cluster "test_mysql" is up and running
    Then following SQL request on host in geo "vla" in "test_mysql" succeeds within "1 minutes"
    """
    SELECT * FROM test_table_3
    """
    When we execute SQL on master in "test_mysql"
    """
    CREATE TABLE test_table_4(id INT)
    """
    Then all changes in mysql "test_mysql" are replicated within "30" seconds

  Scenario: Reset MySQL host replication_source
    Given cluster "test_mysql" is up and running
    When we attempt to modify host in "vla" in cluster "test_mysql" replication_source reset
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And all hosts in mysql "test_mysql" are online
    And cluster has no pending changes

  Scenario: Replication works after reseting replication source of host in MySQL cluster
    Given cluster "test_mysql" is up and running
    Then following SQL request on host in geo "vla" in "test_mysql" succeeds within "1 minutes"
    """
    SELECT * FROM test_table_4
    """
    When we execute SQL on master in "test_mysql"
    """
    CREATE TABLE test_table_5(id INT)
    """
    Then all changes in mysql "test_mysql" are replicated within "30" seconds

  Scenario: Remove cascade MySQL host
    Given cluster "test_mysql" is up and running
    When we attempt to remove host in geo "vla" from "test_mysql"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster "test_mysql" is up and running
    And cluster has no pending changes

  Scenario: Add MySQL host to cluster from backup
    Given cluster "test_mysql" is up and running
    And cluster pillar path "data:walg-restore" set to "true"
    When we attempt to add host in "test_mysql"
    """
    hostSpecs:
      - zoneId: sas
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And all hosts in mysql "test_mysql" are online
    And cluster "test_mysql" is up and running
    And host in geo "sas" in "test_mysql" is replica of host in geo "man"
    And cluster has no pending changes

  Scenario: Replication works after adding new host to MySQL cluster from backup
    Given cluster "test_mysql" is up and running
    Then following SQL request on host in geo "sas" in "test_mysql" succeeds within "1 minutes"
    """
    SELECT * FROM test_table_1
    """
    When we execute SQL on master in "test_mysql"
    """
    CREATE TABLE test_table_6(id INT)
    """
    Then all changes in mysql "test_mysql" are replicated within "30" seconds

  Scenario: Add MySQL database to cluster
    Given cluster "test_mysql" is up and running
    When we attempt to add database in cluster
    """
    databaseSpec:
      name: testdb4
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes

  @backups
  Scenario: Backups
    Given cluster "test_mysql" is up and running
    When we execute SQL on master in "test_mysql"
    """
    CREATE TABLE test_table_b1(id INT)
    """
    And we execute SQL on master in "test_mysql"
    """
    INSERT INTO test_table_b1(id) VALUES (42)
    """
    When we execute command on mysql master in "test_mysql" as user "mysql"
    """
    mysql -e "FLUSH LOGS"
    """
    And we get MySQL backups for "test_mysql"
    Then response should have status 200
    When we remember current response as "backups_at_scenario_start"
    And we create backup for MySQL "test_mysql"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    When we get MySQL backups for "test_mysql"
    Then response should have status 200
    And message with "backups" list is larger than in "backups_at_scenario_start"
    When we remember current response as "backups_before_delete"
    And we execute SQL on master in "test_mysql"
    """
    CREATE TABLE test_table_pitr(id INT)
    """
    And we execute SQL on master in "test_mysql"
    """
    INSERT INTO test_table_pitr(id) VALUES (42)
    """
    # wait before doing changes that must not be represented after PITR
    Then we wait for "10"
    Then we remember current timestamp as "pitr_moment"
    Then we wait for "10"
    When we execute SQL on master in "test_mysql" without results
    """
    DELETE FROM test_table_b1
    """
    When we execute command on mysql master in "test_mysql" as user "mysql"
    """
    mysql -e "FLUSH LOGS"
    """
    When we execute command on mysql master in "test_mysql" as user "mysql"
    """
    flock -w 10 -o /tmp/mysql_walg_cron.lock wal-g-mysql binlog-push --config /etc/wal-g/wal-g.yaml
    """
    Then command retcode should be "0"

    When we restore MySQL using latest "backups_before_delete" timestamp "pitr_moment" and config
    """
    name: test_mysql_restore
    environment: PRESTABLE
    hostSpecs:
      - zoneId: man
      - zoneId: vla
      - zoneId: sas
    configSpec:
      resources:
        resourcePresetId: db1.nano
        diskSize: 20000000000
        diskTypeId: local-ssd
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And all hosts in mysql "test_mysql_restore" are online
    And following SQL request in "test_mysql_restore" succeeds
    """
    SELECT id FROM test_table_b1 WHERE id = 42
    """
    And following SQL request in "test_mysql_restore" succeeds
    """
    SELECT id FROM test_table_pitr WHERE id = 42
    """
    And query result is exactly
    """
    - id: 42
    """
    When we execute SQL on master in "test_mysql_restore"
    """
    CREATE TABLE test_table_b2(id INT)
    """
    Then following SQL request in "test_mysql_restore" succeeds within "1 minutes"
    """
    SELECT * FROM test_table_b2
    """
    And cluster has no pending changes

  Scenario: Remove MySQL database from cluster
    Given cluster "test_mysql" is up and running
    When we attempt to remove database "testdb3" in cluster
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And cluster has no pending changes

  Scenario: Add user to cluster
    Given cluster "test_mysql" is up and running
    When we attempt to add user in cluster
    """
    userSpec:
      name: petya
      password: password-password-password-password
      permissions:
        - databaseName: testdb4
          roles: ['SELECT']
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And all changes in mysql "test_mysql" are replicated within "30" seconds
    And we are able to log in mysql "test_mysql" with following parameters:
    """
    user: petya
    password: password-password-password-password
    dbname: testdb4
    """

  Scenario: Add user with mysql_native_password to cluster
    Given cluster "test_mysql" is up and running
    When we attempt to add user in cluster
    """
    userSpec:
      name: user_native
      password: password-password-password-password
      authenticationPlugin: MYSQL_NATIVE_PASSWORD
      permissions:
        - databaseName: testdb4
          roles: ['SELECT']
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And all changes in mysql "test_mysql" are replicated within "30" seconds
    And we are able to log in mysql "test_mysql" with following parameters:
    """
    user: user_native
    password: password-password-password-password
    dbname: testdb4
    """
    When we execute command on mysql master in "test_mysql" as user "mysql"
    """
    mysql -e "select plugin from mysql.user where user='user_native';"
    """
    Then command output should contain
    """
    mysql_native_password
    """

  Scenario: Add user with sha256_password to cluster
    Given cluster "test_mysql" is up and running
    When we attempt to add user in cluster
    """
    userSpec:
      name: user_sha256
      password: password-password-password-password
      authenticationPlugin: SHA256_PASSWORD
      permissions:
        - databaseName: testdb4
          roles: ['SELECT']
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And all changes in mysql "test_mysql" are replicated within "30" seconds
    And we are able to log in mysql "test_mysql" with following parameters:
    """
    user: user_sha256
    password: password-password-password-password
    dbname: testdb4
    """
    When we execute command on mysql master in "test_mysql" as user "mysql"
    """
    mysql -e "select plugin from mysql.user where user='user_sha256';"
    """
    Then command output should contain
    """
    sha256_password
    """

  Scenario: Add user with caching_sha2_password to cluster
    Given cluster "test_mysql" is up and running
    When we attempt to add user in cluster
    """
    userSpec:
      name: user_sha2
      password: password-password-password-password
      authenticationPlugin: CACHING_SHA2_PASSWORD
      permissions:
        - databaseName: testdb4
          roles: ['SELECT']
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And all changes in mysql "test_mysql" are replicated within "30" seconds
    And we are able to log in mysql "test_mysql" with following parameters:
    """
    user: user_sha2
    password: password-password-password-password
    dbname: testdb4
    """
    When we execute command on mysql master in "test_mysql" as user "mysql"
    """
    mysql -e "select plugin from mysql.user where user='user_sha2';"
    """
    Then command output should contain
    """
    caching_sha2_password
    """

  Scenario Outline: Add original user to cluster
    Given cluster "test_mysql" is up and running
    When we attempt to add user in cluster
    """
    userSpec:
      name: <name>
      password: <password>
      authenticationPlugin: <plugin> 
      permissions:
        - databaseName: testdb4
          roles: ['SELECT']
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And all changes in mysql "test_mysql" are replicated within "30" seconds
    And we are able to log in mysql "test_mysql" with following parameters:
    """
    user: <name>
    password: <password>
    dbname: testdb4
    """
    Examples:
        | name       | password      | plugin                  |
        | ru-sha256- | пароль123     | SHA256_PASSWORD         | 
        | ru-sha2-   | пароль123     | CACHING_SHA2_PASSWORD   | 
        | ru-native- | пароль123     | MYSQL_NATIVE_PASSWORD   | 

  Scenario: Grant permission to user
    Given cluster "test_mysql" is up and running
    When we attempt to grant permission to user "test_user" in cluster
    """
    permission:
      databaseName: testdb4
      roles: ['ALL_PRIVILEGES', 'SELECT']
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And all changes in mysql "test_mysql" are replicated within "30" seconds
    And we are able to log in mysql with following parameters:
    """
    user: test_user
    password: mysupercooltestpassword11111
    dbname: testdb4
    """

  Scenario: Revoke permission from user
    Given cluster "test_mysql" is up and running
    When we attempt to revoke permission from user "test_user" in cluster
    """
    permission:
      databaseName: testdb4
      roles: ['ALL_PRIVILEGES', 'SELECT']
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And all changes in mysql "test_mysql" are replicated within "30" seconds
    And we are unable to log in mysql with following parameters:
    """
    user: test_user
    password: mysupercooltestpassword11111
    dbname: testdb4
    """
    And we are able to log in mysql with following parameters:
    """
    user: test_user
    password: mysupercooltestpassword11111
    dbname: testdb1
    """

  Scenario: Change user password
    Given cluster "test_mysql" is up and running
    When we attempt to modify user "petya" in cluster
    """
    password: password_changed-password_changed-password_changed
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And all changes in mysql "test_mysql" are replicated within "30" seconds
    And we are unable to log in mysql with following parameters:
    """
    user: petya
    password: password-password-password-password
    dbname: testdb4
    """
    And we are able to log in mysql "test_mysql" with following parameters:
    """
    user: petya
    password: password_changed-password_changed-password_changed
    dbname: testdb4
    """

  Scenario: Change user permissions
    Given cluster "test_mysql" is up and running
    When we attempt to modify user "petya" in cluster
    """
    permissions:
      - databaseName: testdb1
        roles: ['SELECT']
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And all changes in mysql "test_mysql" are replicated within "30" seconds
    And we are unable to log in mysql with following parameters:
    """
    user: petya
    password: password_changed-password_changed-password_changed
    dbname: testdb4
    """
    And we are able to log in mysql "test_mysql" with following parameters:
    """
    user: petya
    password: password_changed-password_changed-password_changed
    dbname: testdb1
    """

  Scenario: Remove user
    Given cluster "test_mysql" is up and running
    When we attempt to remove user "petya" in cluster
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And all changes in mysql "test_mysql" are replicated within "30" seconds
    And we are unable to log in mysql with following parameters:
    """
    user: petya
    password: password_changed-password_changed-password_changed
    dbname: testdb1
    """

  Scenario: Change MySQL cluster options with restart
    Given cluster "test_mysql" is up and running
    When we attempt to modify cluster "test_mysql" with following parameters
    """
    configSpec:
      mysqlConfig_8_0:
        defaultAuthenticationPlugin: MYSQL_NATIVE_PASSWORD
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And all hosts in mysql "test_mysql" are online
    And cluster has no pending changes
    And following SQL request in "test_mysql" succeeds
    """
    SELECT @@default_authentication_plugin AS defaultAuthenticationPlugin
    """
    And query result is like
    """
     - defaultAuthenticationPlugin: mysql_native_password
    """

  Scenario: Change MySQL cluster SQL mode
    Given cluster "test_mysql" is up and running
    Then following SQL request in "test_mysql" succeeds
    """
    SELECT @@sql_mode
    """
    And query result is like
    """
    [
        {
            '@@sql_mode': 'ONLY_FULL_GROUP_BY,NO_DIR_IN_CREATE,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION'
        }
    ]
    """
    When we attempt to modify cluster
    """
    configSpec:
        mysqlConfig_8_0:
            sqlMode: [ 'ALLOW_INVALID_DATES', 'ONLY_FULL_GROUP_BY' ]
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And following SQL request in "test_mysql" succeeds
    """
    SELECT @@sql_mode
    """
    And query result is like
    """
    [
        {
            '@@sql_mode': 'ONLY_FULL_GROUP_BY,ALLOW_INVALID_DATES'
        }
    ]
    """
    When we attempt to modify cluster
    """
    configSpec:
        mysqlConfig_8_0:
            sqlMode: []
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And following SQL request in "test_mysql" succeeds
    """
    SELECT @@sql_mode
    """
    And query result is like
    """
    [
        {
            '@@sql_mode': ''
        }
    ]
    """

  Scenario: Change MySQL cluster charset and collation
    Given cluster "test_mysql" is up and running
    Then following SQL request in "test_mysql" succeeds
    """
    SELECT
        @@GLOBAL.collation_connection as collation_connection,
        @@GLOBAL.collation_database as collation_database,
        @@GLOBAL.collation_server as collation_server
    """
    And query result is like
    """
    [
        {
          "collation_connection": "utf8mb4_0900_ai_ci",
          "collation_database": "utf8mb4_0900_ai_ci",
          "collation_server": "utf8mb4_0900_ai_ci"
        }
    ]
    """
    And following SQL request in "test_mysql" succeeds
    """
    SELECT @@GLOBAL.character_set_client as character_set_client,
        @@GLOBAL.character_set_connection as character_set_connection,
        @@GLOBAL.character_set_database as character_set_database,
        @@GLOBAL.character_set_results as character_set_results,
        @@GLOBAL.character_set_server as character_set_server,
        @@GLOBAL.character_set_system as character_set_system
    """
    And query result is like
    """
    [
        {
          "character_set_client": "utf8mb4",
          "character_set_connection": "utf8mb4",
          "character_set_database": "utf8mb4",
          "character_set_results": "utf8mb4",
          "character_set_server": "utf8mb4",
          "character_set_system": "utf8mb3"
        }
    ]
    """
    When we attempt to modify cluster
    """
    configSpec:
        mysqlConfig_8_0:
            collationServer: 'cp1251_general_ci'
            characterSetServer: 'cp1251'
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And cluster has no pending changes
    And following SQL request in "test_mysql" succeeds
    """
    SELECT
        @@GLOBAL.collation_connection as collation_connection,
        @@GLOBAL.collation_database as collation_database,
        @@GLOBAL.collation_server as collation_server
    """
    And query result is like
    """
    [
        {
           "collation_connection": "cp1251_general_ci",
           "collation_database": "utf8mb4_0900_ai_ci",
           "collation_server": "cp1251_general_ci"
        }
    ]
    """
    And following SQL request in "test_mysql" succeeds
    """
    SELECT @@GLOBAL.character_set_client as character_set_client,
        @@GLOBAL.character_set_connection as character_set_connection,
        @@GLOBAL.character_set_database as character_set_database,
        @@GLOBAL.character_set_results as character_set_results,
        @@GLOBAL.character_set_server as character_set_server,
        @@GLOBAL.character_set_system as character_set_system
    """
    And query result is like
    """
    [
        {
           "character_set_client": "cp1251",
           "character_set_connection": "cp1251",
           "character_set_database": "utf8mb4",
           "character_set_results": "cp1251",
           "character_set_server": "cp1251",
           "character_set_system": "utf8mb3"
        }
    ]
    """

  Scenario: Execute maintenance task for MySQL
    Given cluster "test_mysql" is up and running
    When we create maintenance task for MySQL cluster "test_mysql"
    """
    restart: false
    """
    Then generated task is finished within "15 minutes"
    And cluster "test_mysql" is up and running
    When we create maintenance task for MySQL cluster "test_mysql"
    """
    restart: true
    """
    Then generated task is finished within "15 minutes"
    And cluster "test_mysql" is up and running
    When we execute SQL on master in "test_mysql"
    """
    CREATE TABLE test_table_7(id INT)
    """
    Then all changes in mysql "test_mysql" are replicated within "30" seconds

  Scenario: Rename MySQL cluster
    Given cluster "test_mysql" is up and running
    When we attempt to modify cluster "test_mysql" with following parameters
    """
    name: test_mysql_renamed
    """
    Then response should have status 200
    And generated task is finished within "2 minutes"
    And generated task has description "Update MySQL cluster metadata"
    And cluster has no pending changes
    When we execute command on mysql master in "test_mysql_renamed" as user "mysql"
    """
    grep "test_mysql_renamed" /etc/dbaas.conf
    """

  Scenario: Remove MySQL cluster
    Given cluster "test_mysql_renamed" is up and running
    When we attempt to remove cluster "test_mysql_renamed"
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And in worker_queue exists "mysql_cluster_delete_metadata" task
    And this task is done
    And autogenerated shard was deleted from Solomon
    And we are unable to find cluster "test_mysql_renamed"
    But s3 has bucket for cluster
    And in worker_queue exists "mysql_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster
