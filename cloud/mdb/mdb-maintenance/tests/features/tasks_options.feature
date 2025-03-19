Feature: Options
    Background: One cluster
        Given postgresql cluster with name "pgtest1" and "pgbouncer" connection_pooler

    Scenario: By default tasks have 1h timeout
        Given "mw_task" config
        """
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE NOT value ? 'maintained'
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{maintained}', to_jsonb(true))
           WHERE cid = :cid
        """
        When I maintain "mw_task" config
        Then there is one maintenance task
        And "cid1" maintenance task has timeout="1h"

    Scenario: Custom task timeout
        Given "mw_task" config
        """
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE NOT value ? 'maintained'
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{maintained}', to_jsonb(true))
           WHERE cid = :cid
        worker:
          timeout: 42h
        """
        When I maintain "mw_task" config
        Then there is one maintenance task
        And "cid1" maintenance task has timeout="42h"

    Scenario: Custom task timeoutQuery
        Given "mw_task" config
        """
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE NOT value ? 'maintained'
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{maintained}', to_jsonb(true))
           WHERE cid = :cid
        worker:
          timeout_query: >
            SELECT CONCAT(42, 'h')
        """
        When I maintain "mw_task" config
        Then there is one maintenance task
        And "cid1" maintenance task has timeout="42h"

    Scenario: Enabled environment ordering
        Given postgresql cluster with name "pgtest1_prod" and "PRODUCTION" environment and "pgbouncer" connection_pooler
        And "mw_task" config
        """
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
        pillar_change: >
          SELECT 1
        env_order_disabled: false
        """
        When I maintain "mw_task" config
        Then there are "1" maintenance tasks

    Scenario: Disabled environment ordering
        Given postgresql cluster with name "pgtest1_prod" and "PRODUCTION" environment and "pgbouncer" connection_pooler
        And "mw_task" config
        """
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
        pillar_change: >
          SELECT 1
        env_order_disabled: true
        """
        When I maintain "mw_task" config
        Then there are "2" maintenance tasks
