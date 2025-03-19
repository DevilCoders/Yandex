Feature: SQLServer Cluster on dedicated hosts

  Background:
    Given default headers
    And we add default feature flag "MDB_SQLSERVER_CLUSTER"
    And we add default feature flag "MDB_DEDICATED_HOSTS"

  Scenario: Create SQLServer cluster on dedicated hosts
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test dedicated host cluster",
      "networkId": "network1",
      "hostGroupIds": ["hg1", "hg2", "hg3"],
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 17179869184
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      },{
        "zoneId": "sas"
      },{
        "zoneId": "vla"
      }]
    }
    """
    Then we get gRPC response OK
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "host_group_ids": ["hg1", "hg2", "hg3"]
    }
    """
