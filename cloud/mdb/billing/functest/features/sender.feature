Feature: Billing sender works

  Scenario: Metrics sender works properly
    When I successfully execute query in billingdb
    """
    INSERT INTO billing.metrics_queue (batch_id, bill_type, seq_no, batch)
        VALUES
        ('00000000-0000-0000-0000-000000000001', 'BACKUP', 1, '{"payload": 1}'::bytea),
        ('00000000-0000-0000-0000-000000000002', 'BACKUP', 2, '{"payload": 2}'::bytea)
    """
    And all logbroker writes are successful
    And I execute mdb-billing-sender for billing type "BACKUP" with config
    """
    {max_tasks: 1}
    """
    And I execute query in billingdb
    """
    SELECT batch_id, restart_count, seq_no, finished_at IS NOT NULL as sent FROM billing.metrics_queue
    """
    Then it returns from billingdb "2" rows matches
    """
    - batch_id: 00000000-0000-0000-0000-000000000001
      restart_count: 0
      sent: true
      seq_no: 1
    - batch_id: 00000000-0000-0000-0000-000000000002
      restart_count: 0
      sent: false
      seq_no: 2
    """
    And I execute mdb-billing-sender for billing type "BACKUP" with config
    """
    {max_tasks: 1}
    """
    And I execute query in billingdb
    """
    SELECT batch_id, restart_count, seq_no, finished_at IS NOT NULL as sent FROM billing.metrics_queue
    """
    Then it returns from billingdb "2" rows matches
    """
    - batch_id: 00000000-0000-0000-0000-000000000001
      restart_count: 0
      sent: true
      seq_no: 1
    - batch_id: 00000000-0000-0000-0000-000000000002
      restart_count: 0
      sent: true
      seq_no: 2
    """

  Scenario: Metrics sender handles errors properly
    When I successfully execute query in billingdb
    """
    INSERT INTO billing.metrics_queue (batch_id, bill_type, seq_no, batch)
        VALUES
        ('00000000-0000-0000-0000-000000000001', 'BACKUP', 1, '{"payload": 1}'::bytea),
        ('00000000-0000-0000-0000-000000000002', 'BACKUP', 2, '{"payload": 2}'::bytea)
    """
    And all logbroker writes are failed
    And I execute mdb-billing-sender for billing type "BACKUP" with config
    """
    {max_tasks: 1}
    """
    And I execute query in billingdb
    """
    SELECT batch_id, restart_count, seq_no, finished_at IS NOT NULL as sent FROM billing.metrics_queue
    """
    Then it returns from billingdb "2" rows matches
    """
    - batch_id: 00000000-0000-0000-0000-000000000001
      restart_count: 1
      sent: false
      seq_no: 1
    - batch_id: 00000000-0000-0000-0000-000000000002
      restart_count: 0
      sent: false
      seq_no: 2
    """
    And all logbroker writes are successful
    And I execute mdb-billing-sender for billing type "BACKUP" with config
    """
    {max_tasks: 1}
    """
    And I execute query in billingdb
    """
    SELECT batch_id, restart_count, seq_no, finished_at IS NOT NULL as sent FROM billing.metrics_queue
    """
    Then it returns from billingdb "2" rows matches
    """
    - batch_id: 00000000-0000-0000-0000-000000000001
      restart_count: 1
      sent: true
      seq_no: 1
    - batch_id: 00000000-0000-0000-0000-000000000002
      restart_count: 0
      sent: false
      seq_no: 2
    """