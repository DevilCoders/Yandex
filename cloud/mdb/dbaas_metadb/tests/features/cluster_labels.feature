Feature: Clusters paging

  Background: All our operations and require cluster
    Given default database
    And cloud with default quota
    And folder

  Scenario: Set labels
    Given "postgres" cluster with
    """
    name: Postgres
    """
    And "mongodb" cluster with
    """
    name: MongoDB
    """
    When I execute query
    """
    SELECT name, labels::jsonb
      FROM code.get_clusters(
        i_folder_id         => :folder_id,
        i_limit             => NULL
    )
    """
    Then it returns "2" rows matches
    """
    - name: Postgres
      labels: {}
    - name: MongoDB
      labels: {}
    """
    When I execute "postgres" cluster change
    """
    SELECT code.set_labels_on_cluster(
        i_folder_id => :folder_id,
        i_cid       => :postgres_cid,
        i_labels    => ARRAY[('env', 'prod'), ('version', '9.6')]::code.label[],
        i_rev       => :rev
    )
    """
    Then it success
    When I execute query
    """
    SELECT labels::jsonb
      FROM code.get_clusters(
        i_folder_id         => :folder_id,
        i_cid               => :postgres_cid,
        i_limit             => NULL
      )
    """
    Then it returns
    """
    - [{'env': 'prod', 'version': '9.6'}]
    """
    When I execute "postgres" cluster change
    """
    SELECT code.set_labels_on_cluster(
        i_folder_id => :folder_id,
        i_cid       => :postgres_cid,
        i_labels    => ARRAY[('env', 'prod'), ('version', '10')]::code.label[],
        i_rev       => :rev
    )
    """
    Then it success
    When I execute query
    """
    SELECT labels::jsonb
      FROM code.get_clusters(
        i_folder_id         => :folder_id,
        i_cid               => :postgres_cid,
        i_limit             => NULL
      )
    """
    Then it returns
    """
    - [{'env': 'prod', 'version': '10'}]
    """
    When I execute "mongodb" cluster change
    """
    SELECT code.set_labels_on_cluster(
        i_folder_id => :folder_id,
        i_cid       => :mongodb_cid,
        i_labels    => ARRAY[('env', 'prod'), ('version', '5.0'), ('tested', 'yes')]::code.label[],
        i_rev       => :rev
    )
    """
    Then it success
    When I execute query
    """
    SELECT name, labels::jsonb
      FROM code.get_clusters(
        i_folder_id         => :folder_id,
        i_limit             => NULL
    )
    """
    Then it returns "2" rows matches
    """
    - name: Postgres
      labels: {'env': 'prod', 'version': '10'}
    - name: MongoDB
      labels: {'env': 'prod', 'version': '5.0', 'tested': 'yes'}
    """

  Scenario: Set labels using json
    Given "mongodb" cluster with
    """
    name: Mongodb
    """
    When I execute "mongodb" cluster change
    """
    SELECT code.set_labels_on_cluster(
        i_folder_id => :folder_id,
        i_cid       => :mongodb_cid,
        i_labels    => jsonb_build_object('env', 'testing')::code.label[],
        i_rev       => :rev
    )
    """
    Then it success
    When I execute query
    """
    SELECT labels::jsonb
      FROM code.get_clusters(
        i_folder_id         => :folder_id,
        i_limit             => NULL
    )
    """
    Then it returns
    """
    - [{'env': 'testing'}]
    """


  Scenario: Set labels with duplicate keys
    Given "redis" cluster with
    """
    name: Redis
    """
    When I execute "redis" cluster change
    """
    SELECT code.set_labels_on_cluster(
        i_folder_id => :folder_id,
        i_cid       => :redis_cid,
        i_labels    => ARRAY[
          ('env', 'prod'), ('env', 'dev')
        ]::code.label[],
        i_rev       => :rev
    )
    """
    Then it fail with error matches "duplicate key value violates unique constraint"


  Scenario Outline: Set labels with NULLs
    Given "redis" cluster with
    """
    name: Redis
    """
    When I execute "redis" cluster change
    """
    SELECT code.set_labels_on_cluster(
        i_folder_id => :folder_id,
        i_cid       => :redis_cid,
        i_labels    => ARRAY[
          (<key>, <value>)
        ]::code.label[],
        i_rev       => :rev
    )
    """
    Then it fail with error matches "violates not-null constraint"

  Examples:
    | key       | value     |
    | 'version' | NULL      |
    | NULL      | 'version' |
    | NULL      | NULL      |


  Scenario Outline: Set labels with key length
    Given "redis" cluster with
    """
    name: Redis
    """
    When I execute "redis" cluster change
    """
    SELECT code.set_labels_on_cluster(
        i_folder_id => :folder_id,
        i_cid       => :redis_cid,
        i_labels    => ARRAY[
          (<key>, 'good-value')
        ]::code.label[],
        i_rev       => :rev
    )
    """
    Then it fail with error matches "violates check constraint .*check_label_key_size"

  Examples:
    | key                                                                                                    |
    | ''                                                                                                     |
    | 'vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv' |


  Scenario: Set labels with invalid value length
    Given "redis" cluster with
    """
    name: Redis
    """
    When I execute "redis" cluster change
    """
    SELECT code.set_labels_on_cluster(
        i_folder_id => :folder_id,
        i_cid       => :redis_cid,
        i_labels    => ARRAY[
          ('good-key',
           'vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv'
          )
        ]::code.label[],
        i_rev       => :rev
    )
    """
    Then it fail with error matches "violates check constraint .*check_label_value_size"


  Scenario: Get labels at rev
    Given "postgres" cluster with
    """
    name: Postgres
    """
    When I execute query
    """
    SELECT code.get_cluster_labels_at_rev(:postgres_cid, 1)::jsonb AS labels
    """
    Then it returns
    """
    - [ { } ]
    """
    When I execute "postgres" cluster change
    """
    SELECT code.set_labels_on_cluster(
        i_folder_id => :folder_id,
        i_cid       => :postgres_cid,
        i_labels    => ARRAY[('env', 'prod')]::code.label[],
        i_rev       => :rev
    )
    """
    Then it success
    When I execute query
    """
    SELECT code.get_cluster_labels_at_rev(:postgres_cid, :rev)::jsonb AS labels
    """
    Then it returns
    """
    - [ {'env': 'prod'} ]
    """
