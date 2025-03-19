Feature: Task rejection
  Background:
    Given default database
    And cloud with default quota
    And folder
    And cluster with
    """
    name: initial
    env: qa
    type: postgresql_cluster
    """
    And task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id     => :task_id || '-safe',
        i_folder_id        => :folder_id,
        i_cid              => :cid,
        i_task_type        => 'postgresql_cluster_create',
        i_operation_type   => 'postgresql_cluster_create',
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
        i_task_id   => :task_id || '-safe'
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.finish_task(
        i_worker_id => 'Billy',
        i_task_id   => :task_id || '-safe',
        i_result    => true,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    And successfully executed cluster change
    """
    SELECT * FROM code.update_cluster_name(
        i_cid  => :cid,
        i_rev  => :rev,
        i_name => 'changed'
    )
    """
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id     => :task_id || '-unsafe',
        i_folder_id        => :folder_id,
        i_cid              => :cid,
        i_task_type        => 'postgresql_cluster_modify',
        i_operation_type   => 'postgresql_cluster_modify',
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
        i_task_id   => :task_id || '-unsafe'
    )
    """

  Scenario: Reject non-existing task
    When I execute query
    """
    SELECT * FROM code.reject_task(
        i_worker_id => 'Billy',
        i_task_id   => '11111111-1111-1111-1111-111111111111',
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it fail with error matches "Unable to lock cloud for task 11111111-1111-1111-1111-111111111111"

  Scenario: Reject task in terminal state
    When I execute query
    """
    SELECT * FROM code.reject_task(
        i_worker_id => 'Billy',
        i_task_id   => :task_id || '-safe',
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it fail with error matches "This task [\w-]+ is in terminal state, it is DONE"

  Scenario: Reject restarted task
    Given successfully executed query
    """
    SELECT * FROM code.finish_task(
        i_worker_id => 'Billy',
        i_task_id   => :task_id || '-unsafe',
        i_result    => true,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.restart_task(
        i_task_id   => :task_id || '-unsafe',
        i_force     => true
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.acquire_task(
        i_worker_id => 'Billy',
        i_task_id   => :task_id || '-unsafe'
    )
    """
    When I execute query
    """
    SELECT * FROM code.reject_task(
        i_worker_id => 'Billy',
        i_task_id   => :task_id || '-unsafe',
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it fail with error matches "This task [\w-]+ has non-zero restart count: 1"

  Scenario: Reject running task
    When I execute query
    """
    SELECT * FROM code.reject_task(
        i_worker_id => 'Billy',
        i_task_id   => :task_id || '-unsafe',
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it success
    When I execute query
    """
    SELECT c.status,
           c.name
      FROM dbaas.clusters c
           JOIN dbaas.worker_queue q ON (c.cid = q.cid)
     WHERE c.cid = :cid
           AND q.task_id = :task_id || '-unsafe'
           AND q.finish_rev = c.actual_rev
    """
    Then it returns one row matches
    """
    name: initial
    status: RUNNING
    """

  Scenario: Reject failed task
    Given successfully executed query
    """
    SELECT * FROM code.finish_task(
        i_worker_id => 'Billy',
        i_task_id   => :task_id || '-unsafe',
        i_result    => false,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    When I execute query
    """
    SELECT * FROM code.reject_failed_task(
        i_task_id   => :task_id || '-unsafe'
    )
    """
    Then it success

  Scenario: Finish failed task
    Given successfully executed query
    """
    SELECT * FROM code.finish_task(
        i_worker_id => 'Billy',
        i_task_id   => :task_id || '-unsafe',
        i_result    => false,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    When I execute query
    """
    SELECT * FROM code.finish_failed_task(
        i_task_id   => :task_id || '-unsafe'
    )
    """
    Then it success
