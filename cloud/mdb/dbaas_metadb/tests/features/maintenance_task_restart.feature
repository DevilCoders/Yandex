Feature: Maintenance task restart
    Background: All our base stuff
        Given default database
        And cloud with default quota
        And folder
        And cluster with
        """
        env: qa
        type: mysql_cluster
        """
        And task_id
        And successfully executed cluster change
        """
        SELECT * FROM code.add_operation(
            i_operation_id     => :task_id,
            i_folder_id        => :folder_id,
            i_cid              => :cid,
            i_task_type        => 'mongodb_cluster_create',
            i_operation_type   => 'mongodb_cluster_create',
            i_task_args        => '{}',
            i_metadata         => '{}',
            i_user_id          => 'tester',
            i_idempotence_data => (NULL, NULL)::code.idempotence_data,
            i_version          => 42,
            i_rev              => :rev
        )
        """
        And successfully executed query
        """
        SELECT * FROM code.acquire_task(
            i_worker_id => 'Billy',
            i_task_id   => :task_id
        )
        """
        And successfully executed query
        """
        SELECT * FROM code.finish_task(
            i_worker_id => 'Billy',
            i_task_id   => :task_id,
            i_result    => true,
            i_changes   => '{}',
            i_comment   => ''
        )
        """
        And task_id
        And new transaction
        When I lock future cluster
        And I execute in transaction
        """
        SELECT code.plan_maintenance_task(
            i_task_id        => :task_id,
            i_cid            => :cid,
            i_config_id      => 'mysql_8_0_update_tls_certs',
            i_folder_id      => :folder_id,
            i_operation_type => 'mysql_cluster_modify',
            i_task_type      => 'mysql_cluster_maintenance',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'mdb-maintainer',
            i_rev            => :actual_rev,
            i_target_rev     => :next_rev,
            i_plan_ts        => now() + '1 week'::interval,
            i_info           => 'Updating tls certificates',
            i_max_delay      => now() + '3 week'::interval
        )
        """
        Then it success
        When I complete future cluster change
        And I commit transaction

    Scenario: Allow restart task only failed maintenance
        Given successfully executed query
        """
        SELECT * FROM code.acquire_task(
            i_worker_id => 'Billy',
            i_task_id   => :task_id
        )
        """
        And successfully executed query
        """
        SELECT * FROM code.finish_task(
            i_worker_id => 'Billy',
            i_task_id   => :task_id,
            i_result    => false,
            i_changes   => '{}',
            i_comment   => ''
        )
        """

        When I execute query
        """
        SELECT status FROM dbaas.maintenance_tasks where task_id = :task_id
        """
        Then It returns one row matches
        """
        status: FAILED
        """

        When I execute query
        """
        SELECT * FROM code.restart_task(
            i_task_id => :task_id,
            i_force   => false
        )
        """
        Then it success

    Scenario Outline: Forbid restart task non-failed maintenance
        Given successfully executed query
        """
        SELECT * FROM code.acquire_task(
            i_worker_id => 'Billy',
            i_task_id   => :task_id
        )
        """
        And successfully executed query
        """
        SELECT * FROM code.finish_task(
            i_worker_id => 'Billy',
            i_task_id   => :task_id,
            i_result    => false,
            i_changes   => '{}',
            i_comment   => ''
        )
        """
        And successfully executed query
        """
        UPDATE dbaas.maintenance_tasks SET status = '<state>' where task_id = :task_id
        """
        When I execute query
        """
        SELECT * FROM code.restart_task(
            i_task_id => :task_id,
            i_force   => <force>
        )
        """
        Then it fail with error matches "Restarting a maintenance task is allowed only for active maintenance in \"FAILED\" status"

        Examples:
            | state     | force |
            | PLANNED   | false |
            | CANCELED  | false |
            | COMPLETED | false |
            | REJECTED  | false |

    Scenario Outline: Allow restart any maintenance task if the force flag be with you
        Given successfully executed query
        """
        SELECT * FROM code.acquire_task(
            i_worker_id => 'Billy',
            i_task_id   => :task_id
        )
        """
        And successfully executed query
        """
        SELECT * FROM code.finish_task(
            i_worker_id => 'Billy',
            i_task_id   => :task_id,
            i_result    => false,
            i_changes   => '{}',
            i_comment   => ''
        )
        """
        And successfully executed query
        """
        UPDATE dbaas.maintenance_tasks SET status = '<state>' where task_id = :task_id
        """
        When I execute query
        """
        SELECT * FROM code.restart_task(
            i_task_id => :task_id,
            i_force   => <force>
        )
        """
        Then it success

        Examples:
            | state     | force |
            | PLANNED   | true  |
            | CANCELED  | true  |
            | COMPLETED | true  |
            | REJECTED  | true  |
            | FAILED    | true  |
