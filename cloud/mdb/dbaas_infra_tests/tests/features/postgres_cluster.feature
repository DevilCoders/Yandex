@dependent-scenarios
Feature: Internal API PostgreSQL Cluster lifecycle

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard PostgreSQL cluster

  @setup
  Scenario: PostgreSQL cluster created successfully
    Given feature flag "MDB_PG_ALLOW_DEPRECATED_VERSIONS" is set
    When we try to create cluster "test_cluster_renamed"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And we are able to log in to "test_cluster_renamed" with following parameters:
    """
    user: another_test_user
    password: mysupercooltestpassword11111
    """
    And s3 has bucket for cluster
    And cluster has no pending changes
    And following SQL request on master in "test_cluster_renamed" succeeds
    """
    SELECT datcollate, datctype FROM pg_database WHERE datname = 'testdb'
    """
    And query result is like
    """
    - datcollate: 'fi_FI.UTF-8'
      datctype: 'fi_FI.UTF-8'
    """
    And initial backup task is done

  Scenario: Rename PostgreSQL cluster
    Given cluster "test_cluster_renamed" is up and running
    When we attempt to modify cluster "test_cluster_renamed" with following parameters
    """
    name: test_cluster
    """
    Then response should have status 200
    And generated task is finished within "2 minutes"
    And generated task has description "Update PostgreSQL cluster metadata"
    And cluster has no pending changes
    When we execute command on host in geo "man"
    """
    grep -q "test_cluster" /etc/dbaas.conf
    """
    Then command retcode should be "0"

  Scenario: Failover without target works
    Given cluster "test_cluster" is up and running
    When we attempt to failover cluster "test_cluster"
    Then response should have status 200
    And generated task is finished within "2 minutes"
    And following SQL request on host in geo "man" in "test_cluster" succeeds
    """
    SELECT pg_is_in_recovery() AS replica
    """
    And query result is exactly
    """
    - replica: True
    """
    # FIXME: remove this sleep when MDB-5726 is done
    And we wait for "60"
    And master in "test_cluster" has "2" replics within "15 minutes"

  Scenario: Failover with target works
    Given cluster "test_cluster" is up and running
    When we attempt to failover cluster "test_cluster" to host in geo "man"
    Then response should have status 200
    And generated task is finished within "2 minutes"
    And following SQL request on host in geo "man" in "test_cluster" succeeds
    """
    SELECT pg_is_in_recovery() AS replica
    """
    And query result is exactly
    """
    - replica: False
    """
    # FIXME: remove this sleep when MDB-5726 is done
    And we wait for "60"
    And master in "test_cluster" has "2" replics within "15 minutes"

  Scenario: Change pool mode on odyssey
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
        poolerConfig:
            poolingMode: SESSION
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  Scenario: Scale PostgreSQL cluster volume size up
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      resources:
        diskSize: 21474836480
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  Scenario: Scale PostgreSQL cluster volume size down
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      resources:
        diskSize: 10737418240
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  Scenario: Change PostgreSQL cluster options with restart
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      postgresqlConfig_10:
        sharedBuffers: 268435456
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT setting FROM pg_settings WHERE name = 'shared_buffers'
    """
    And query result is like
    """
    # 8kB unit
    - setting: '32768'
    """

  Scenario: Remove PostgreSQL host
    Given cluster "test_cluster" is up and running
    When we attempt to remove host in geo "man" from "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT count(1) AS replics FROM pg_stat_replication
    """
    And query result is like
    """
    - replics: 1
    """

  Scenario: Add PostgreSQL host to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add host in "test_cluster"
    """
    hostSpecs:
      - zoneId: man
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT count(1) AS replics FROM pg_stat_replication
    """
    And query result is like
    """
    - replics: 2
    """

  Scenario: Change cluster options without restart
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      postgresqlConfig_10:
        logMinDurationStatement: 100
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  @scale-up
  Scenario: Scale cluster instanceType up
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      resources:
        resourcePresetId: db1.micro
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT setting FROM pg_settings WHERE name = 'max_connections'
    """
    And query result is like
    """
    - setting: '200'
    """

  @users
  Scenario: Add original user to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to add user in "test_cluster"
    """
    userSpec:
        name: pet-ya-
        connLimit: 10
        password: password-password-password-пароль
        permissions: []
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are unable to log in to "test_cluster" with following parameters:
    """
    user: pet-ya-
    password: password-password-password-пароль
    """

  @users
  Scenario: Add user to cluster with empty databases acl
    Given cluster "test_cluster" is up and running
    When we attempt to add user in "test_cluster"
    """
    userSpec:
        name: petya
        connLimit: 90
        password: password-password-password-password
        permissions: []
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are unable to log in to "test_cluster" with following parameters:
    """
    user: petya
    password: password-password-password-password
    """

  @users
  Scenario: Modify user adding database to acl
    Given cluster "test_cluster" is up and running
    When we attempt to modify user "petya" in "test_cluster"
    """
    permissions:
        - databaseName: testdb
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters:
    """
    user: petya
    password: password-password-password-password
    """

  @users
  Scenario: Change user connection limit
    Given cluster "test_cluster" is up and running
    When we attempt to modify user "petya" in "test_cluster"
    """
    connLimit: 10
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters:
    """
    user: petya
    password: password-password-password-password
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
    And following SQL request on master in "test_cluster_at_last_backup" succeeds
    """
    SELECT datcollate, datctype FROM pg_database WHERE datname = 'testdb'
    """
    # we don't provide database options,
    # but restore should get them
    # from source cluster
    And query result is like
    """
    - datcollate: 'fi_FI.UTF-8'
      datctype: 'fi_FI.UTF-8'
    """
    And cluster has no pending changes

  @backups
  Scenario: PostgreSQL backup service enabled
    Given cluster "test_cluster" is up and running
    When we get PostgreSQL backups for "test_cluster"
    Then response should have status 200
    And message should have not empty "backups" list
    When we remember current response as "initial_backups_storage"
    And we enable backup service
    And we import cluster backups to backup service with args "--dry-run=false --skip-sched-date-dups=true"
    When we get PostgreSQL backups for "test_cluster"
    Then response should have status 200
    And message should have not empty "backups" list
    Then message with "backups" list length is equal to "initial_backups_storage"

  @idm
  Scenario: Add user via IDM service
    Given cluster "test_cluster" is up and running
    And cluster is managed by IDM
    Then generated task is finished within "15 minutes"
    When we attempt to add grant "reader" to "some_user" on cluster "test_cluster"
    Then generated task is finished within "15 minutes"
    And cluster has no pending changes
    And user "some_user" has grant "reader" on cluster "test_cluster"

  @idm
  Scenario: Rotate passwords via IDM service
    Given cluster "test_cluster" is up and running
    When we attempt to rotate passwords on IDM managed clusters
    Then generated task is already finished
    And user "some_user" got email with new password
    And all IDM users on cluster "test_cluster" have new passwords
    And cluster has no pending changes

  @idm
  Scenario: Remove role from user via IDM service
    Given cluster "test_cluster" is up and running
    When we attempt to revoke grant "reader" from "some_user" on cluster "test_cluster"
    Then generated task is finished within "15 minutes"
    And user "some_user" doesn't have grant "reader" on cluster "test_cluster"
    And cluster has no pending changes

  @idm
  Scenario: Get list of IDM clusters
    Given cluster "test_cluster" is up and running
    When we send request to get IDM clusters
    Then IDM response OK and body has "test_cluster"

  Scenario: Scale cluster instanceType down
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      resources:
        resourcePresetId: db1.nano
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT setting FROM pg_settings WHERE name = 'max_connections'
    """
    And query result is like
    """
    - setting: '100'
    """

  Scenario: Grant mdb_admin
    Given cluster "test_cluster" is up and running
    When we attempt to modify user "test_user" in "test_cluster"
    """
    grants:
     - mdb_admin
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT 'ok' result WHERE pg_has_role('test_user', 'mdb_admin', 'member')
    """
    And query result is like
    """
    - result: ok
    """

  Scenario: Rename database
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    newDatabaseName: testdb_new
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    When we execute SQL on master in "test_cluster" in database "testdb_new"
    """
    SELECT datname FROM pg_catalog.pg_database WHERE datname IN ('testdb', 'testdb_new');
    """
    Then query result is exactly
    """
    - datname: testdb_new
    """
    When we attempt to modify database "testdb_new" in "test_cluster"
    """
    newDatabaseName: testdb
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  @extensions
  Scenario: Add extension to cluster
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: pg_trgm
      - name: btree_gist
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: btree_gist
    - extname: pg_trgm
    """

  @extensions
  Scenario: Add oracle_fdw extension
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: oracle_fdw
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: oracle_fdw
    """
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT oracle_diag();
    """

  @extensions
  Scenario: Add orafce extension
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: orafce
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: orafce
    """
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT oracle.to_date('2014-07-02 10:08:55','YYYY-MM-DD HH:MI:SS');
    """

  @extensions
  Scenario: Add plv8 extension
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: plv8
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: plv8
    """
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT plv8_version();
    """

  @extensions
   Scenario: Add hypopg extension
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: hypopg
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: hypopg
    """
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT hypopg_reset();
    """

  @extensions
  Scenario: Create db with extension works:
    Given cluster "test_cluster" is up and running
    When we attempt to add database in "test_cluster"
    """
    "databaseSpec": {
      "name": "another_test_db",
      "owner": "test_user",
      "lcCtype": "C",
      "extensions": [
        {
          "name": "pg_partman",
          "version": "5",
        },
        {
          "name": "postgis",
          "version": "5",
        },
      ],
      "lcCollate":"C"
    }
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    When we execute SQL on master in "test_cluster" in database "another_test_db"
    """
    select has_table_privilege('test_user', 'public.part_config', 'select');
    """
    Then query result is like
    """
    - has_table_privilege: True
    """

  @extensions
  Scenario: Add pg_partman extension
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: pg_partman
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: pg_partman
    """
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT unnest(pg_default_acl.defaclacl) FROM pg_catalog.pg_default_acl
    """
    And query result is like
    """
    - unnest: test_user=a*r*w*d*/postgres
    """

  @extensions
  Scenario: Add extension with dependencies
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: pg_stat_kcache
      - name: pg_stat_statements
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: pg_stat_kcache
    - extname: pg_stat_statements
    """
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT 'ok1' result FROM pg_proc WHERE proname='pg_stat_kcache_reset' AND proacl::text~'mdb_admin'
    UNION
    SELECT 'ok2' result FROM pg_proc WHERE proname='pg_stat_statements_reset' AND proacl::text~'mdb_admin';
    """
    And query result is exactly
    """
    - result: ok1
    - result: ok2
    """

  @extensions
  Scenario: Add postgis, postgis_topology
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: postgis
      - name: postgis_topology
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: postgis
    - extname: postgis_topology
    """
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT 'ok' result FROM pg_namespace n JOIN pg_roles r ON (n.nspowner=r.oid) WHERE nspname = 'topology' AND rolname='mdb_admin';
    """
    And query result is like
    """
    - result: ok
    """

  @extensions
  Scenario: Change PostgreSQL shared_preload_libraries
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      postgresqlConfig_10:
        sharedPreloadLibraries: ["SHARED_PRELOAD_LIBRARIES_AUTO_EXPLAIN", "SHARED_PRELOAD_LIBRARIES_PG_HINT_PLAN", "SHARED_PRELOAD_LIBRARIES_PG_QUALSTATS", "SHARED_PRELOAD_LIBRARIES_PG_CRON"]
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT 'ok' result FROM pg_settings WHERE name='shared_preload_libraries' AND setting~'auto_explain|pg_hint_plan|pg_qualstats|pg_cron'
    """
    And query result is like
    """
    - result: ok
    """

  @extensions
  Scenario: Add pg_hint_plan extension
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: pg_hint_plan
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: pg_hint_plan
    """

  @extensions
  Scenario: Add pg_cron extension
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: pg_cron
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: pg_cron
    """

  @extensions
  Scenario: Hunspell dictionaries for full-text search are installed
    Given cluster "test_cluster" is up and running
    When we execute SQL on master in "test_cluster" without results
    """
    CREATE TEXT SEARCH DICTIONARY russian_hunspell (
        TEMPLATE = ispell,
        DictFile = ru_RU,
        AffFile = ru_RU,
        Stopwords = russian);

    CREATE TEXT SEARCH CONFIGURATION hello_world (
      COPY = simple
    );

    ALTER TEXT SEARCH CONFIGURATION hello_world
        ALTER MAPPING FOR word
        WITH russian_hunspell, russian_stem;

    SET default_text_search_config = 'hello_world';
    """
    Then following SQL request on master in "test_cluster" succeeds
    """
    SELECT * FROM ts_debug('Привет');
    """
    And query result is like
    """
    - dictionaries: "{russian_hunspell,russian_stem}"
    """

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
    SELECT extname FROM pg_extension
    """
    And query result is exactly
    """
    - extname: plpgsql
    """

  @extensions
  Scenario: pg_repack cannot be dropped
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: pg_repack
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: pg_repack
    """
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT table_name, privilege_type FROM information_schema.role_table_grants WHERE table_schema='repack' AND grantee = 'mdb_admin';
    """
    And query result is like
    """
    - table_name: primary_keys
      privilege_type: SELECT
    - table_name: tables
      privilege_type: SELECT
    """
    When we attempt to modify database "testdb" in "test_cluster"
    """
    "extensions": []
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: pg_repack
    """

  @extensions
  Scenario: Add pgvector extension
    Given cluster "test_cluster" is up and running
    When we attempt to modify database "testdb" in "test_cluster"
    """
    extensions:
      - name: pgvector
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT extname FROM pg_extension
    """
    And query result is like
    """
    - extname: pgvector
    """
    And following SQL request on master in "test_cluster" succeeds
    """
    CREATE TABLE IF NOT EXISTS t1(c vector(3));
    DROP TABLE IF EXISTS t1 CASCADE; -- workaround for retries
    SELECT 1;
    """

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
    When we execute SQL on master in "test_cluster" in database "testdb2" without results
    """
    CREATE SUBSCRIPTION sub CONNECTION 'host=vasyanhost' PUBLICATION pub WITH (CONNECT=false)
    """
    And we attempt to remove database "testdb2" in cluster
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes

  Scenario: Change user password
    Given cluster "test_cluster" is up and running
    When we attempt to modify user "petya" in "test_cluster"
    """
    password: password_changed-password_changed-password_changed
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are able to log in to "test_cluster" with following parameters:
    """
    user: petya
    password: password_changed-password_changed-password_changed
    """

  Scenario: Remove user
    Given cluster "test_cluster" is up and running
    When we attempt to remove user "petya" in "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    And we are unable to log in to "test_cluster" with following parameters:
    """
    user: petya
    password: password_changed-password_changed-password_changed
    """

  Scenario: Manual failover without target and with autofailover turned off works
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
      autofailover: false
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And cluster has no pending changes
    When we attempt to failover cluster "test_cluster"
    Then response should have status 200
    And generated task is finished within "2 minutes"
    And following SQL request on host in geo "man" in "test_cluster" succeeds
    """
    SELECT pg_is_in_recovery() AS replica
    """
    And query result is exactly
    """
    - replica: True
    """
    # FIXME: remove this sleep when MDB-5726 is done
    And we wait for "60"
    And master in "test_cluster" has "2" replics within "15 minutes"

  Scenario: Manual failover with target and with autofailover turned off works
    Given cluster "test_cluster" is up and running
    When we attempt to failover cluster "test_cluster" to host in geo "man"
    Then response should have status 200
    And generated task is finished within "2 minutes"
    And following SQL request on host in geo "man" in "test_cluster" succeeds
    """
    SELECT pg_is_in_recovery() AS replica
    """
    And query result is exactly
    """
    - replica: False
    """
    # FIXME: remove this sleep when MDB-5726 is done
    And we wait for "60"
    And master in "test_cluster" has "2" replics within "15 minutes"

  Scenario: Execute maintenance task for PostgreSQL
    Given cluster "test_cluster" is up and running
    When we create maintenance task for PostgreSQL cluster "test_cluster"
    """
    restart: true
    """
    Then generated task is finished within "15 minutes"
    And cluster "test_cluster" is up and running

  @delete
  Scenario: Remove cluster
    Given cluster "test_cluster" is up and running
    When we attempt to remove cluster "test_cluster"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And in worker_queue exists "postgresql_cluster_delete_metadata" task
    And this task is done
    And autogenerated shard was deleted from Solomon
    And we are unable to find cluster "test_cluster"
    But s3 has bucket for cluster
    And in worker_queue exists "postgresql_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster
