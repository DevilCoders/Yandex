Feature: MySQL Cluster upgrade from 5.7 to 8.0

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    Given feature flag "MDB_MYSQL_8_0" is set
    Given feature flag "MDB_MYSQL_5_7_TO_8_0_UPGRADE" is set


  @setup
  Scenario: MySQL cluster creation works
    Given we are working with standard MySQL cluster
    When we try to create cluster "test_mysql"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_mysql" is up and running
    And s3 has bucket for cluster
    And initial backup task is done
    And cluster has no pending changes


  Scenario: Upgrade-checker is working
    Given we are working with standard MySQL cluster
    Given cluster "test_mysql" is up and running
    When we execute command on mysql master in "test_mysql" as user "mysql"
    """
    mysql -e "CREATE TABLE mysql.catalogs(id INT)"
    """
    When we attempt to modify cluster "test_mysql" with following parameters
    """
    configSpec:
        version: '8.0'
    """
    Then response should have status 200
    And generated task is failed within "5 minutes" with error message "Upgrade prohibited by upgrade-checker. Fix following issues: Error: The following tables in mysql schema have names that will conflict with the ones introduced in 8.0 version. They must be renamed or removed before upgrading (use RENAME TABLE command). This may also entail changes to applications that use the affected tables. [mysql.catalogs]"
    And cluster has no pending changes
    When we execute command on mysql master in "test_mysql" as user "mysql"
    """
    mysql -e "DROP TABLE mysql.catalogs"
    """

  Scenario: Upgrade MySQL cluster to version 8.0 works
    Given we are working with standard MySQL cluster
    Given cluster "test_mysql" is up and running
    When we attempt to modify cluster "test_mysql" with following parameters
    """
    configSpec:
        version: '8.0'
    """
    Then generated task is finished within "20 minutes"
    And following SQL request on master in "test_mysql" succeeds
    """
    SHOW VARIABLES LIKE "version";
    """
    And MySQL major version result is like
    """
    - version: 8
    """
    And cluster has no pending changes
    And following SQL request on master in "test_mysql" succeeds
    """
    SELECT version() LIKE '%8.0%' AS ok
    """
    And query result is exactly
    """
    - ok: 1
    """
    When we execute SQL on master in "test_mysql"
    """
    CREATE TABLE test_table_1(id INT)
    """
    Then all changes in mysql "test_mysql" are replicated within "30" seconds



  @setup
  Scenario: MySQL single node creation works
    Given we are working with single_instance MySQL cluster
    When we try to create cluster "test_mysql_single"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_mysql_single" is up and running
    And s3 has bucket for cluster
    And cluster has no pending changes

  Scenario: Upgrade MySQL single node to version 8.0 works
    Given we are working with single_instance MySQL cluster
    Given cluster "test_mysql_single" is up and running
    When we attempt to modify cluster "test_mysql_single" with following parameters
    """
    configSpec:
      mysqlConfig_5_7:
        sqlMode: [ 'NO_AUTO_CREATE_USER' ]
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And all hosts in mysql "test_mysql_single" are online
    And cluster has no pending changes


    When we attempt to modify cluster "test_mysql_single" with following parameters
    """
    configSpec:
        version: '8.0'
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"
    And following SQL request on master in "test_mysql_single" succeeds
    """
    SHOW VARIABLES LIKE "version";
    """
    And MySQL major version result is like
    """
    - version: 8
    """
    And cluster has no pending changes
    And following SQL request on master in "test_mysql_single" succeeds
    """
    SELECT version() LIKE '%8.0%' AS ok
    """
    And query result is exactly
    """
    - ok: 1
    """
