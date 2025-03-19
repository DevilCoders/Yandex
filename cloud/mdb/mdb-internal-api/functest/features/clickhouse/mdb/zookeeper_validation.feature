Feature: Validate ZooKeeper's resources

  Background:
    Given default headers
    When we POST "/mdb/1.0/support/quota/cloud1/resources" with data
    """
    {
        "action": "add",
        "count": 10,
        "presetId": "s2.porto.5"
    }
    """
    Then we get response with status 200
    When we POST "/mdb/1.0/support/quota/cloud1/clusters" with data
    """
    {
        "action": "add",
        "clustersQuota": 2
    }
    """
    Then we get response with status 200
    When we POST "/mdb/1.0/support/quota/cloud1/ssd_space" with data
    """
    {
        "action": "add",
        "ssdSpaceQuota": 1073741824000
    }
    """
    Then we get response with status 200
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.5",
                    "diskTypeId": "local-ssd",
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
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.5",
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
        }],
        "description": "test cluster",
        "network_id": "network1"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
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
    And "worker_task_id2" acquired and finished by worker

  Scenario: Creating ClickHouse cluster with insufficient ZooKeeper flavor fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.5",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
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
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The resource preset of ZooKeeper hosts must have at least 2 CPU cores (s2.porto.2 or higher) for the requested cluster configuration."
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test2",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.5",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "zookeeper": {
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
        }, {
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }],
        "description": "test cluster",
        "network_id": "network1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "The resource preset of ZooKeeper hosts must have at least 2 CPU cores (s2.porto.2 or higher) for the requested cluster configuration."

  Scenario: ZooKeeper flavor is ignored when creating ClickHouse cluster without ZooKeeper
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.5",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
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
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test2",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.5",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "zookeeper": {
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
        }],
        "description": "test cluster",
        "network_id": "network1"
    }
    """
    Then we get gRPC response OK

  Scenario: Adding ZooKeeper with insufficient flavor fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "resources": {
            "resourcePresetId": "s1.porto.1",
            "diskTypeId": "local-ssd",
            "diskSize": 10737418240
        },
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "sas"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The resource preset of ZooKeeper hosts must have at least 2 CPU cores (s1.porto.2 or higher) for the requested cluster configuration."
    }
    """
    When we "AddZookeeper" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "resources": {
            "resource_preset_id": "s1.porto.1",
            "disk_type_id": "local-ssd",
            "disk_size": 10737418240
        },
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "sas"
        }]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "The resource preset of ZooKeeper hosts must have at least 2 CPU cores (s1.porto.2 or higher) for the requested cluster configuration."

  Scenario: Adding ZooKeeper without host specs creates subcluster with right flavor
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper"
    Then we get response with status 200
    When we "AddZookeeper" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response OK
    Given "worker_task_id3" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id3"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Add ZooKeeper to ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.AddClusterZookeeperMetadata",
            "cluster_id": "cid1"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
            "config": "**IGNORE**",
            "created_at": "**IGNORE**",
            "description": "test cluster",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "id": "cid1",
            "name": "test",
            "network_id": "network1",
            "status": "RUNNING",
            "maintenance_window": { "anytime": {} },
            "monitoring": [{
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm/cid=cid1",
                "name": "YASM"
            }, {
                "description": "Solomon charts",
                "link": "https://solomon/cid=cid1&fExtID=folder1",
                "name": "Solomon"
            }, {
                "description": "Console charts",
                "link": "https://console/cid=cid1&fExtID=folder1",
                "name": "Console"
            }]
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.zookeeper" contains
    """
    {
        "resources": {
            "resourcePresetId": "s2.porto.2",
            "diskTypeId": "local-ssd",
            "diskSize": 10737418240
        }
    }
    """

  Scenario: Adding ZooKeeper without resources creates subcluster with right flavor
    When we run query
    """
    DELETE FROM
        dbaas.valid_resources
    WHERE
        role = 'zk'
        AND geo_id = (select geo_id from dbaas.geo where name = 'iva')
        AND flavor in (select id from dbaas.flavors where generation = 2)
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "sas"
        }]
    }
    """
    Then we get response with status 200
    When we "AddZookeeper" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zone_id": "iva"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "sas"
        }]
    }
    """
    Then we get gRPC response OK
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.zookeeper" contains
    """
    {
        "resources": {
            "resourcePresetId": "s1.porto.2",
            "diskTypeId": "local-ssd",
            "diskSize": 10737418240
        }
    }
    """

  Scenario: Adding too many hosts fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "sas"
        }]
    }
    """
    When we "AddZookeeper" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "sas"
        }]
    }
    """
    Then we get gRPC response OK
    And "worker_task_id3" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt",
            "shardName": "shard1"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "ZooKeeper hosts must be upscaled in order to create additional ClickHouse hosts. The resource preset of ZooKeeper hosts must have at least 4 CPU cores (s2.porto.3 or higher) for the requested cluster configuration."
    }
    """
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt",
            "shard_name": "shard1"
        }],
        "copy_schema": true
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "ZooKeeper hosts must be upscaled in order to create additional ClickHouse hosts. The resource preset of ZooKeeper hosts must have at least 4 CPU cores (s2.porto.3 or higher) for the requested cluster configuration."

  Scenario: Adding extra shard fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "sas"
        }]
    }
    """
    When we "AddZookeeper" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "sas"
        }]
    }
    """
    Then we get gRPC response OK
    And "worker_task_id3" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard3",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.5",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "ZooKeeper hosts must be upscaled in order to create an additional ClickHouse shard. The resource preset of ZooKeeper hosts must have at least 4 CPU cores (s2.porto.3 or higher) for the requested cluster configuration."
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard3",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.5",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }
        ]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "ZooKeeper hosts must be upscaled in order to create an additional ClickHouse shard. The resource preset of ZooKeeper hosts must have at least 4 CPU cores (s2.porto.3 or higher) for the requested cluster configuration."

  Scenario: Updating ClickHouse cluster's or shard's flavor fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "sas"
        }]
    }
    """
    When we "AddZookeeper" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "sas"
        }]
    }
    """
    Then we get gRPC response OK
    And "worker_task_id3" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard3",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.2",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "myt"
            }
        ]
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard3",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.2",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            }
        ]
    }
    """
    Then we get gRPC response OK
    And "worker_task_id4" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1/shards/shard3" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.5"
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "ZooKeeper hosts must be upscaled in order to update the shard's resource preset. The resource preset of ZooKeeper hosts must have at least 4 CPU cores (s2.porto.3 or higher) for the requested cluster configuration."
    }
    """
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.5"
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The resource preset of ZooKeeper hosts must have at least 4 CPU cores (s2.porto.3 or higher) for the requested cluster configuration."
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.5"
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.resource_preset_id"
            ]
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "ZooKeeper hosts must be upscaled in order to update the cluster's resource preset. The resource preset of ZooKeeper hosts must have at least 4 CPU cores (s2.porto.3 or higher) for the requested cluster configuration."
