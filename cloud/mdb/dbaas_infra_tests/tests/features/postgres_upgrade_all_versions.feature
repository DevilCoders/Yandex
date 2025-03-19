@dependent-scenarios
Feature: Internal API PostgreSQL Cluster upgrade from 10 to 14

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
  Scenario: PostgreSQL cluster with version 10 and enabled feature flag creation works
    Given feature flag "MDB_PG_ALLOW_DEPRECATED_VERSIONS" is set
    When we try to create cluster "test_cluster" with following config overrides
    """
    configSpec:
        version: '10'
        postgresqlConfig_10:
          logMinDurationStatement: 1000
          sharedBuffers: 134217728
          sharedPreloadLibraries: ["SHARED_PRELOAD_LIBRARIES_AUTO_EXPLAIN", "SHARED_PRELOAD_LIBRARIES_PG_HINT_PLAN", "SHARED_PRELOAD_LIBRARIES_PG_QUALSTATS", "SHARED_PRELOAD_LIBRARIES_PG_CRON"]
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
          - name: smlar
          - name: pg_partman
          - name: postgis
          - name: postgis_topology
          - name: postgis_tiger_geocoder
          - name: pgrouting
          - name: address_standardizer
          - name: pgstattuple
          - name: pg_buffercache
          - name: pg_hint_plan
          - name: pg_tm_aux
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

  Scenario: Upgrade PostgreSQL cluster to version 11 works
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
        version: '11'
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT setting::int / 100 AS version FROM pg_settings WHERE name = 'server_version_num';
    """
    And query result is like
    """
    - version: 1100
    """
    And host in geo "sas" in "test_cluster" has "1" replics
    And cluster has no pending changes
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
        version: '12'
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And following SQL request on master in "test_cluster" succeeds
    """
    SELECT setting::int / 100 AS version FROM pg_settings WHERE name = 'server_version_num';
    """
    And query result is like
    """
    - version: 1200
    """
    And host in geo "sas" in "test_cluster" has "1" replics
    And cluster has no pending changes
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
        version: '13'
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
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
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    configSpec:
        version: '14'
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
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
