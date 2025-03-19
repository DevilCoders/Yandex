@dependent-scenarios
Feature: Internal API OpenSearch cluster lifecycle

  Background: Wait until internal api is ready
    Given Go Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard OpenSearch cluster

  @setup
  Scenario: Create OpenSearch cluster
    When we create "OpenSearch" cluster "test_cluster" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And "OpenSearch" cluster "test_cluster" is up and running [grpc]
    And  number of "master" nodes equals to "1"
    And  number of "data" nodes equals to "2"
    And cluster has no pending changes
    And cluster environment is "PRESTABLE"
    And s3 has bucket for cluster
    When we index the following documents to "test-index"
    """
    [
        {
            "_id": "0",
            "name": "postgresql",
            "type": "regular"
        },
        {
            "_id": "1",
            "name": "mysql",
            "type": "regular"
        },
        {
            "_id": "2",
            "name": "mongodb",
            "type": "spec"
        },
        {
            "_id": "3",
            "name": "clickhouse",
            "type": "spec"
        }
    ]
    """
    Then number of documents in "test-index" equals to "4"
    When we search "test-index" with the following query on all hosts
    """
    {
        "query": {
            "match": {
                "type": "regular"
            }
        }
    }
    """
    Then result contains the following documents on all hosts
    """
    [
        {
            "_id": "0",
            "name": "postgresql",
            "type": "regular"
        },
        {
            "_id": "1",
            "name": "mysql",
            "type": "regular"
        }
    ]
    """

  @hosts @create
  Scenario: Add OpenSearch host to cluster
    Given "OpenSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to add hosts in "OpenSearch" cluster "test_cluster"
    """
    host_specs:
      - zone_id: sas
        type: DATA_NODE
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And "OpenSearch" cluster "test_cluster" is up and running [grpc]
    And  number of "master" nodes equals to "1"
    And  number of "data" nodes equals to "3"
    And cluster has no pending changes

  @hosts @delete
  Scenario: Delete OpenSearch host from cluster
    Given "OpenSearch" cluster "test_cluster" is up and running [grpc]
    When we reset all the juggler crits
    And we attempt to delete "DATA_NODE" host in "sas" from "OpenSearch" cluster "test_cluster"
    Then response status is "OK" [grpc]
    And generated task is finished within "7 minutes" [grpc]
    And "OpenSearch" cluster "test_cluster" is up and running [grpc]
    And  number of "master" nodes equals to "1"
    And  number of "data" nodes equals to "2"
    And cluster has no pending changes

  Scenario: Delete OpenSearch cluster
    When we attempt to delete "OpenSearch" cluster "test_cluster" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And in worker_queue exists "opensearch_cluster_delete_metadata" task
    And this task is done
    And we are unable to find "OpenSearch" cluster "test_cluster_renamed" [grpc]
    But s3 has bucket for cluster
    And in worker_queue exists "opensearch_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster
