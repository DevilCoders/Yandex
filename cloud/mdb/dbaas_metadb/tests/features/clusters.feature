Feature: Clusters API

  Background: All our operations and require cluster
    Given default database
    And cloud with default quota
    And folder
    And cluster with
    """
    env: qa
    type: mongodb_cluster
    name: is web scale
    """

  Scenario: Get clusters in folder
    When I execute query
    """
    SELECT *
      FROM code.get_clusters(
        i_folder_id        => :folder_id,
        i_limit             => 42
    )
    """
    Then it returns one row matches
    """
    cid: {{ cid }}
    env: qa
    type: mongodb_cluster
    name: is web scale
    """

  Scenario Outline: Cluster status create transitions
    Given task_id
    When I execute query
    """
    SELECT status
      FROM code.get_clusters(
        i_folder_id        => :folder_id,
        i_limit             => 42
    )
    """
    Then it returns one row matches
    """
    status: CREATING
    """
    When I add "mongodb_cluster_create" task
    And that task acquired by worker
    And that task finished by worker with result = <worker_result>
    And I execute query
    """
    SELECT status
      FROM code.get_clusters(
        i_folder_id        => :folder_id,
        i_limit             => 42
    )
    """
    Then it returns one row matches
    """
    status: <cluster_status>
    """

  Examples:
    | worker_result | cluster_status |
    | true          | RUNNING        |
    | false         | CREATE-ERROR   |

  Scenario Outline: Cluster status stop transitions
    Given task_id
    When I execute query
    """
    UPDATE dbaas.clusters
    SET status = 'RUNNING'
    RETURNING cid
    """
    And I add "mongodb_cluster_stop" task
    And that task acquired by worker
    And that task finished by worker with result = <worker_result>
    And I execute query
    """
    SELECT status
      FROM code.get_clusters(
        i_folder_id        => :folder_id,
        i_limit             => 42
    )
    """
    Then it returns one row matches
    """
    status: <cluster_status>
    """

  Examples:
    | worker_result | cluster_status |
    | true          | STOPPED        |
    | false         | STOP-ERROR     |

  Scenario Outline: Cluster status start transitions
    Given task_id
    When I execute query
    """
    UPDATE dbaas.clusters
    SET status = 'STOPPED'
    RETURNING cid
    """
    And I add "mongodb_cluster_start" task
    And that task acquired by worker
    And that task finished by worker with result = <worker_result>
    And I execute query
    """
    SELECT status
      FROM code.get_clusters(
        i_folder_id        => :folder_id,
        i_limit             => 42
    )
    """
    Then it returns one row matches
    """
    status: <cluster_status>
    """

  Examples:
    | worker_result | cluster_status |
    | true          | RUNNING        |
    | false         | START-ERROR    |


  @delete
  Scenario: Deleted clusters in listing
    Given task_id
    And "mongodb_cluster_create" task
    And that task acquired by worker
    And that task finished by worker with result = true
    When I execute query
    """
    SELECT status
      FROM code.get_clusters(
        i_folder_id => :folder_id,
        i_limit     => 42
    )
    """
    Then it returns one row matches
    """
    status: RUNNING
    """
    Given vars "delete_task_id, delete_metadata_task_id, purge_task_id"
    When I add "mongodb_cluster_delete" task with "delete_task_id" task_id
    When I add "mongodb_cluster_delete_metadata" task with "delete_metadata_task_id" task_id
    And I add "mongodb_cluster_purge" task with "7days" delay_by and "purge_task_id" task_id
    And I execute query
    """
    SELECT status
      FROM code.get_clusters(
        i_folder_id => :folder_id,
        i_limit     => 42
    )
    """
    Then it success
    But it returns nothing
    When I execute query
    """
    SELECT status
      FROM code.get_clusters(
        i_folder_id       => :folder_id,
        i_limit           => 42,
        i_visibility      => 'visible+deleted'
    )
    """
    Then it returns one row matches
    """
    status: DELETING
    """
    When "delete_task_id" acquired by worker
    And "delete_task_id" finished by worker with result = true
    And "delete_metadata_task_id" acquired by worker
    When I execute query
    """
    SELECT status
      FROM code.get_clusters(
        i_folder_id       => :folder_id,
        i_limit           => 42,
        i_visibility      => 'visible+deleted'
    )
    """
    Then it returns one row matches
    """
    status: METADATA-DELETING
    """
    When "delete_metadata_task_id" finished by worker with result = true
    And I execute query
    """
    SELECT status
      FROM code.get_clusters(
        i_folder_id       => :folder_id,
        i_limit           => 42,
        i_visibility      => 'visible+deleted'
    )
    """
    Then it returns one row matches
    """
    status: METADATA-DELETED
    """
    When "purge_task_id" acquired by worker
    And I execute query
    """
    SELECT status
      FROM code.get_clusters(
        i_folder_id       => :folder_id,
        i_limit           => 42,
        i_visibility      => 'visible+deleted'
    )
    """
    Then it success
    But it returns nothing
    When I execute query
    """
    SELECT status
      FROM code.get_clusters(
        i_folder_id       => :folder_id,
        i_limit           => 42,
        i_visibility      => 'all'
    )
    """
    Then it returns one row matches
    """
    status: PURGING
    """

  @update-folder
  Scenario: Update cluster folder
    Given other folder
    When I execute query
    """
    SELECT cid FROM code.get_clusters(:folder_id, NULL)
    """
    Then it returns one row matches
    """
    cid: {{ cid }}
    """
    When I execute cluster change
    """
    SELECT cid
      FROM code.update_cluster_folder(
        i_cid       => :cid,
        i_folder_id => :other_folder_id,
        i_rev       => :rev
    )
    """
    Then it returns one row matches
    """
    cid: {{ cid }}
    """
    When I execute query
    """
    SELECT cid FROM code.get_clusters(:folder_id, NULL)
    """
    Then it returns nothing
    When I execute query
    """
    SELECT cid FROM code.get_clusters(:other_folder_id, NULL)
    """
    Then it returns one row matches
    """
    cid: {{ cid }}
    """

  @update-folder
  Scenario: Update cluster folder for deleted cluster
    Given other folder
    And vars "create_task_id, delete_task_id"
    When I add "mongodb_cluster_create" task with "create_task_id" task_id
    And "create_task_id" acquired by worker
    And "create_task_id" finished by worker with result = true
    And I add "mongodb_cluster_delete" task with "delete_task_id" task_id
    And "delete_task_id" acquired by worker
    And "delete_task_id" finished by worker with result = true
    When I execute cluster change
    """
    SELECT cid
      FROM code.update_cluster_folder(
        i_cid       => :cid,
        i_folder_id => :other_folder_id,
        i_rev       => :rev
    )
    """
    Then it fail with error matches "Cluster [\w-]+ not exists, invisible or already in folder"

  @update-folder
  Scenario: Update cluster folder for same folder
    When I execute cluster change
    """
    SELECT cid
      FROM code.update_cluster_folder(
        i_cid       => :cid,
        i_folder_id => :folder_id,
        i_rev       => :rev
    )
    """
    Then it fail with error matches "Cluster [\w-]+ not exists, invisible or already in folder"

  Scenario: Update cluster name
    When I execute cluster change
    """
    SELECT name
      FROM code.update_cluster_name(
        i_cid  => :cid,
        i_rev  => :rev,
        i_name => 'New name? No name!'
      )
    """
    Then it returns one row matches
    """
    name: 'New name? No name!'
    """

  Scenario: Update cluster description
    When I execute cluster change
    """
    SELECT description
      FROM code.update_cluster_description(
        i_cid         => :cid,
        i_rev         => :rev,
        i_description => 'MongoDB is a cross-platform document-oriented database program'
      )
    """
    Then it returns one row matches
    """
    description: 'MongoDB is a cross-platform document-oriented database program'
    """

  Scenario: Update cluster deletion_protection flag
    When I execute cluster change
    """
    SELECT deletion_protection
      FROM code.update_cluster_deletion_protection(
        i_cid                 => :cid,
        i_rev                 => :rev,
        i_deletion_protection => true
      )
    """
    Then it returns one row matches
    """
    deletion_protection: true
    """
