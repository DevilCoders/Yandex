@elasticsearch
@grpc_api
@search
Feature: Validate ElasticSearch search docs
  Background:
    Given default headers
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "testName",
        "environment": "PRESTABLE",
        "user_specs": [{
            "name": "user1",
            "password": "fencesmakegoodneighbours"
        }],
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "elasticsearch_spec": {
                "data_node": {
                    "elasticsearch_config_7": {
                        "fielddata_cache_size": "100mb",
                        "max_clause_count": 200
                    },
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }, {
            "zone_id": "sas",
            "type": "DATA_NODE"
        }, {
            "zone_id": "man",
            "type": "DATA_NODE"
        }, {
            "zone_id": "myt",
            "type": "MASTER_NODE"
        }, {
            "zone_id": "sas",
            "type": "MASTER_NODE"
        }, {
            "zone_id": "man",
            "type": "MASTER_NODE"
        }],
        "description": "test cluster description",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id1"
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
        "service": "managed-elasticsearch",
        "name": "testName",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": [
              "man-1.db.yandex.net",
              "man-2.db.yandex.net",
              "myt-1.db.yandex.net",
              "myt-2.db.yandex.net",
              "sas-1.db.yandex.net",
              "sas-2.db.yandex.net"
            ],
            "users": ["admin", "user1"]
        }
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
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
        "service": "managed-elasticsearch",
        "deleted": "<TIMESTAMP>",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {}
    }
    """

  Scenario: User creation and deletion
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
          "name": "user2",
          "password": "TestPassword2"
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "done": false,
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
        "service": "managed-elasticsearch",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": [
              "man-1.db.yandex.net",
              "man-2.db.yandex.net",
              "myt-1.db.yandex.net",
              "myt-2.db.yandex.net",
              "sas-1.db.yandex.net",
              "sas-2.db.yandex.net"
            ],
            "users": ["admin", "user1", "user2"]
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "user1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "done": false,
        "id": "worker_task_id3"
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
        "service": "managed-elasticsearch",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": [
              "man-1.db.yandex.net",
              "man-2.db.yandex.net",
              "myt-1.db.yandex.net",
              "myt-2.db.yandex.net",
              "sas-1.db.yandex.net",
              "sas-2.db.yandex.net"
            ],
            "users": ["admin", "user2"]
        }
    }
    """

  @reindexer
  Scenario: Reindexer works with ES clusters
    When we run mdb-search-reindexer
    Then last document in search_queue matches
    """
    {
        "service": "managed-elasticsearch",
        "timestamp": "<TIMESTAMP>",
        "reindex_timestamp": "<TIMESTAMP>"
    }
    """
