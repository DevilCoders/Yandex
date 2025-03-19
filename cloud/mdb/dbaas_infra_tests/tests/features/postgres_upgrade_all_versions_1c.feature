@dependent-scenarios
Feature: Internal API PostgreSQL 10-1c Cluster lifecycle

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard 10-1c PostgreSQL cluster

  @setup
  Scenario: PostgreSQL cluster created successfully
    Given feature flag "MDB_POSTGRESQL_10_1C" is set
    Given feature flag "MDB_PG_ALLOW_DEPRECATED_VERSIONS" is set
    When we try to create cluster "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And we are able to log in to "test_cluster" with following parameters:
    """
    user: test_user
    password: mysupercooltestpassword11111
    """
    And s3 has bucket for cluster
    And cluster has no pending changes
    When we execute SQL on master in "test_cluster" in database "template1"
    """
    SELECT extname
        FROM pg_extension
    WHERE extname IN (
        'yndx_1c_aux',
        'fasttrun',
        'fulleq',
        'mchar'
    )
    ORDER BY extname
    """
    Then query result is exactly
    """
    - extname: 'fasttrun'
    - extname: 'fulleq'
    - extname: 'mchar'
    - extname: 'yndx_1c_aux'
    """
    When we execute SQL on master in "test_cluster" without results
    """
    SET lc_messages TO 'ru_RU.UTF-8';
    """
    Then following SQL request on master in "test_cluster" succeeds
    """
    SHOW lc_messages;
    """
    And query result is like
    """
    - lc_messages: 'ru_RU.UTF-8'
    """
    And initial backup task is done

  Scenario: Upgrade PostgreSQL cluster to version 11 works
    Given cluster "test_cluster" is up and running
    Given feature flag "MDB_POSTGRESQL_11_1C" is set
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
        version: '11-1c'
    """
    Then response should have status 200
    And generated task is finished within "14 minutes"
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT setting::int / 100 AS version FROM pg_settings WHERE name = 'server_version_num';
    """
    And query result is like
    """
    - version: 1100
    """
    And cluster has no pending changes
    When we execute SQL on master in "test_cluster" in database "template1"
    """
    SELECT extname
        FROM pg_extension
    WHERE extname IN (
        'yndx_1c_aux',
        'fasttrun',
        'fulleq',
        'mchar'
    )
    ORDER BY extname
    """
    Then query result is exactly
    """
    - extname: 'fasttrun'
    - extname: 'fulleq'
    - extname: 'mchar'
    - extname: 'yndx_1c_aux'
    """

