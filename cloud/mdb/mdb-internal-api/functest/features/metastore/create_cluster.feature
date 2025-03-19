@metastore
@grpc_api
@wip
Feature: Create Compute Metastore Cluster
  Background:
    Given default headers
    And we add default feature flag "MDB_METASTORE_ALPHA"

  Scenario: Cluster creation and delete works
    Given health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "Alive"
           }
       ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.metastore.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "name": "test",
      "description": "test cluster",
      "labels": {
        "foo": "bar"
      },
      "subnet_ids": ["subnet_a_1", "subnet_b_1", "subnet_c_1"],
      "min_servers_per_zone": 1,
      "max_servers_per_zone": 1
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Metastore cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.metastore.v1.CreateClusterMetadata",
        "cluster_id": "cid1"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And we "Get" via gRPC at "yandex.cloud.priv.metastore.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "description": "test cluster",
      "labels": {
        "foo": "bar"
      },
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1",
      "subnet_ids": ["subnet_a_1", "subnet_b_1", "subnet_c_1"],
      "min_servers_per_zone": "1",
      "max_servers_per_zone": "1",
      "status": "RUNNING",
      "health": "ALIVE"
    }
    """

    And we run query
    """
    UPDATE dbaas.worker_queue
    SET create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00',
        start_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00',
        end_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    WHERE task_id = 'worker_task_id1'
    """
    When we "ListOperations" via gRPC at "yandex.cloud.priv.metastore.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "operations": [
        {
          "id": "worker_task_id1",
          "description": "Create Metastore cluster",
          "created_by": "user",
          "created_at": "2000-01-01T00:00:00Z",
          "modified_at": "2000-01-01T00:00:00Z",
          "done": true,
          "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.metastore.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
          },
          "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
          }
        }
      ]
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.metastore.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete Metastore cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.metastore.v1.DeleteClusterMetadata",
        "cluster_id": "cid1"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
