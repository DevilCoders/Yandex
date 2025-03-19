Feature: worker_queue

  Background: Database with cluster
    Given default database
    And cloud with default quota
    And folder
    And cluster

  Scenario: Add to worker_queue without created_by sets it to default
    Given task_id
    And cluster
    When I execute query
    """
    INSERT INTO dbaas.worker_queue
        (task_id, cid, task_type, task_args, folder_id, operation_type, metadata, timeout, create_rev)
    VALUES
        (:task_id, :cid, 'task_type', '{}', :folder_id, 'op_type', '{}', '2h', 0)
    """
    And I execute query
    """
    SELECT created_by FROM dbaas.worker_queue WHERE task_id = :task_id
    """
    Then it returns one row matches
    """
    created_by: ""
    """
