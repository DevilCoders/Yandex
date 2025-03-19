Feature: Unmanaged operations

  Background: All our operations and require cluster
    Given default database
    And cloud with default quota
    And folder
    And cluster with
    """
    env: qa
    type: hadoop_cluster
    """


  Scenario: Add unmanaged operation
    Given task_id
    And expected row "new_unmanaged_operation"
    """
    operation_id: {{ task_id }}
    cid: {{ cid }}
    cluster_type: hadoop_cluster
    env: qa
    operation_type: hadoop_job_create
    created_by: tester
    status: RUNNING
    """
    When I execute cluster change
    """
    SELECT * FROM code.add_unmanaged_operation(
      i_operation_id   => :task_id,
      i_folder_id      => :folder_id,
      i_cid            => :cid,
      i_task_type      => 'hadoop_job_create',
      i_operation_type => 'hadoop_job_create',
      i_task_args      => '{}',
      i_metadata       => '{}',
      i_user_id        => 'tester',
      i_version        => 42,
      i_rev            => :rev
    )
    """
    Then it returns one row matches "new_unmanaged_operation"
    When I execute query
    """
    SELECT unmanaged FROM dbaas.worker_queue
     WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    unmanaged: true
    """
    When I execute query
    """
    SELECT * FROM code.acquire_task(
      i_worker_id => 'Alice',
      i_task_id   => :task_id
    )
    """
    Then it fail with error matches "Unable to acquire task [\w-]+, this task is unmanaged"
    When I execute query
    """
    SELECT * FROM code.finish_task(
      i_worker_id => 'Alice',
      i_task_id   => :task_id,
      i_result    => true,
      i_changes   => '{}',
      i_comment   => ''
    )
    """
    Then it fail with error matches "Unable to finish task [\w-]+, this task is unmanaged"

  Scenario Outline: Add unmanaged operation with tracing
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_unmanaged_operation(
        i_operation_id     => :task_id,
        i_folder_id       => :folder_id,
        i_cid              => :cid,
        i_task_type        => 'hadoop_job_create',
        i_operation_type   => 'hadoop_job_create',
        i_task_args        => '{}',
        i_metadata         => '{}',
        i_user_id          => 'tester',
        i_version          => 42,
        i_rev              => :rev,
        i_tracing          => <tracing>
    )
    """
    When I execute query
    """
    SELECT tracing FROM dbaas.worker_queue WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    tracing: <tracing>
    """

  Examples:
      | tracing                   |
      | NULL                      |
      | '{"xyz-trace-id": "111"}' |
