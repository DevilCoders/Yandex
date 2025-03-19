@dependent-scenarios
Feature: Internal API PostgreSQL Cluster upgrade from 13 to 14

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard 13 PostgreSQL cluster

  @setup
  Scenario: PostgreSQL cluster with version 13 and enabled feature flag creation works
    When we try to create cluster "test_cluster" with following config overrides
    """
    configSpec:
        version: '13'
        postgresqlConfig_13:
          logMinDurationStatement: 1000
          maxPreparedTransactions: 5
          sharedBuffers: 134217728
          sharedPreloadLibraries: ["SHARED_PRELOAD_LIBRARIES_AUTO_EXPLAIN", "SHARED_PRELOAD_LIBRARIES_PG_HINT_PLAN", "SHARED_PRELOAD_LIBRARIES_TIMESCALEDB", "SHARED_PRELOAD_LIBRARIES_PG_QUALSTATS", "SHARED_PRELOAD_LIBRARIES_PG_CRON"]
    databaseSpecs:
      - name: testdb
        owner: test_user
        extensions:
          - name: pgvector
          - name: pg_trgm
          - name: uuid-ossp
          - name: bloom
          - name: dblink
          - name: postgres_fdw
          - name: oracle_fdw
          - name: orafce
          - name: tablefunc
          - name: pg_repack
          - name: autoinc
          - name: pgrowlocks
          - name: hstore
          - name: cube
          - name: citext
          - name: earthdistance
          - name: btree_gist
          - name: xml2
          - name: isn
          - name: fuzzystrmatch
          - name: ltree
          - name: btree_gin
          - name: intarray
          - name: moddatetime
          - name: lo
          - name: pgcrypto
          - name: dict_xsyn
          - name: seg
          - name: unaccent
          - name: dict_int
          - name: jsquery
          - name: pg_partman
          - name: postgis
          - name: postgis_topology
          - name: postgis_tiger_geocoder
          - name: pgrouting
          - name: address_standardizer
          - name: pgstattuple
          - name: pg_buffercache
          - name: pg_hint_plan
          - name: timescaledb
          - name: rum
          - name: plv8
          - name: hypopg
          - name: pg_qualstats
          - name: pg_cron
        lcCollate: fi_FI.UTF-8
        lcCtype: fi_FI.UTF-8
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    When we attempt to add host in "vla" to "test_cluster" replication_source "sas"
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And host in geo "sas" in "test_cluster" has "1" replics
    And cluster has no pending changes

  Scenario: Upgrade PostgreSQL cluster with prepared transactions is reverted
    Given cluster "test_cluster" is up and running
    When we execute SQL on master in "test_cluster" without results
    """
    BEGIN;
    PREPARE TRANSACTION 'test_tpc';
    """
    And we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
        version: '14'
    """
    Then response should have status 200
    And generated task is failed within "15 minutes" with error message "The source cluster contains prepared transactions"
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT setting::int / 100 AS version FROM pg_settings WHERE name = 'server_version_num';
    """
    And query result is like
    """
    - version: 1300
    """
    And host in geo "sas" in "test_cluster" has "1" replics
    And cluster has no pending changes
    And following SQL request on master in "test_cluster" succeeds without results
    """
    ROLLBACK PREPARED 'test_tpc';
    """

  Scenario: Upgrade PostgreSQL cluster to version 14 works
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
        version: '14'
    """
    Then response should have status 200
    And generated task is finished within "14 minutes"
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT setting::int / 100 AS version FROM pg_settings WHERE name = 'server_version_num';
    """
    And query result is like
    """
    - version: 1400
    """
    And host in geo "sas" in "test_cluster" has "1" replics
    And cluster has no pending changes
