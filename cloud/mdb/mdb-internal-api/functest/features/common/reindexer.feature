@grpc_api
@search
@reindexer
Feature: gRPC-API clusters reindex

  Background:
    Given default headers
    And we add default feature flag "MDB_KAFKA_CLUSTER"
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "description": "Create Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
        "cluster_id": "cid1"
      }
    }
    """
    When "worker_task_id1" acquired and finished by worker


  Scenario: Reindexer works only with gRPC-API clusters
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "testPostgre",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test_user"
       }],
       "userSpecs": [{
           "name": "test_user",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "sas"
       }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "description": "Create PostgreSQL cluster"
    }
    """
    When we run query
    """
    DELETE FROM dbaas.search_queue
    """
    And we run mdb-search-reindexer
    Then in search_queue there is one document
    And last document in search_queue matches
    """
    {
        "reindex_timestamp": "<TIMESTAMP>"
    }
    """

  Scenario: Reindexer ignore deleted clusters
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "description": "Delete Apache Kafka cluster"
    }
    """
    When we add default feature flag "MDB_GREENPLUM_CLUSTER"
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "environment": "PRESTABLE",
        "name": "test",
        "network_id": "network1",
        "config": {
            "zone_id": "myt",
            "subnet_id": "",
            "assign_public_ip": false
        },
        "master_config": {
            "resources": {
                "resourcePresetId": "s1.compute.2",
                "diskTypeId": "network-ssd",
                "diskSize": 10737418240
            }
        },
        "segment_config": {
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 10737418240
            }
        },
        "master_host_count": 2,
        "segment_in_host": 1,
        "segment_host_count": 2,
        "user_name": "usr1",
        "user_password": "Pa$$w0rd"
    }
    """
    Then we get gRPC response with body
    """
    {
        "description": "Create Greenplum cluster"
    }
    """
    When we run query
    """
    DELETE FROM dbaas.search_queue
    """
    And we run mdb-search-reindexer
    Then in search_queue there is one document
    And last document in search_queue matches
    """
    {
        "reindex_timestamp": "<TIMESTAMP>"
    }
    """
