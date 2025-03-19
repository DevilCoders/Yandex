@dependent-scenarios
Feature: Internal API ElasticSearch single instance Cluster lifecycle

  Background: Wait until internal api is ready
    Given Go Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with single ElasticSearch cluster
    And set user defined Elasticsearch version

  @setup
  Scenario: Create ElasticSearch cluster
    When we create "ElasticSearch" cluster "test_cluster" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    And cluster has no pending changes
    And cluster environment is "PRESTABLE"
    And s3 has bucket for cluster

  @users
  Scenario: Add user to Elasticsearch cluster
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to create user in "ElasticSearch" cluster "test_cluster"
    """
    userSpec:
      name: petya
      password: good-password
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And cluster has no pending changes
    Then we are able to log in to "ElasticSearch" cluster "test_cluster" using
    """
    user: petya
    password: good-password
    """

  @users
  Scenario: Update user in Elasticsearch cluster
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to update user in "ElasticSearch" cluster "test_cluster"
    """
    user_name: petya
    update_mask:
      paths:
        - password
    password: new-password
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And cluster has no pending changes
    Then we are unable to log in to "ElasticSearch" cluster "test_cluster" using
    """
    user: petya
    password: good-password
    """
    And we are able to log in to "ElasticSearch" cluster "test_cluster" using
    """
    user: petya
    password: new-password
    """

  @users
  Scenario: Update admin user password in Elasticsearch cluster
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
      admin_password: new-admin-password
    update_mask:
      paths:
        - config_spec.admin_password
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And cluster has no pending changes
    Then we are able to log in to "ElasticSearch" cluster "test_cluster" using
    """
    user: admin
    password: new-admin-password
    """
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
      admin_password: password
    update_mask:
      paths:
        - config_spec.admin_password
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And cluster has no pending changes
    Then we are able to log in to "ElasticSearch" cluster "test_cluster" using
    """
    user: admin
    password: password
    """

  @users
  Scenario: Delete user from Elasticsearch cluster
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to delete user in "ElasticSearch" cluster "test_cluster"
    """
    user_name: petya
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And cluster has no pending changes
    Then we are unable to log in to "ElasticSearch" cluster "test_cluster" using
    """
    user: petya
    password: new-password
    """

  Scenario: Upgrade cluster edition
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
      edition: platinum
    update_mask:
      paths:
        - config_spec.edition
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And cluster has no pending changes

  Scenario: Downgrade cluster edition
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
      edition: basic
    update_mask:
      paths:
        - config_spec.edition
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And cluster has no pending changes

  @hosts @create
  Scenario: Add ElasticSearch host to a single-node cluster
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to add hosts in "ElasticSearch" cluster "test_cluster"
    """
    host_specs:
      - zone_id: sas
        type: DATA_NODE
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    And  number of "master" nodes equals to "2"
    And  number of "data" nodes equals to "2"
    And cluster has no pending changes

  Scenario: Delete ElasticSearch cluster
    When we attempt to delete "ElasticSearch" cluster "test_cluster" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And in worker_queue exists "elasticsearch_cluster_delete_metadata" task
    And this task is done
    And we are unable to find "ElasticSearch" cluster "test_cluster" [grpc]
    But s3 has bucket for cluster
    And in worker_queue exists "elasticsearch_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster
