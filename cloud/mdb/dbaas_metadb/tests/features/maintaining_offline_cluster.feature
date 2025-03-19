Feature: Maintaining offline cluster

    Background: Database with stopped cluster and planned MW task
        Given default database
        And cloud with default quota
        And folder
        Given "postgres" cluster with
        """
        name: Postgres
        """
        And successfully executed query
        """
        UPDATE dbaas.clusters
           SET status = 'STOPPED'
        RETURNING cid
        """
        And task_id
        And new transaction
        When I lock future cluster
        And I execute in transaction
        """
        SELECT code.plan_maintenance_task(
            i_task_id        => :task_id,
            i_cid            => :cid,
            i_config_id      => 'postgresql_minor_upgrade',
            i_folder_id      => :folder_id,
            i_operation_type => 'postgresql_cluster_maintenance',
            i_task_type      => 'postgresql_cluster_maintenance',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'maintenance',
            i_rev            => :actual_rev,
            i_target_rev     => :next_rev,
            i_plan_ts        => now() + '1 week'::interval,
            i_info           => 'Upgrade PostgreSQL',
            i_max_delay      => now() + '3 week'::interval
        )
        """
        Then it success
        When I complete future cluster change
        And I commit transaction

    Scenario: Successful offline maintenance
        When that task acquired by worker
        Then cluster in "MAINTAINING-OFFLINE" status
        When that task finished by worker with result = true
        Then cluster in "STOPPED" status

    Scenario: Restart failed offline maintenance task
        When that task acquired by worker
        Then cluster in "MAINTAINING-OFFLINE" status
        When that task finished by worker with result = false
        Then cluster in "MAINTAIN-OFFLINE-ERROR" status
        When I execute query
        """
        SELECT code.restart_task(:task_id)
        """
        Then it success
        And cluster in "MAINTAIN-OFFLINE-ERROR" status
        When that task acquired by worker
        Then cluster in "MAINTAINING-OFFLINE" status
        When that task finished by worker with result = true
        Then cluster in "STOPPED" status

    Scenario: Release offline maintenance task
        . Here we check the case when worker restarted
        . during offline maintenance task.
        When that task acquired by worker
        And that task released by worker
        And I execute query
        """
        SELECT code.acquire_transition_exists(worker_queue) AS it_can_be_acquired
          FROM dbaas.worker_queue
         WHERE task_id = :task_id
        """
        Then it returns one row matches
        """
        it_can_be_acquired: true
        """
        When that task acquired by worker
        Then cluster in "MAINTAINING-OFFLINE" status

    Scenario: Delete cluster with failed offline maintenance
        Given vars "delete_task_id"
        When that task acquired by worker
        When that task finished by worker with result = false
        Then cluster in "MAINTAIN-OFFLINE-ERROR" status
        When I add "postgresql_cluster_delete" task with "delete_task_id" task_id
        Then cluster in "DELETING" status
