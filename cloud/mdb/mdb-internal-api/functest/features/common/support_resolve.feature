Feature: Support resolve cluster test

  Background:
    Given default headers
    Given we use following default resources for "clickhouse_cluster" with role "clickhouse_cluster"
    # language=json
    """
    {
        "resource_preset_id": "s1.compute.1",
        "disk_type_id": "network-ssd",
        "disk_size": 10737418240,
        "generation": 1
    }
    """
    Given we use following default resources for "clickhouse_cluster" with role "zk"
    # language=json
    """
    {
        "resource_preset_id": "s1.compute.1",
        "disk_type_id": "network-ssd",
        "disk_size": 10737418240,
        "generation": 1
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    # language=json
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1",
        "serviceAccountId": "sa1",
        "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
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
        }, {
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }],
        "description": "test cluster",
        "network_id": "network1",
        "service_account_id": "sa1",
        "security_group_ids": ["sg_id1", "sg_id2"]
    }
    """
    And "worker_task_id1" acquired and finished by worker

    Scenario: Create and resolve cluster
      When we "Get" via gRPC at "yandex.cloud.priv.mdb.v1.support.ClusterResolveService" with data
      """
      {"cluster_id": "cid1"}
      """
      Then we get gRPC response with body ignoring empty
      """
      {
        "cloud_id": "cloud1",
        "created_at": "**IGNORE**",
        "environment": "qa",
        "folder_id": "folder1",
        "health": "UNKNOWN",
        "hosts": [
          {
            "name": "myt-1.df.cloud.yandex.net",
            "resources": {
              "disk_size": "10737418240",
              "disk_type": "network-ssd",
              "preset": "s1.compute.1"
            },
            "role": "ZOOKEEPER",
            "status": "Unknown"
          },
          {
            "name": "myt-2.df.cloud.yandex.net",
            "resources": {
              "disk_size": "10737418240",
              "disk_type": "network-ssd",
              "preset": "s1.compute.1"
            },
            "role": "ZOOKEEPER",
            "status": "Unknown"
          },
          {
            "name": "myt-3.df.cloud.yandex.net",
            "resources": {
              "disk_size": "10737418240",
              "disk_type": "network-ssd",
              "preset": "s1.compute.1"
            },
            "role": "ZOOKEEPER",
            "status": "Unknown"
          },
          {
            "name": "myt-4.df.cloud.yandex.net",
            "resources": {
              "disk_size": "10737418240",
              "disk_type": "network-ssd",
              "preset": "s1.compute.1"
            },
            "role": "CLICKHOUSE",
            "status": "Unknown"
          },
          {
            "name": "sas-1.df.cloud.yandex.net",
            "resources": {
              "disk_size": "10737418240",
              "disk_type": "network-ssd",
              "preset": "s1.compute.1"
            },
            "role": "CLICKHOUSE",
            "status": "Unknown"
          }
        ],
        "id": "cid1",
        "name": "test",
        "net_id": "network1",
        "status": "RUNNING",
        "type": "clickhouse_cluster",
        "version": "unknown"
      }
      """

    Scenario: Resolve deleted cluster
      When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
      Then we get response with status 200
      When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
      """
      {
          "cluster_id": "cid1"
      }
      """
      Then we get gRPC response OK
      And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.DeleteCluster" event
      And "worker_task_id2" acquired and finished by worker
      And we "Get" via gRPC at "yandex.cloud.priv.mdb.v1.support.ClusterResolveService" with data
      """
      {"cluster_id": "cid1"}
      """
      Then we get gRPC response with body ignoring empty
      """
      {
        "cloud_id": "cloud1",
        "created_at": "**IGNORE**",
        "environment": "qa",
        "folder_id": "folder1",
        "health": "UNKNOWN",
        "id": "cid1",
        "name": "test",
        "net_id": "network1",
        "status": "DELETED",
        "type": "clickhouse_cluster",
        "version": "unknown"
      }
      """
