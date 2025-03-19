@greenplum
@grpc_api
@search
Feature: Validate Greenplum search docs
  Background:
    Given default headers
    When we add default feature flag "MDB_GREENPLUM_CLUSTER"
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "environment": "PRESTABLE",
        "name": "testName",
        "description": "test cluster description",
        "labels": {
            "foo": "bar"
        },
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
        "segment_host_count": 4,
        "user_name": "usr1",
        "user_password": "Pa$$w0rd"
    }
    """
    Then we get gRPC response with body
    """
    {
        "id": "worker_task_id1",
        "done": false
    }
    """
    When "worker_task_id1" acquired and finished by worker

  Scenario: Cluster creation and deletion
    Then last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "service": "managed-greenplum",
        "name": "testName",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {"foo": "bar"},
            "users": ["usr1"],
            "hosts": [
              "myt-1.df.cloud.yandex.net",
              "myt-2.df.cloud.yandex.net",
              "myt-3.df.cloud.yandex.net",
              "myt-4.df.cloud.yandex.net",
              "myt-5.df.cloud.yandex.net",
              "myt-6.df.cloud.yandex.net"
            ]
        }
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "worker_task_id2"
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-greenplum",
        "deleted": "<TIMESTAMP>",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {}
    }
    """

  @reindexer
  Scenario: Reindexer works with Greenplum clusters
    When we run mdb-search-reindexer
    Then last document in search_queue matches
    """
    {
        "service": "managed-greenplum",
        "timestamp": "<TIMESTAMP>",
        "reindex_timestamp": "<TIMESTAMP>"
    }
    """
