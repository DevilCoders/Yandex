@dependent-scenarios
Feature: Internal API OpenSearch single instance Cluster lifecycle

  Background: Wait until internal api is ready
    Given Go Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with single OpenSearch cluster

  @setup
  Scenario: Create OpenSearch cluster
    When we create "OpenSearch" cluster "test_cluster" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "15 minutes" [grpc]
    And "OpenSearch" cluster "test_cluster" is up and running [grpc]
    And cluster has no pending changes
    And cluster environment is "PRESTABLE"
    And s3 has bucket for cluster

  Scenario: Install plugins to OpenSearch cluster
    Given "OpenSearch" cluster "test_cluster" is up and running [grpc]
    When we attempt to modify "OpenSearch" cluster "test_cluster" with the following parameters [grpc]
    """
    config_spec:
        opensearch_spec:
            plugins:
                - analysis-icu
                - analysis-kuromoji
                - analysis-nori
                - analysis-phonetic
                - analysis-smartcn
                - analysis-stempel
                - analysis-ukrainian
                - ingest-attachment
                - mapper-annotated-text
                - mapper-murmur3
                - mapper-size
                - repository-azure
                - repository-gcs
                - repository-hdfs
                - repository-s3
                - transport-nio
    update_mask:
        paths:
            - config_spec.opensearch_spec.plugins
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "25 minutes" [grpc]
    And "OpenSearch" cluster "test_cluster" is up and running [grpc]
    And cluster has no pending changes
    And specified plugins installed on all nodes
        | plugin_name               |
        | analysis-icu              |
        | analysis-kuromoji         |
        | analysis-nori             |
        | analysis-phonetic         |
        | analysis-smartcn          |
        | analysis-stempel          |
        | analysis-ukrainian        |
        | ingest-attachment         |
        | mapper-annotated-text     |
        | mapper-murmur3            |
        | mapper-size               |
        | repository-azure          |
        | repository-gcs            |
        | repository-hdfs           |
        | repository-s3             |
        | transport-nio             |

  Scenario: Delete OpenSearch cluster
    When we attempt to delete "OpenSearch" cluster "test_cluster" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And in worker_queue exists "opensearch_cluster_delete_metadata" task
    And this task is done
    And we are unable to find "OpenSearch" cluster "test_cluster" [grpc]
    But s3 has bucket for cluster
    And in worker_queue exists "opensearch_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster
