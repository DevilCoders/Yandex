@sqlserver
@grpc_api
@security_groups
Feature: Security groups in SQLServer cluster
  Background:
    Given default headers
    When we add default feature flag "MDB_SQLSERVER_CLUSTER"

  Scenario: Create cluster with SG
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1", "sg_id2"],
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {},
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
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
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id1"
    }
    """
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" set to "[sg_id1, sg_id2]"
    When "worker_task_id1" acquired and finished by worker
    And worker set "sg_id1,sg_id2" security groups on "cid1"
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "security_group_ids": ["sg_id1", "sg_id2"]
    }
    """


  Scenario: Create cluster with non existed security group
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "networkId": "network1",
      "securityGroupIds": ["non_existed_sg_id"],
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {},
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
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
      }]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "security group "non_existed_sg_id" not found"

  Scenario: Create cluster with security groups duplicates
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1", "sg_id1", "sg_id1", "sg_id2"],
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {},
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
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
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id1"
    }
    """
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" set to "[sg_id1, sg_id2]"

  Scenario: Modify security groups
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1"],
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {},
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
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
      }]
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
    And worker set "sg_id1" security groups on "cid1"
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "security_group_ids": ["sg_id2"],
      "update_mask": {
        "paths": [
          "security_group_ids"
        ]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "worker_task_id2"
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" set to "[sg_id2]"

  Scenario: Modify security groups to same value
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1", "sg_id2"],
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {},
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
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
      }]
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
    And worker set "sg_id1,sg_id2" security groups on "cid1"
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "security_group_ids": ["sg_id2", "sg_id1"],
      "update_mask": {
        "paths": [
          "security_group_ids"
        ]
      }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"

  Scenario: Restore cluster with security groups
    Given s3 response
    """
    {
      "Contents": [
        {
          "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T010002Z_backup_stop_sentinel.json",
          "LastModified": 5400,
          "Body": {
            "StartLocalTime": "1970-01-01T01:00:02Z",
            "StopLocalTime": "1970-01-01T01:29:02Z"
          }
        },
        {
          "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T000002Z_backup_stop_sentinel.json",
          "LastModified": 3600,
          "Body": {
            "StartLocalTime": "1970-01-01T00:00:02Z",
            "StopLocalTime": "1970-01-01T00:59:02Z"
          }
        }
      ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1"],
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {},
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
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
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "worker_task_id1"
    }
    """
    Given all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
      "network_id": "network1",
      "securityGroupIds": ["sg_id2"],
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {},
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": 10737418240
        }
      },
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "worker_task_id2"
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" set to "[sg_id2]"
