Feature: Maintainer
    Background: PostgreSQL cluster and MW task
        Given postgresql cluster with name "pgtest1" and "pgbouncer" connection_pooler
        And "pg_change_pgbouncer_to_odyssey" config
        """
        description: Change PGBouncer to Odyssey
        clusters_selection:
          db: >
              SELECT cid
                FROM dbaas.clusters c
                JOIN dbaas.pillar p
               USING (cid)
               WHERE type = 'postgresql_cluster'
                 AND value @> jsonb_build_object('data', jsonb_build_object('connection_pooler', 'pgbouncer'))
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        """

    Scenario: Maintain one config
        When I maintain "pg_change_pgbouncer_to_odyssey" config
        Then there is one maintenance task
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "PLANNED" status
        And there is last sent single notification to cloud "cloud1" for cluster "cid1"
        When I maintain "pg_change_pgbouncer_to_odyssey" config
        # it should success and shouldn't create new tasks
        But there is one maintenance task

    Scenario: Maintain one config for two clusters
        Given postgresql cluster with name "pgtest2" and "pgbouncer" connection_pooler
        When I maintain "pg_change_pgbouncer_to_odyssey" config
        Then there are "2" maintenance tasks
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "PLANNED" status
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid2" in "PLANNED" status
        And there is last sent multiple notification to cloud "cloud1"

    Scenario: Cluster should have only one 'active' MW task
        Given "pg_update_minor_version" config
        """
        description: Update PostgreSQL minor version
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
           WHERE type = 'postgresql_cluster'
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = value
           WHERE cid = :cid
        """
        When I maintain "pg_update_minor_version" config
        Then there is one maintenance task
        When I maintain "pg_change_pgbouncer_to_odyssey" config
        Then there is one maintenance task

    Scenario: Maintenance should touch only running clusters
        When I initiate host creation in postgresql cluster
        And I maintain "pg_change_pgbouncer_to_odyssey" config
        Then there are "0" maintenance tasks

    Scenario: Maintain one config with re-plan
        When I maintain "pg_change_pgbouncer_to_odyssey" config
        Then there is one maintenance task
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "PLANNED" status
        When I change description of cluster "cid1" to "cluster after change"
        Then exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "CANCELED" status
        When I maintain "pg_change_pgbouncer_to_odyssey" config
        Then there is one maintenance task
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "PLANNED" status

    Scenario: Re-plan rejected task
        When I maintain "pg_change_pgbouncer_to_odyssey" config
        Then there is one maintenance task
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "PLANNED" status
        And worker task for cluster "cid1" and config "pg_change_pgbouncer_to_odyssey" is acquired by worker
        Then maintenance task "pg_change_pgbouncer_to_odyssey" on "cid1" was REJECTED
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "REJECTED" status
        When I maintain "pg_change_pgbouncer_to_odyssey" config
        Then exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "PLANNED" status

    Scenario: Finish not actual PLANNED task
        Given "pg_test" config
        """
        description: Change pooler
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND true
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        """
        When I maintain "pg_test" config
        Then exists "pg_test" maintenance task on "cid1" in "PLANNED" status
        And "pg_test" config
        """
        description: Change pooler
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND false
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        """
        And I maintain "pg_test" config
        And exists "pg_test" maintenance task on "cid1" in "COMPLETED" status

    Scenario: Finish not actual CANCELED task
        Given "pg_test" config
        """
        description: Change pooler
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND true
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        """
        When I maintain "pg_test" config
        Then exists "pg_test" maintenance task on "cid1" in "PLANNED" status
        When I change description of cluster "cid1" to "cluster after change"
        And "pg_test" config
        """
        description: Change pooler
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND false
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        """
        Then exists "pg_test" maintenance task on "cid1" in "CANCELED" status
        And I maintain "pg_test" config
        And exists "pg_test" maintenance task on "cid1" in "COMPLETED" status

    Scenario: Second run leaves old maintenance planned
        When I maintain "pg_change_pgbouncer_to_odyssey" config
        Then there is one maintenance task
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "PLANNED" status
        When I maintain "pg_change_pgbouncer_to_odyssey" config
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "PLANNED" status

    Scenario: Do not cancel on not RUNNING cluster
        Given "pg_modify" config
        """
        description: Change PGBouncer to Odyssey
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND value @> jsonb_build_object('data', jsonb_build_object('connection_pooler', 'pgbouncer'))
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        worker:
          operation_type: postgresql_cluster_modify
          task_type: postgresql_cluster_maintenance
          task_args:
            restart: true
        """
        When I maintain "pg_modify" config
        Then there is one maintenance task
        And exists "pg_modify" maintenance task on "cid1" in "PLANNED" status
        And worker task for cluster "cid1" and config "pg_modify" is acquired by worker
        Then cluster "cid1" is in "MODIFYING" status
        When I maintain "pg_modify" config
        And exists "pg_modify" maintenance task on "cid1" in "PLANNED" status

    Scenario: Disable config
        When I maintain "pg_change_pgbouncer_to_odyssey" config
        Then there is one maintenance task
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "PLANNED" status
        When "pg_change_pgbouncer_to_odyssey" config
        """
        description: Change PGBouncer to Odyssey
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND value @> jsonb_build_object('data', jsonb_build_object('connection_pooler', 'pgbouncer'))
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        disabled: true
        """
        And I maintain "pg_change_pgbouncer_to_odyssey" config
        Then exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "COMPLETED" status

    Scenario: Don't plan new task if config is completed
        Given "pg_test" config
        """
        description: Change pooler
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND true
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        """
        When I maintain "pg_test" config
        Then exists "pg_test" maintenance task on "cid1" in "PLANNED" status
        And "pg_test" config
        """
        description: Change pooler
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND false
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        """
        And I maintain "pg_test" config
        And exists "pg_test" maintenance task on "cid1" in "COMPLETED" status
        When "pg_test" config
        """
        description: Change pooler
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND true
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        """
        And I maintain "pg_test" config
        # should not reschedule tasks for already completed config
        Then exists "pg_test" maintenance task on "cid1" in "COMPLETED" status

    Scenario: Maintain and repeat one config
        Given "pg_test" config
        """
        description: Change pooler
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND true
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        """
        When I maintain "pg_test" config
        Then exists "pg_test" maintenance task on "cid1" in "PLANNED" status
        And "pg_test" config
        """
        description: Change pooler
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND false
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        """
        And I maintain "pg_test" config
        And exists "pg_test" maintenance task on "cid1" in "COMPLETED" status
        When "pg_test" config
        """
        description: Change pooler
        clusters_selection:
         db: >
          SELECT cid
            FROM dbaas.clusters c
            JOIN dbaas.pillar p
           USING (cid)
           WHERE type = 'postgresql_cluster'
             AND true
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,connection_pooler}', to_jsonb(CAST('odyssey' AS text)))
           WHERE cid = :cid
        repeatable: true
        """
        And I maintain "pg_test" config
        Then exists "pg_test" maintenance task on "cid1" in "PLANNED" status
        And there are total "2" notifications to cloud "cloud1" for cluster "cid1"

    Scenario: Reschedule config if mw settings are changed
        When I set cluster "cid1" mw day to "MON", mw hour to "15"
        And I maintain "pg_change_pgbouncer_to_odyssey" config
        Then there is one maintenance task
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "PLANNED" status
        And Cluster "cid1" MW settings matches with task
        When I set cluster "cid1" mw day to "TUE", mw hour to "16"
        And I maintain "pg_change_pgbouncer_to_odyssey" config
        Then there is one maintenance task
        And exists "pg_change_pgbouncer_to_odyssey" maintenance task on "cid1" in "PLANNED" status
        And Cluster "cid1" MW settings matches with task
