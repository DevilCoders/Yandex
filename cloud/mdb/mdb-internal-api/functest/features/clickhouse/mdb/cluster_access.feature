@grpc_api
Feature: Access fields added to Cluster Config

Background:
  Given default headers
  When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
  """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "cloud_storage": {
                "enabled": true
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }],
        "description": "test cluster",
        "network_id": "network1"
    }
    """
  Then we get gRPC response OK
  When "worker_task_id1" acquired and finished by worker



Scenario: Create/Get of CH cluster with Access params works
When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
"""
    {
        "folder_id": "folder1",
        "name": "test_access_create",
        "environment": "PRESTABLE",
        "config_spec": {
            "access": {
                "web_sql": true,
                "data_lens": true,
                "metrika": true,
                "serverless": true,
                "data_transfer": true,
                "yandex_query": true
            },
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        },
        {
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        },
        {
            "type": "CLICKHOUSE",
            "zone_id": "man"
        }],
        "description": "test cluster",
        "network_id": "IN-PORTO-NO-NETWORK-API"
    }
    """
Then we get gRPC response OK
When "worker_task_id2" acquired and finished by worker

When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2"
    }
    """
Then we get gRPC response OK
And gRPC response body at path "$.config.access" contains
    """
    {
      "web_sql": true,
      "data_lens": true,
      "metrika": true,
      "serverless": true,
      "data_transfer": true,
      "yandex_query": true
    }
    """

Scenario: Modify cluster access works
  When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
  Then we get gRPC response OK
  And gRPC response body at path "$.config.access" contains
    """
    {}
    """
When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
"""
  {
    "cluster_id": "cid1",
    "config_spec": {
          "access": {
              "web_sql": true,
              "data_lens": true,
              "metrika": true,
              "serverless": true,
              "data_transfer": true,
              "yandex_query": true
          }
    },
    "update_mask": {
        "paths": [
            "config_spec.access"
        ]
    }
  }
  """
Then we get gRPC response OK
When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
  """
  {
      "cluster_id": "cid1"
  }
  """
Then we get gRPC response OK
And gRPC response body at path "$.config.access" contains
  """
  {
    "web_sql": true,
    "data_lens": true,
    "metrika": true,
    "serverless": true,
    "data_transfer": true,
    "yandex_query": true
  }
  """
