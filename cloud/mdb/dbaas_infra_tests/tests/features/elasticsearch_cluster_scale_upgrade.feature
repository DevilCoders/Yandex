@dependent-scenarios
Feature: Internal API ElasticSearch Cluster lifecycle

  Background: Wait until internal api is ready
    Given Go Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard ElasticSearch cluster

  @setup
  Scenario: Create ElasticSearch cluster
    When we create "ElasticSearch" cluster "test_cluster" [grpc]
    """
    config_spec:
        version: "{{ versions[1] }}"
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    And cluster has no pending changes
    And cluster environment is "PRESTABLE"
    And s3 has bucket for cluster

  @upgrade
  Scenario: Upgrade ElasticSearch
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
        version: "{{ versions[0] }}"
    update_mask:
        paths:
            - config_spec.version
    """
    Then response status is "OK" [grpc]
    # hosts get upgraded sequentially: 5 * 10 minutes
    And generated task is finished within "3 hours" [grpc]
    And "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    And all nodes are of version
    """
    version: "{{ versions[0] }}"
    """
    And cluster has no pending changes

  @scale
  Scenario: Scale ElasticSearch datanode up
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
      elasticsearch_spec:
        data_node:
          resources:
            resource_preset_id: db1.small
    update_mask:
      paths:
        - config_spec.elasticsearch_spec.data_node.resources.resource_preset_id
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And heap size of "data" node equals to "8589934592" bytes
    And cluster has no pending changes

  @scale
  Scenario: Scale ElasticSearch datanode down
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
      elasticsearch_spec:
        data_node:
          resources:
            resource_preset_id: db1.micro
    update_mask:
      paths:
        - config_spec.elasticsearch_spec.data_node.resources.resource_preset_id
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And heap size of "data" node equals to "4294967296" bytes
    And cluster has no pending changes

  @scale
  Scenario: Scale ElasticSearch datanode volume size up
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
      elasticsearch_spec:
        data_node:
          resources:
            disk_size: 21474836480
    update_mask:
      paths:
        - config_spec.elasticsearch_spec.data_node.resources.disk_size
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And cluster has no pending changes

  @scale
  Scenario: Scale ElasticSearch datanode volume size down
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
      elasticsearch_spec:
        data_node:
          resources:
            disk_size: 10737418240
    update_mask:
      paths:
        - config_spec.elasticsearch_spec.data_node.resources.disk_size
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And cluster has no pending changes

  @scale
  Scenario: Scale ElasticSearch masternode up
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
      elasticsearch_spec:
        master_node:
          resources:
            resource_preset_id: db1.small
    update_mask:
      paths:
        - config_spec.elasticsearch_spec.master_node.resources.resource_preset_id
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And heap size of "master" node equals to "10309599232" bytes
    And cluster has no pending changes

  @scale
  Scenario: Scale ElasticSearch masternode down
    Given "ElasticSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "ElasticSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
      elasticsearch_spec:
        master_node:
          resources:
            resource_preset_id: db1.micro
    update_mask:
      paths:
        - config_spec.elasticsearch_spec.master_node.resources.resource_preset_id
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And heap size of "master" node equals to "4831838208" bytes
    And cluster has no pending changes
