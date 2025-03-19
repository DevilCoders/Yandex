Feature: maintenance_window

  Background: Database with cluster
    Given default database
    And cloud with default quota
    And folder
    And cluster

  Scenario: Set maintenance window settings
    Given "postgres" cluster with
    """
    name: Postgres
    """
    When I execute "postgres" cluster change
    """
    SELECT code.set_maintenance_window_settings(
        i_cid       => :postgres_cid,
        i_day       => 'MON',
        i_hour      => 2,
        i_rev       => :rev
    )
    """
    Then it success
    When I execute query
    """
    SELECT *
    FROM dbaas.maintenance_window_settings
    WHERE cid = :postgres_cid;
    """
    Then it returns one row matches
    """
    cid: {{ postgres_cid }}
    day: 'MON'
    hour: 2
    """
    When I execute "postgres" cluster change
    """
    SELECT code.set_maintenance_window_settings(
        i_cid       => :postgres_cid,
        i_day       => NULL,
        i_hour      => 2,
        i_rev       => :rev
    )
    """
    Then it fail with error matches "unable to update maintenance settings"
    When I execute query
    """
    SELECT *
    FROM dbaas.maintenance_window_settings
    WHERE cid = :postgres_cid;
    """
    Then it returns one row matches
    """
    cid: {{ postgres_cid }}
    day: 'MON'
    hour: 2
    """
    When I execute "postgres" cluster change
    """
    SELECT code.set_maintenance_window_settings(
        i_cid       => :postgres_cid,
        i_day       => NULL,
        i_hour      => NULL,
        i_rev       => :rev
    )
    """
    Then it success
    When I execute query
    """
    SELECT *
    FROM dbaas.maintenance_window_settings
    WHERE cid = :postgres_cid;
    """
    Then it returns nothing


  Scenario: Plan maintenance task with changes
    Given "clickhouse" cluster with
    """
    name: Clickhouse Initial
    """
    Given task_id
    And new transaction
    When I lock future cluster
    And I execute in transaction
    """
    UPDATE dbaas.clusters
      SET name='Clickhouse Maintained'
    WHERE cid=:clickhouse_cid
    """
    And I execute in transaction
    """
    SELECT code.plan_maintenance_task(
        i_task_id        => :task_id,
        i_cid            => :clickhouse_cid,
        i_config_id      => 'clickhouse_minor_upgrade',
        i_folder_id      => :folder_id,
        i_operation_type => 'clickhouse_cluster_modify',
        i_task_type      => 'clickhouse_cluster_maintenance',
        i_task_args      => '{}',
        i_version        => 2,
        i_metadata       => '{}',
        i_user_id        => 'maintenance',
        i_rev            => :actual_rev,
        i_target_rev     => :next_rev,
        i_plan_ts        => now() + '1 week'::interval,
        i_info           => 'Upgrade Clickhouse',
        i_max_delay      => now() + '3 week'::interval
    )
    """
    And I complete future cluster change
    And I commit transaction
    When I execute query
    """
    SELECT *
    FROM dbaas.clusters
    WHERE cid=:clickhouse_cid
    """
    Then it returns one row matches
    """
    name: Clickhouse Initial
    """
    When I execute query
    """
    SELECT *
    FROM dbaas.clusters_revs
    WHERE cid=:clickhouse_cid AND rev=:next_rev
    """
    Then it returns one row matches
    """
    name: Clickhouse Maintained
    """
    When I execute query
    """
    SELECT *
    FROM dbaas.worker_queue
    WHERE task_id=:task_id
    """
    Then it returns one row matches
    """
    cid: {{ clickhouse_cid }}
    created_by: maintenance
    target_rev: {{ next_rev }}
    """


  Scenario: Plan maintenance task and reject it on user changes
    Given "clickhouse" cluster with
    """
    name: Clickhouse Initial
    """
    And task_id
    And new transaction
    When I lock future cluster
    And I execute in transaction
    """
    UPDATE dbaas.clusters
      SET name='Clickhouse Maintained'
    WHERE cid=:clickhouse_cid
    """
    And I execute in transaction
    """
    SELECT code.plan_maintenance_task(
        i_task_id        => :task_id,
        i_cid            => :clickhouse_cid,
        i_config_id      => 'clickhouse_minor_upgrade',
        i_folder_id      => :folder_id,
        i_operation_type => 'clickhouse_cluster_modify',
        i_task_type      => 'clickhouse_cluster_maintenance',
        i_task_args      => '{}',
        i_version        => 2,
        i_metadata       => '{}',
        i_user_id        => 'maintenance',
        i_rev            => :actual_rev,
        i_target_rev     => :next_rev,
        i_plan_ts        => now() + '1 week'::interval,
        i_info           => 'Upgrade Clickhouse',
        i_max_delay      => now() + '3 week'::interval
    )
    """
    And I complete future cluster change
    And I commit transaction
    When I execute query
    """
    SELECT *
    FROM dbaas.clusters
    WHERE cid=:clickhouse_cid
    """
    Then it returns one row matches
    """
    name: Clickhouse Initial
    """
    When I execute query
    """
    SELECT *
    FROM dbaas.worker_queue
    WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    cid: {{ clickhouse_cid }}
    created_by: maintenance
    target_rev: {{ next_rev }}
    """
    When I execute cluster change
    """
    UPDATE dbaas.clusters
      SET name='Clickhouse User'
    WHERE cid=:clickhouse_cid
    """
    Then it success
    When I execute query
    """
    SELECT *
    FROM dbaas.clusters
    WHERE cid=:clickhouse_cid
    """
    Then it returns one row matches
    """
    name: Clickhouse User
    """
    When I execute query
    """
    SELECT *
    FROM dbaas.worker_queue
    WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    cid: {{ clickhouse_cid }}
    result: false
    target_rev: {{ next_rev }}
    """
    Given task_id
    And new transaction
    When I lock future cluster
    And I execute in transaction
    """
    UPDATE dbaas.clusters
      SET name='Clickhouse Another Future'
    WHERE cid=:clickhouse_cid
    """
    And I execute in transaction
    """
    SELECT code.plan_maintenance_task(
        i_task_id        => :task_id,
        i_cid            => :clickhouse_cid,
        i_config_id      => 'clickhouse_minor_upgrade',
        i_folder_id      => :folder_id,
        i_operation_type => 'clickhouse_cluster_modify',
        i_task_type      => 'clickhouse_cluster_maintenance',
        i_task_args      => '{}',
        i_version        => 2,
        i_metadata       => '{}',
        i_user_id        => 'maintenance',
        i_rev            => :actual_rev,
        i_target_rev     => :next_rev,
        i_plan_ts        => now() + '1 week'::interval,
        i_info           => 'Upgrade Clickhouse',
        i_max_delay      => now() + '3 week'::interval
    )
    """
    And I complete future cluster change
    And I commit transaction
    When I execute query
    """
    SELECT *
    FROM dbaas.clusters
    WHERE cid=:clickhouse_cid
    """
    Then it returns one row matches
    """
    name: Clickhouse User
    """
    When I execute query
    """
    SELECT *
    FROM dbaas.worker_queue
    WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    cid: {{ clickhouse_cid }}
    created_by: maintenance
    target_rev: {{ next_rev }}
    result: NULL
    """

  Scenario: Plan maintenance task and finish it
    Given "clickhouse" cluster with
    """
    name: Clickhouse Initial
    """
    And task_id
    And new transaction
    When I add "clickhouse_cluster_create" task
    And that task acquired by worker
    And that task finished by worker with result = true
    Given task_id
    When I lock future cluster
    And I execute in transaction
    """
    UPDATE dbaas.clusters
      SET name='Clickhouse Maintained'
    WHERE cid=:clickhouse_cid
    """
    And I execute in transaction
    """
    SELECT code.plan_maintenance_task(
        i_task_id        => :task_id,
        i_cid            => :clickhouse_cid,
        i_config_id      => 'clickhouse_minor_upgrade',
        i_folder_id      => :folder_id,
        i_operation_type => 'clickhouse_cluster_modify',
        i_task_type      => 'clickhouse_cluster_maintenance',
        i_task_args      => '{}',
        i_version        => 2,
        i_metadata       => '{}',
        i_user_id        => 'maintenance',
        i_rev            => :actual_rev,
        i_target_rev     => :next_rev,
        i_plan_ts        => now() + '1 week'::interval,
        i_info           => 'Upgrade Clickhouse',
        i_max_delay      => now() + '3 week'::interval
    )
    """
    And I complete future cluster change
    And I commit transaction
    When that task acquired by worker
    And I execute query
    """
    SELECT *
    FROM dbaas.clusters
    WHERE cid=:clickhouse_cid
    """
    Then it returns one row matches
    """
    status: MODIFYING
    """
    When I execute query
    """
    SELECT status FROM dbaas.maintenance_tasks WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    status: PLANNED
    """
    When that task finished by worker with result = true
    And I execute query
    """
    SELECT *
    FROM dbaas.clusters
    WHERE cid=:clickhouse_cid
    """
    Then it returns one row matches
    """
    name: Clickhouse Maintained
    status: RUNNING
    """
    When I execute query
    """
    SELECT status FROM dbaas.maintenance_tasks WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    status: COMPLETED
    """

  Scenario: Plan maintenance task and reschedule it
    Given "clickhouse" cluster with
    """
    name: Clickhouse Initial
    """
    And task_id
    And new transaction
    When I lock future cluster
    And I execute in transaction
    """
    UPDATE dbaas.clusters
      SET name='Clickhouse Maintained'
    WHERE cid=:clickhouse_cid
    """
    And I execute in transaction
    """
    SELECT code.plan_maintenance_task(
        i_task_id        => :task_id,
        i_cid            => :clickhouse_cid,
        i_config_id      => 'clickhouse_minor_upgrade',
        i_folder_id      => :folder_id,
        i_operation_type => 'clickhouse_cluster_modify',
        i_task_type      => 'clickhouse_cluster_maintenance',
        i_task_args      => '{}',
        i_version        => 2,
        i_metadata       => '{}',
        i_user_id        => 'maintenance',
        i_rev            => :actual_rev,
        i_target_rev     => :next_rev,
        i_plan_ts        => '2020-06-01T13:00:00+00:00'::timestamp,
        i_create_ts      => '2020-01-01T13:00:00+00:00'::timestamp,
        i_info           => 'Upgrade Clickhouse',
        i_max_delay      => now() + '3 week'::interval
    )
    """
    And I complete future cluster change
    And I commit transaction
    And I execute query
    """
    SELECT status FROM dbaas.maintenance_tasks
    WHERE cid = :clickhouse_cid
          AND config_id = 'clickhouse_minor_upgrade'
          AND plan_ts > '2020-05-01T13:00:00+00:00'::timestamp
    """
    Then it returns one row matches
    """
    status: PLANNED
    """
    When I execute query
    """
    SELECT code.reschedule_maintenance_task(
        i_cid       => :clickhouse_cid,
        i_config_id => 'clickhouse_minor_upgrade',
        i_plan_ts   => '2020-04-01T13:00:00+00:00'::timestamp
    )
    """
    Then it success
    When I execute query
    """
    SELECT status FROM dbaas.maintenance_tasks
    WHERE cid = :clickhouse_cid
          AND config_id = 'clickhouse_minor_upgrade'
          AND plan_ts = '2020-04-01T13:00:00+00:00'::timestamp
    """
    Then it returns one row matches
    """
    status: PLANNED
    """
    When I execute query
    """
    SELECT config_id
    FROM dbaas.worker_queue wq
    JOIN dbaas.maintenance_tasks mt USING(cid, config_id)
    WHERE wq.task_id = :task_id AND wq.delayed_until = mt.plan_ts
    """
    Then it returns one row matches
    """
    config_id: 'clickhouse_minor_upgrade'
    """
    When I execute query
    """
    SELECT *
    FROM dbaas.maintenance_tasks
    WHERE cid=:clickhouse_cid AND status='PLANNED'::dbaas.maintenance_task_status;
    """
    Then it returns one row matches
    """
    config_id: 'clickhouse_minor_upgrade'
    create_ts: 2020-01-01 13:00:00+00:00
    plan_ts: 2020-04-01 13:00:00+00:00
    """

Scenario: Set invalid maintenance window settings
    Given "postgres" cluster with
    """
    name: Postgres
    """
    When I execute "postgres" cluster change
    """
    SELECT code.set_maintenance_window_settings(
        i_cid       => :postgres_cid,
        i_day       => 'MON',
        i_hour      => 0,
        i_rev       => :rev
    )
    """
    Then it fail with error matches "new row for relation \"maintenance_window_settings\" violates check constraint \"maintenance_window_hour_min\""
    When I execute "postgres" cluster change
    """
    SELECT code.set_maintenance_window_settings(
        i_cid       => :postgres_cid,
        i_day       => 'MON',
        i_hour      => 25,
        i_rev       => :rev
    )
    """
    Then it fail with error matches "new row for relation \"maintenance_window_settings\" violates check constraint \"maintenance_window_hour_max\""

Scenario: Set boarder maintenance window settings
    Given "postgres" cluster with
    """
    name: Postgres
    """
    When I execute "postgres" cluster change
    """
    SELECT code.set_maintenance_window_settings(
        i_cid       => :postgres_cid,
        i_day       => 'MON',
        i_hour      => 1,
        i_rev       => :rev
    )
    """
    Then it success
    When I execute "postgres" cluster change
    """
    SELECT code.set_maintenance_window_settings(
        i_cid       => :postgres_cid,
        i_day       => 'MON',
        i_hour      => 24,
        i_rev       => :rev
    )
    """
    Then it success

  Scenario: Plan maintenance task and it is rejected
    Given "clickhouse" cluster with
    """
    name: Clickhouse Initial
    """
    And task_id
    And new transaction
    When I add "clickhouse_cluster_create" task
    And that task acquired by worker
    And that task finished by worker with result = true
    Given task_id
    When I lock future cluster
    And I execute in transaction
    """
    UPDATE dbaas.clusters
      SET name='Clickhouse Maintained'
    WHERE cid=:clickhouse_cid
    """
    And I execute in transaction
    """
    SELECT code.plan_maintenance_task(
        i_task_id        => :task_id,
        i_cid            => :clickhouse_cid,
        i_config_id      => 'clickhouse_minor_upgrade',
        i_folder_id      => :folder_id,
        i_operation_type => 'clickhouse_cluster_modify',
        i_task_type      => 'clickhouse_cluster_maintenance',
        i_task_args      => '{}',
        i_version        => 2,
        i_metadata       => '{}',
        i_user_id        => 'maintenance',
        i_rev            => :actual_rev,
        i_target_rev     => :next_rev,
        i_plan_ts        => now() + '1 week'::interval,
        i_info           => 'Upgrade Clickhouse',
        i_max_delay      => now() + '3 week'::interval
    )
    """
    And I complete future cluster change
    And I commit transaction
    When that task acquired by worker
    And I execute query
    """
    SELECT *
    FROM dbaas.clusters
    WHERE cid=:clickhouse_cid
    """
    Then it returns one row matches
    """
    status: MODIFYING
    """
    When I execute query
    """
    SELECT status FROM dbaas.maintenance_tasks WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    status: PLANNED
    """
    When I execute query
    """
    SELECT * FROM code.reject_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it success
    When I execute query
    """
    SELECT *
    FROM dbaas.clusters
    WHERE cid=:clickhouse_cid
    """
    Then it returns one row matches
    """
    name: Clickhouse Initial
    status: RUNNING
    """
    When I execute query
    """
    SELECT status FROM dbaas.maintenance_tasks WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    status: REJECTED
    """

  Scenario: Plan maintenance task and it fails
    Given "clickhouse" cluster with
    """
    name: Clickhouse Initial
    """
    And task_id
    And new transaction
    When I add "clickhouse_cluster_create" task
    And that task acquired by worker
    And that task finished by worker with result = true
    Given task_id
    When I lock future cluster
    And I execute in transaction
    """
    UPDATE dbaas.clusters
      SET name='Clickhouse Maintained'
    WHERE cid=:clickhouse_cid
    """
    And I execute in transaction
    """
    SELECT code.plan_maintenance_task(
        i_task_id        => :task_id,
        i_cid            => :clickhouse_cid,
        i_config_id      => 'clickhouse_minor_upgrade',
        i_folder_id      => :folder_id,
        i_operation_type => 'clickhouse_cluster_modify',
        i_task_type      => 'clickhouse_cluster_maintenance',
        i_task_args      => '{}',
        i_version        => 2,
        i_metadata       => '{}',
        i_user_id        => 'maintenance',
        i_rev            => :actual_rev,
        i_target_rev     => :next_rev,
        i_plan_ts        => now() + '1 week'::interval,
        i_info           => 'Upgrade Clickhouse',
        i_max_delay      => now() + '3 week'::interval
    )
    """
    And I complete future cluster change
    And I commit transaction
    When that task acquired by worker
    And I execute query
    """
    SELECT *
    FROM dbaas.clusters
    WHERE cid=:clickhouse_cid
    """
    Then it returns one row matches
    """
    status: MODIFYING
    """
    When I execute query
    """
    SELECT status FROM dbaas.maintenance_tasks WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    status: PLANNED
    """
    When I execute query
    """
    SELECT * FROM code.finish_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id,
        i_result    => false,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it success
    When I execute query
    """
    SELECT *
    FROM dbaas.clusters
    WHERE cid=:clickhouse_cid
    """
    Then it returns one row matches
    """
    name: Clickhouse Maintained
    status: MODIFY-ERROR
    """
    When I execute query
    """
    SELECT status FROM dbaas.maintenance_tasks WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    status: FAILED
    """
