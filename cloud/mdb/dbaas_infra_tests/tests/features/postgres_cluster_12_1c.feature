@dependent-scenarios
Feature: Internal API PostgreSQL 12-1c Cluster lifecycle

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard 12-1c PostgreSQL cluster

  @setup
  Scenario: PostgreSQL cluster created successfully
    Given feature flag "MDB_POSTGRESQL_12_1C" is set
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

  @extensions
  Scenario: Drop extension without dependent objects works, but with raise
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: rum
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    DROP SCHEMA IF EXISTS extensions CASCADE; -- workaround for retries
    CREATE SCHEMA extensions;
    CREATE TABLE extensions.drop (t text, a tsvector);
    
    CREATE TRIGGER tsvectorupdate
    BEFORE UPDATE OR INSERT ON extensions.drop
    FOR EACH ROW EXECUTE PROCEDURE tsvector_update_trigger('a', 'pg_catalog.english', 't');
    
    INSERT INTO extensions.drop(t) VALUES ('The situation is most beautiful');
    INSERT INTO extensions.drop(t) VALUES ('It is a beautiful');
    INSERT INTO extensions.drop(t) VALUES ('It looks like a beautiful place');
    
    CREATE INDEX rumidx ON extensions.drop USING rum (a rum_tsvector_ops);
    SELECT 'ok' result;
    """
    And query result is like
    """
    - result: ok
    """
    When we attempt to modify database "testdb" in "test_cluster"
    """
    "extensions": []
    """
    Then response should have status 200
    And generated task is failed within "3 minutes" with error message
    """
    ERROR:  cannot drop extension rum because other objects depend on it
    DETAIL:  index extensions.rumidx depends on operator class rum_tsvector_ops for access method rum
    HINT:  Use DROP ... CASCADE to drop the dependent objects too.
    """
    And cluster has no pending changes
    When we execute SQL on master in "test_cluster" without results
    """
    DROP INDEX extensions.rumidx CASCADE;
    """
    And we attempt to modify database "testdb" in "test_cluster"
    """
    "extensions": []
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension ORDER BY extname;
    """
    And query result is exactly
    """
    - extname: fasttrun
    - extname: fulleq
    - extname: mchar
    - extname: plpgsql
    - extname: yndx_1c_aux
    """

  @backups
  Scenario: Backups
    When we get PostgreSQL backups for "test_cluster"
    Then response should have status 200
    And message with "backups" list is larger than "0"
    When we remember current response as "initial_backups"
    And we execute SQL on master in "test_cluster" without results
    """
    DROP SCHEMA IF EXISTS mail CASCADE; -- workaround for retries
    CREATE SCHEMA mail;
    CREATE TABLE mail.box (id SERIAL, message TEXT);
    INSERT INTO mail.box (message) VALUES ('Spam'), ('Ham'), ('Important!');
    """
    And we create backup for PostgreSQL "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    When we get PostgreSQL backups for "test_cluster"
    Then response should have status 200
    And message with "backups" list is larger than in "initial_backups"
    When we remember current response as "backups_before_delete"
    And we execute SQL on master in "test_cluster" without results
    """
    DELETE FROM mail.box
    """
    # restore cluster with smaller instanceType - MDB-2722
    And we restore PostgreSQL using latest "backups_before_delete" and config
    """
    name: test_cluster_at_last_backup
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
    And generated task is finished within "14 minutes"
    And following SQL request on master in "test_cluster_at_last_backup" succeeds
    """
    SELECT message FROM mail.box WHERE message ~ 'Important'
    """
    And query result is exactly
    """
    - message: 'Important!'
    """
    When we execute SQL on master in "test_cluster_at_last_backup" in database "template1"
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
    Then query result is like
    """
    - extname: 'fasttrun'
    - extname: 'fulleq'
    - extname: 'mchar'
    - extname: 'yndx_1c_aux'
    """
    And cluster has no pending changes

  Scenario: Add PostgreSQL database to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add database in "test_cluster"
    """
    databaseSpec:
      name: testdb2
      owner: test_user
      lcCollate: ru_RU.UTF-8
      lcCtype: ru_RU.UTF-8
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  Scenario: Remove PostgreSQL database from cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove database "testdb" in cluster
    Then response should have status 200
    And generated task is finished within "15 minutes"
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
    Then query result is like
    """
    - extname: 'fasttrun'
    - extname: 'fulleq'
    - extname: 'mchar'
    - extname: 'yndx_1c_aux'
    """
