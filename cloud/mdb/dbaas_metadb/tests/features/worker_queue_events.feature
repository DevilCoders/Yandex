Feature: worker_queue_events 

  Background: Database with cluster
    Given default database
    And cloud with default quota
    And folder
    And cluster

  Scenario: Add to worker_queue_events
    Given task_id
    And "mongodb_cluster_create" task 
    When I execute query
    """
    INSERT INTO dbaas.worker_queue_events 
        (task_id, data)
    VALUES
        (:task_id, '{"new": "doc"}')
    """
    Then it success

  Scenario: Add to worker_queue_events for non existed task
    When I execute query
    """
    INSERT INTO dbaas.worker_queue_events 
        (task_id, data)
    VALUES
        ('Non-Existed-Task-Id', '{"new": "doc"}')
    """
    Then it fail with error matches "violates foreign key constraint"

  Scenario: Mark worker_queue_events
    Given task_id
    And "mongodb_cluster_create" task 
    And successfully executed query
    """
    INSERT INTO dbaas.worker_queue_events 
        (task_id, data)
    VALUES
        (:task_id, '{"new": "doc"}')
    """
    When I execute query
    """
    UPDATE dbaas.worker_queue_events
       SET start_sent_at = now()
     WHERE task_id = :task_id
    """
    Then it success
    When I execute query
    """
    UPDATE dbaas.worker_queue_events
       SET done_sent_at = now()
     WHERE task_id = :task_id
    """
    Then it success

  Scenario: Mark not started event as finished
    Given task_id
    And "mongodb_cluster_create" task 
    And successfully executed query
    """
    INSERT INTO dbaas.worker_queue_events 
        (task_id, data)
    VALUES
        (:task_id, '{"new": "doc"}')
    """
    When I execute query
    """
    UPDATE dbaas.worker_queue_events
       SET done_sent_at = now()
     WHERE task_id = :task_id
    """
    Then it fail with error matches "violates check constraint .check_only_started_events_can_be_done."
