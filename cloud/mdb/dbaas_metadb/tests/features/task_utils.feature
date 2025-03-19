Feature: Task utils
    . When I am on a duty
    . I should be able to simply restart failed tasks.
    . But I do not want break something

  Background: All our base stuff
    Given default database
    And cloud with default quota
    And folder
    And cluster with
    """
    env: qa
    type: mongodb_cluster
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

  Scenario: Restart not existed tasks
    . When I am on a duty,
    . I can copy-paste different uuid
    When I execute query
    """
    SELECT * FROM code.restart_task('11111111-1111-1111-1111-111111111111')
    """
    Then it fail with error matches "Unable to restart task, probably 11111111-1111-1111-1111-111111111111 is not task_id"


  Scenario: Restart running task
    . When I am on duty,
    . I can copy-paste wrong task_id,
    . or my college can restart this task
    Given successfully executed query
    """
    SELECT * FROM code.acquire_task(
        i_worker_id => 'Billy',
        i_task_id   => :task_id
    )
    """
    When I execute query
    """
    SELECT * FROM code.restart_task(:task_id)
    """
    Then it fail with error matches "This task [\w-]+ is not in terminal state, it RUNNING"


  Scenario Outline: Restart task in terminal state
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
        i_result    => <result>,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    When I execute query
    """
    SELECT * FROM code.restart_task(
        i_task_id => :task_id,
        i_force   => <force>
    )
    """
    Then it success
    When I execute query
    """
    SELECT code.task_status(t) AS status
      FROM dbaas.worker_queue t
     WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    status: PENDING
    """

  Examples:
    | result | force |
    | true   | true  |
    | false  | false |
