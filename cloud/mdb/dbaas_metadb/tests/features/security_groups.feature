Feature: Security groups

  Background: All our operations and require cluster
    Given default database
    And cloud with default quota
    And folder
    And cluster

  Scenario: get_clusters with security groups
    When I execute query
    """
    SELECT *
      FROM code.get_clusters(
        i_folder_id => :folder_id,
        i_limit     => 42
    )
    """
    Then it returns one row matches
    """
    user_sgroup_ids: []
    """
    Given successfully executed query
    """
    INSERT INTO dbaas.sgroups
      (cid, sg_ext_id, sg_type)
    VALUES
      (:cid, 'u1', 'user'),
      (:cid, 'u2', 'user'),
      (:cid, 's2', 'service')
    """
    When I execute query
    """
    SELECT *
      FROM code.get_clusters(
        i_folder_id => :folder_id,
        i_limit     => 42
    )
    """
    Then it returns one row matches
    """
    user_sgroup_ids: ['u1', 'u2']
    """

  Scenario: lock_cluster with security groups
    Given new transaction
    When I execute in transaction
    """
    SELECT user_sgroup_ids
      FROM code.lock_cluster(:cid)
    """
    Then it returns one row matches
    """
    user_sgroup_ids: []
    """
    And I rollback transaction
    When I successfully execute query
    """
    INSERT INTO dbaas.sgroups
      (cid, sg_ext_id, sg_type)
    VALUES
      (:cid, 'u2', 'user'),
      (:cid, 'u1', 'user'),
      (:cid, 's2', 'service')
    """
    And I start transaction
    And I execute in transaction
    """
    SELECT user_sgroup_ids
      FROM code.lock_cluster(:cid)
    """
    Then it returns one row matches
    """
    user_sgroup_ids: ['u1', 'u2']
    """
    And I rollback transaction
