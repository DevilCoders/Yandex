Feature: Operations and task

  Background: All our operations and require cluster
    Given default database
    And cloud with default quota
    And folder
    And cluster with
    """
    env: qa
    type: mongodb_cluster
    """


  Scenario: Add operation
    Given task_id
    And expected row "new_operation"
    """
    operation_id: {{ task_id }}
    cid: {{ cid }}
    cluster_type: mongodb_cluster
    env: qa
    operation_type: mongodb_cluster_create
    created_by: tester
    status: PENDING
    """
    When I execute cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    Then it returns one row matches "new_operation"
    When I execute query
    """
    SELECT * FROM code.get_operations(
        i_folder_id => :folder_id,
        i_cid        => NULL,
        i_limit      => 42
    )
    """
    Then it returns one row matches "new_operation"
    When I execute query
    """
    SELECT * FROM code.get_operations(
        i_folder_id => NULL,
        i_cid        => :cid,
        i_limit      => 42
    )
    """
    Then it returns one row matches "new_operation"


   Scenario: Add operation with idempotentce
    Given task_id
    And idempotence_id
    When I execute cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id     => :task_id,
        i_folder_id       => :folder_id,
        i_cid              => :cid,
        i_task_type        => 'mongodb_cluster_create',
        i_operation_type   => 'mongodb_cluster_create',
        i_task_args        => '{}',
        i_metadata         => '{}',
        i_user_id          => 'tester',
        i_idempotence_data => (:idempotence_id, '\x00'::bytea)::code.idempotence_data,
        i_version          => 42,
        i_rev              => :rev
    )
    """
    Then it success
    When I execute query
    """
    SELECT operation_id, request_hash::text FROM code.get_operation_id_by_idempotence(
        i_idempotence_id => :idempotence_id,
        i_folder_id     => :folder_id,
        i_user_id        => 'tester'
    )
    """
    Then it returns one row matches
    """
    operation_id: {{ task_id }}
    request_hash: '\x00'
    """

   Scenario: Add operation with empty request_hash in idempotence
    Given task_id
    And idempotence_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id     => :task_id,
        i_folder_id       => :folder_id,
        i_cid              => :cid,
        i_task_type        => 'mongodb_cluster_create',
        i_operation_type   => 'mongodb_cluster_create',
        i_task_args        => '{}',
        i_metadata         => '{}',
        i_user_id          => 'tester',
        i_idempotence_data => (:idempotence_id, NULL)::code.idempotence_data,
        i_version          => 42,
        i_rev              => :rev
    )
    """
    When I execute query
    """
    SELECT operation_id, request_hash::text FROM code.get_operation_id_by_idempotence(
        i_idempotence_id => :idempotence_id,
        i_folder_id     => :folder_id,
        i_user_id        => 'tester'
    )
    """
    Then it returns nothing

  Scenario Outline: Add operation with tracing
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id     => :task_id,
        i_folder_id       => :folder_id,
        i_cid              => :cid,
        i_task_type        => 'mongodb_cluster_create',
        i_operation_type   => 'mongodb_cluster_create',
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
    SELECT tracing FROM code.acquire_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id
    )
    """
    Then it returns one row matches
    """
    tracing: <tracing>
    """

  Examples:
      | tracing                   |
      | NULL                      |
      | '{"xyz-trace-id": "111"}' |

   @fin
   Scenario: Add finished operation
    Given task_id
    When I execute cluster change
    """
    SELECT * FROM code.add_finished_operation(
        i_operation_id     => :task_id,
        i_folder_id        => :folder_id,
        i_cid              => :cid,
        i_operation_type   => 'mongodb_cluster_modify',
        i_metadata         => '{}',
        i_user_id          => 'tester',
        i_version          => 42,
        i_rev              => :rev
    )
    """
    Then it returns one row matches
    """
    status: DONE
    operation_type: mongodb_cluster_modify
    """
    When I execute query
    """
    SELECT *
      FROM code.get_operation_by_id(
        i_folder_id    => :folder_id,
        i_operation_id => :task_id
    )
    """
    Then it returns one row matches
    """
    status: DONE
    operation_type: mongodb_cluster_modify
    """

   Scenario: Operation acquired by worker have status RUNNING
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    When I execute query
    """
    SELECT * FROM code.acquire_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id
    )
    """
    Then it returns one row matches
    """
    task_id: {{ task_id }}
    cid: {{ cid }}
    task_type: 'mongodb_cluster_create'
    """
    When I execute query
    """
    SELECT * FROM code.get_operations(
        i_folder_id => :folder_id,
        i_cid        => :cid,
        i_limit      => 42
    )
    """
    Then it returns one row matches
    """
    operation_type: mongodb_cluster_create
    status: RUNNING
    """


  Scenario Outline: Operation finished by worker
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.acquire_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id
    )
    """
    When I execute query
    """
    SELECT * FROM code.finish_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id,
        i_result    => <result>,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    When I execute query
    """
    SELECT * FROM code.get_operations(
        i_folder_id => :folder_id,
        i_cid        => :cid,
        i_limit      => 42
    )
    """
    Then it returns one row matches
    """
    operation_type: mongodb_cluster_create
    status: <status>
    """

  Examples:
    | result | status |
    | true   | DONE   |
    | false  | FAILED |


  Scenario: Worker try finished not acquired task
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    When I execute query
    """
    SELECT * FROM code.finish_task(
        i_worker_id => 'Craig',
        i_task_id   => :task_id,
        i_result    => true,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it fail with error matches "Unable to finish task [\w-]+, another worker NULL != 'Craig' acquire it"


  Scenario: Worker try finished not existed task
    Given task_id
    When I execute query
    """
    SELECT * FROM code.finish_task(
        i_worker_id => 'Craig',
        i_task_id   => :task_id,
        i_result    => true,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it fail with error matches "Unable to lock cluster by task [\w-]+. No task with that task_id found"


  Scenario: Another worker try finished task
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.acquire_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id
    )
    """
    When I execute query
    """
    SELECT * FROM code.finish_task(
        i_worker_id => 'Craig',
        i_task_id   => :task_id,
        i_result    => true,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it fail with error matches "Unable to finish task [\w-]+, another worker 'Alice' != 'Craig' acquire it"


  Scenario: Worker update task
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.acquire_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id
    )
    """
    When I execute query
    """
    SELECT * FROM code.update_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id,
        i_changes   => '{"state": "working"}',
        i_comment   => 'working hard'
    )
    """
    Then it success
    When I execute query
    """
    SELECT changes, comment
      FROM dbaas.worker_queue
     WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    changes: {'state': 'working'}
    comment: 'working hard'
    """


  Scenario: Worker try update not existed task
    Given task_id
    When I execute query
    """
    SELECT * FROM code.update_task(
        i_worker_id => 'Craig',
        i_task_id   => :task_id,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it fail with error matches "Unable to update task [\w-]+, cause no task with this id found"


  Scenario: Worker try update not acquired task
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    When I execute query
    """
    SELECT * FROM code.update_task(
        i_worker_id => 'Craig',
        i_task_id   => :task_id,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it fail with error matches "Unable to update task [\w-]+, another worker NULL != 'Craig' acquire it"


  Scenario: Worker try update task acquired by different worker
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.acquire_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id
    )
    """
    When I execute query
    """
    SELECT * FROM code.update_task(
        i_worker_id => 'Craig',
        i_task_id   => :task_id,
        i_changes   => '{}',
        i_comment   => ''
    )
    """
    Then it fail with error matches "Unable to update task [\w-]+, another worker 'Alice' != 'Craig' acquire it"


  Scenario: Worker can not reacquire task
    . for example when he process stale task
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.acquire_task(
        i_worker_id => 'Craig',
        i_task_id   => :task_id
    )
    """
    When I execute query
    """
    SELECT * FROM code.acquire_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id
    )
    """
    Then it fail with error matches "Unable to acquire task [\w-]+, another worker 'Craig' != 'Alice' acquire it"


  Scenario: Add hidden operation
    Given task_id
    And expected row "new_operation"
    """
    operation_id: {{ task_id }}
    cid: {{ cid }}
    cluster_type: mongodb_cluster
    env: qa
    operation_type: mongodb_cluster_create
    created_by: tester
    status: PENDING
    hidden: true
    """
    When I execute cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_hidden         => true,
        i_version        => 42,
        i_rev            => :rev
    )
    """
    Then it returns one row matches "new_operation"
    When I execute query
    """
    SELECT * FROM code.get_operations(
        i_folder_id      => :folder_id,
        i_cid            => NULL,
        i_limit          => 42
    )
    """
    Then it returns
    """
    []
    """
    When I execute query
    """
    SELECT * FROM code.get_operations(
        i_folder_id      => :folder_id,
        i_cid            => NULL,
        i_limit          => 42,
        i_include_hidden => true
    )
    """
    Then it returns one row matches "new_operation"
    When I execute query
    """
    SELECT * FROM code.get_operations(
        i_folder_id => NULL,
        i_cid        => :cid,
        i_limit      => 42
    )
    """
    Then it returns
    """
    []
    """
    When I execute query
    """
    SELECT * FROM code.get_operations(
        i_folder_id => NULL,
        i_cid        => :cid,
        i_limit      => 42,
        i_include_hidden => true
    )
    """
    Then it returns one row matches "new_operation"


  Scenario: Acquire task trasition
    Given task_id
    And "mongodb_cluster_create" task
    And that task acquired by worker
    And that task finished by worker with result = true
    And vars "delete_task_id, purge_task_id"
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :delete_task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_delete',
        i_operation_type => 'mongodb_cluster_delete',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :purge_task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_purge',
        i_operation_type => 'mongodb_cluster_delete',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_hidden         => true,
        i_delay_by       => '10 days',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    When I execute query
    """
    SELECT code.acquire_transition_exists(q)
      FROM dbaas.worker_queue q
     WHERE task_id = :purge_task_id
    """
    Then it returns
    """
    - [ false ]
    """
    When "delete_task_id" acquired by worker
    When I execute query
    """
    SELECT code.acquire_transition_exists(q)
      FROM dbaas.worker_queue q
     WHERE task_id = :purge_task_id
    """
    Then it returns
    """
    - [ false ]
    """
    When "delete_task_id" finished by worker with result = true
    And I execute query
    """
    SELECT code.acquire_transition_exists(q)
      FROM dbaas.worker_queue q
     WHERE task_id = :purge_task_id
    """
    Then it returns
    """
    - [ true ]
    """


  Scenario: Add operation with dep
    Given task_id
    And required_task_id
    And expected row "new_operation"
    """
    operation_id: {{ task_id }}
    cid: {{ cid }}
    cluster_type: mongodb_cluster
    env: qa
    operation_type: mongodb_cluster_create
    created_by: tester
    status: PENDING
    hidden: false
    required_operation_id: {{ required_task_id }}
    """
    Given successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :required_task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    When I execute cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id          => :task_id,
        i_folder_id             => :folder_id,
        i_cid                   => :cid,
        i_task_type             => 'mongodb_cluster_create',
        i_operation_type        => 'mongodb_cluster_create',
        i_task_args             => '{}',
        i_metadata              => '{}',
        i_user_id               => 'tester',
        i_version               => 42,
        i_required_operation_id => :required_task_id,
        i_rev                   => :rev
    )
    """
    Then it returns one row matches "new_operation"
    When I execute query
    """
    SELECT * FROM code.get_operations(
        i_folder_id      => :folder_id,
        i_cid            => NULL,
        i_limit          => 42
    )
    """
    When I execute query
    """
    SELECT * FROM code.acquire_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id
    )
    """
    Then it fail with error matches "Unable to acquire task [\w-]+, because required task [\w-]+ has result <NULL>"


  Scenario: Worker try release not acquired task
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    When I execute query
    """
    SELECT * FROM code.release_task(
        i_worker_id => 'Craig',
        i_task_id   => :task_id,
        i_context   => '{}'
    )
    """
    Then it fail with error matches "Unable to release task [\w-]+, another worker NULL != 'Craig' acquire it"

  Scenario: Tasks finish resets cancelled flag
    Given task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => 'mongodb_cluster_create',
        i_operation_type => 'mongodb_cluster_create',
        i_task_args      => '{}',
        i_metadata       => '{}',
        i_user_id        => 'tester',
        i_version        => 42,
        i_rev            => :rev
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.acquire_task(
        i_worker_id => 'Alice',
        i_task_id   => :task_id
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.cancel_task(
        i_task_id   => :task_id
    )
    """
    When I execute query
    """
    SELECT cancelled FROM dbaas.worker_queue WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    cancelled: true
    """
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
    And I execute query
    """
    SELECT cancelled FROM dbaas.worker_queue WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    cancelled: NULL
    """
