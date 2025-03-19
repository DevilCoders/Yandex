Feature: Lock cluster

  Background: ...
    Given default database
    And cloud with default quota
    And folder
    And cluster

  Scenario: Lock cluster lock it
    . Here we can write concurrent test,
    . but we don't need that mechaincs in other cases.
    . So just look at the row locks.
    Given successfully executed query
      """
      CREATE EXTENSION IF NOT EXISTS pgrowlocks
      """
    When I start transaction
    And I execute in transaction
      """
      SELECT rev FROM code.lock_cluster(:cid)
      """
    Then it returns one row matches
      """
      rev: 2
      """
    When I execute query
      """
      SELECT modes
        FROM pgrowlocks('dbaas.clusters')
       WHERE locked_row = (
          SELECT ctid
            FROM dbaas.clusters
           WHERE cid = :cid)
      """
    Then it returns one row matches
      """
      modes: ['No Key Update']
      """
     And I rollback transaction

  Scenario: Lock cluster returns new revision
    When I start transaction
    And I execute in transaction
      """
      SELECT rev FROM code.lock_cluster(:cid)
      """
    Then it returns one row matches
      """
      rev: 2
      """
     And I rollback transaction

  Scenario: Lock cluster initiate cluster change
    When I start transaction
    And I execute in transaction
      """
      SELECT * FROM code.lock_cluster(:cid, 'foo.bar')
      """
    Then it success
    When I execute in transaction
      """
      SELECT *
        FROM dbaas.clusters_changes
       WHERE cid = :cid
         AND rev = 2
      """
    Then it returns one row matches
      """
      x_request_id: foo.bar
      changes: []
      """
     And I rollback transaction

  Scenario: Change can not be commited if complete_cluster_change not called
    . We allocate the revision for this change
    . and there are nothing usefull that we can do with it
    When I start transaction
    And I execute in transaction
      """
      SELECT * FROM code.lock_cluster(:cid, 'foo.bar')
      """
    And I commit transaction
    Then it fail with error matches "insert or update on table .clusters. violates foreign key constraint"

  Scenario: Change can be commited if complete called
    When I start transaction
    And I execute in transaction
      """
      SELECT * FROM code.lock_cluster(:cid, 'foo.bar')
      """
    Then it success
    When I execute in transaction
      """
      SELECT * FROM code.complete_cluster_change(:cid, 2)
      """
    Then it success
    When I commit transaction
    Then it success

