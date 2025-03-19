Feature: Create/Modify Porto ClickHouse Cluster with enabled Keeper

  Background:
    Given default headers

  @clusters @create @events
  Scenario: Cluster creation works
    Given feature flags
    """
    ["MDB_CLICKHOUSE_KEEPER"]
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "embeddedKeeper": true
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
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 200
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
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "embedded_keeper": true
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
    When "worker_task_id1" acquired and finished by worker
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config" contains
    """
    {
        "embeddedKeeper": true
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then body at path "$.data.clickhouse" contains
    """
    {
        "embedded_keeper": true,
        "keeper_hosts": {
            "man-1.db.yandex.net": 1,
            "myt-1.db.yandex.net": 2,
            "sas-1.db.yandex.net": 3
        },
        "zk_users": {
            "clickhouse": {
                "password": {
                    "data": "dummy",
                    "encryption_version": 0
                }
            }
        }
    }
    """

  Scenario: Attemp to create cluster with ClickHouse Keeper and version lower 22.3 fails
    Given feature flags
    """
    ["MDB_CLICKHOUSE_KEEPER"]
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.7",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "embeddedKeeper": true
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
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Minimum required version for clusters with ClickHouse Keeper is 22.3"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.7",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "embedded_keeper": true
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "minimum required version for clusters with ClickHouse Keeper is 22.3"

  Scenario: Attemp to delete shard with ClickHouse Keeper hosts fails
    Given feature flags
    """
    ["MDB_CLICKHOUSE_KEEPER"]
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "embeddedKeeper": true
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
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 200
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
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "embedded_keeper": true
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
    When "worker_task_id1" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 200
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2",
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards/shard1"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Cluster reconfiguration affecting hosts with ClickHouse Keeper is not currently supported."
    }
    """
    When we "DeleteShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "Cluster reconfiguration affecting hosts with ClickHouse Keeper is not currently supported."

  Scenario: Attempt to delete host with ClickHouse Keeper fails
    Given feature flags
    """
    ["MDB_CLICKHOUSE_KEEPER"]
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "embeddedKeeper": true
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
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 200
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
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "embedded_keeper": true
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
    When "worker_task_id1" acquired and finished by worker
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["myt-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Cluster reconfiguration affecting hosts with ClickHouse Keeper is not currently supported."
    }
    """
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["myt-1.db.yandex.net"]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "Cluster reconfiguration affecting hosts with ClickHouse Keeper is not currently supported."
