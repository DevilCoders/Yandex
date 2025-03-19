@offline
Feature: Offline clusters maintenance

    Scenario: Maintain stopped PostgreSQL cluster
        Given stopped postgresql cluster with name "name"
        And "pg_mw" config
        """
        description: PG MW
        clusters_selection:
          db: >
              SELECT cid
                FROM dbaas.clusters
               WHERE type = 'postgresql_cluster'
        pillar_change: >
          SELECT 1
        supports_offline: true
        """
        When I maintain "pg_mw" config
        Then there is one maintenance task
        And exists "pg_mw" maintenance task on "cid1" in "PLANNED" status
