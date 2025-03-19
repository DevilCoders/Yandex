Feature: Create/Modify Compute MongoDB Cluster

  Background:
    Given default headers
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 10737418240
                    }
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
            "zoneId": "myt"
        }, {
            "zoneId": "vla"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "network1",
        "securityGroupIds": ["sg_id2", "sg_id3", "sg_id4"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create MongoDB cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" containing:
      |sg_id2|
      |sg_id3|
      |sg_id4|
    When "worker_task_id1" acquired and finished by worker
    And worker set "sg_id2,sg_id3,sg_id4" security groups on "cid1"
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": ["sg_id2", "sg_id3", "sg_id4"]
    }
    """

  @events
  Scenario: Cluster creation works
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": false,
        "config": {
            "version": "4.4",
            "featureCompatibilityVersion": "4.4",
            "access": {
                "webSql": false,
                "dataLens": false,
                "dataTransfer": false,
                "serverless": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "backupRetainPeriodDays": 7,
            "performanceDiagnostics": {
                "profilingEnabled": false
            },
            "mongodb_4_4": {
                "mongod": {
                    "config": {
                        "defaultConfig": {
                            "net": {
                                "maxIncomingConnections": 1024
                            },
                            "operationProfiling": {
                                "mode": "SLOW_OP",
                                "slowOpThreshold": 300
                            },
                            "storage": {
                                "journal": {
                                    "commitInterval": 100
                                },
                                "wiredTiger": {
                                    "collectionConfig": {
                                        "blockCompressor": "SNAPPY"
                                    },
                                    "engineConfig": {
                                        "cacheSizeGB": 2.0
                                    }
                                }
                            }
                        },
                        "effectiveConfig": {
                            "net": {
                                "maxIncomingConnections": 1024
                            },
                            "operationProfiling": {
                                "mode": "SLOW_OP",
                                "slowOpThreshold": 300
                            },
                            "storage": {
                                "journal": {
                                    "commitInterval": 100
                                },
                                "wiredTiger": {
                                    "collectionConfig": {
                                        "blockCompressor": "SNAPPY"
                                    },
                                    "engineConfig": {
                                        "cacheSizeGB": 2.0
                                    }
                                }
                            }
                        },
                        "userConfig": {}
                    },
                    "resources": {
                        "diskSize": 10737418240,
                        "diskTypeId": "network-ssd",
                        "resourcePresetId": "s1.compute.1"
                    }
                }
            }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "UNKNOWN",
        "id": "cid1",
        "labels": {},
        "monitoring": [
            {
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm.yandex-team.ru/template/panel/dbaas_mongodb_metrics/cid=cid1",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mongodb",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/mongodb_cluster/cid1?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test",
        "networkId": "network1",
        "status": "RUNNING"
    }
    """
    And for "worker_task_id1" exists "yandex.cloud.events.mdb.mongodb.CreateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 3.0,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 32212254720
    }
    """

  Scenario: Billing cost estimation works
    When we POST "/mdb/mongodb/1.0/console/clusters:estimate?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "local-nvme",
                        "diskSize": 107374182400
                    }
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
            "zoneId": "myt"
        }, {
            "zoneId": "vla"
        }, {
            "assignPublicIp": true,
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "mongodb_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "local-nvme",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "mongodb_cluster.mongod"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "mongodb_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "local-nvme",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "mongodb_cluster.mongod"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "mongodb_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "local-nvme",
                    "online": 1,
                    "public_ip": 1,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "mongodb_cluster.mongod"
                    ],
                    "software_accelerated_network_cores": 0
                }
            }
        ]
    }
    """

  Scenario: Console create host estimate costs
    When we POST "/mdb/mongodb/1.0/console/hosts:estimate" with data
    """
    {
        "folderId": "folder1",
        "billingHostSpecs": [
        {
          "host": {
            "type": "MONGOD",
            "zoneId": "vla",
            "subnetId": "subnet-id",
            "assignPublicIp": true
          },
          "resources": {
            "resourcePresetId": "s1.compute.1",
            "diskSize": 1,
            "diskTypeId": "disk-type-id"
          }
        },
        {
          "host": {
            "type": "MONGOD",
            "zoneId": "iva",
            "subnetId": "subnet-id",
            "assignPublicIp": true
          },
          "resources": {
            "resourcePresetId": "s1.compute.1",
            "diskSize": 1,
            "diskTypeId": "disk-type-id"
          }
        },
        {
          "host": {
            "type": "MONGOS",
            "zoneId": "iva",
            "subnetId": "subnet-id",
            "assignPublicIp": true
          },
          "resources": {
            "resourcePresetId": "s1.compute.1",
            "diskSize": 1,
            "diskTypeId": "disk-type-id"
          }
        },
        {
          "host": {
            "type": "MONGOS",
            "zoneId": "iva",
            "subnetId": "subnet-id",
            "assignPublicIp": true
          },
          "resources": {
            "resourcePresetId": "s1.compute.1",
            "diskSize": 1,
            "diskTypeId": "disk-type-id"
          }
        },
        {
          "host": {
            "type": "MONGOCFG",
            "zoneId": "iva",
            "subnetId": "subnet-id",
            "assignPublicIp": true
          },
          "resources": {
            "resourcePresetId": "s1.compute.1",
            "diskSize": 1,
            "diskTypeId": "disk-type-id"
          }
        },
        {
          "host": {
            "type": "MONGOCFG",
            "zoneId": "iva",
            "subnetId": "subnet-id",
            "assignPublicIp": true
          },
          "resources": {
            "resourcePresetId": "s1.compute.1",
            "diskSize": 1,
            "diskTypeId": "disk-type-id"
          }
        }
      ]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "metrics": [
        {
          "folder_id": "folder1",
          "schema": "mdb.db.generic.v1",
          "tags": {
            "cluster_type": "mongodb_cluster",
            "disk_size": 1,
            "disk_type_id": "disk-type-id",
            "online": 1,
            "public_ip": 1,
            "resource_preset_id": "s1.compute.1",
            "platform_id": "mdb-v1",
            "cores": 1,
            "core_fraction": 100,
            "memory": 4294967296,
            "on_dedicated_host": 0,
            "roles": [
              "mongodb_cluster.mongod"
            ],
            "software_accelerated_network_cores": 0
          }
        },
        {
          "folder_id": "folder1",
          "schema": "mdb.db.generic.v1",
          "tags": {
            "cluster_type": "mongodb_cluster",
            "disk_size": 1,
            "disk_type_id": "disk-type-id",
            "online": 1,
            "public_ip": 1,
            "resource_preset_id": "s1.compute.1",
            "platform_id": "mdb-v1",
            "cores": 1,
            "core_fraction": 100,
            "memory": 4294967296,
            "on_dedicated_host": 0,
            "roles": [
              "mongodb_cluster.mongod"
            ],
            "software_accelerated_network_cores": 0
          }
        },
        {
          "folder_id": "folder1",
          "schema": "mdb.db.generic.v1",
          "tags": {
            "cluster_type": "mongodb_cluster",
            "disk_size": 1,
            "disk_type_id": "disk-type-id",
            "online": 1,
            "public_ip": 1,
            "resource_preset_id": "s1.compute.1",
            "platform_id": "mdb-v1",
            "cores": 1,
            "core_fraction": 100,
            "memory": 4294967296,
            "on_dedicated_host": 0,
            "roles": [
              "mongodb_cluster.mongos"
            ],
            "software_accelerated_network_cores": 0
          }
        },
        {
          "folder_id": "folder1",
          "schema": "mdb.db.generic.v1",
          "tags": {
            "cluster_type": "mongodb_cluster",
            "disk_size": 1,
            "disk_type_id": "disk-type-id",
            "online": 1,
            "public_ip": 1,
            "resource_preset_id": "s1.compute.1",
            "platform_id": "mdb-v1",
            "cores": 1,
            "core_fraction": 100,
            "memory": 4294967296,
            "on_dedicated_host": 0,
            "roles": [
              "mongodb_cluster.mongos"
            ],
            "software_accelerated_network_cores": 0
          }
        },
        {
          "folder_id": "folder1",
          "schema": "mdb.db.generic.v1",
          "tags": {
            "cluster_type": "mongodb_cluster",
            "disk_size": 1,
            "disk_type_id": "disk-type-id",
            "online": 1,
            "public_ip": 1,
            "resource_preset_id": "s1.compute.1",
            "platform_id": "mdb-v1",
            "cores": 1,
            "core_fraction": 100,
            "memory": 4294967296,
            "on_dedicated_host": 0,
            "roles": [
              "mongodb_cluster.mongocfg"
            ],
            "software_accelerated_network_cores": 0
          }
        },
        {
          "folder_id": "folder1",
          "schema": "mdb.db.generic.v1",
          "tags": {
            "cluster_type": "mongodb_cluster",
            "disk_size": 1,
            "disk_type_id": "disk-type-id",
            "online": 1,
            "public_ip": 1,
            "resource_preset_id": "s1.compute.1",
            "platform_id": "mdb-v1",
            "cores": 1,
            "core_fraction": 100,
            "memory": 4294967296,
            "on_dedicated_host": 0,
            "roles": [
              "mongodb_cluster.mongocfg"
            ],
            "software_accelerated_network_cores": 0
          }
        }
      ]
    }
    """

  Scenario: Modify to burstable on multi-host cluster fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "b2.compute.3"
                    }
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Mongod shard with resource preset 'b2.compute.3' and disk type 'network-ssd' allows at most 1 host"
    }
    """

  Scenario: Resources modify on network-ssd disk works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.compute.2"
                    }
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """

  Scenario: Disk scale up on network-ssd disk works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "diskSize": 21474836480
                    }
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """

  Scenario: Disk scale down on network-ssd disk without feature flag fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "diskSize": 21474836480
                    }
                }
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "diskSize": 10737418240
                    }
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Requested feature is not available"
    }
    """

  Scenario: Disk scale down on network-ssd disk with feature flag works
    Given feature flags
    """
    ["MDB_NETWORK_DISK_TRUNCATE"]
    """
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "diskSize": 21474836480
                    }
                }
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "diskSize": 10737418240
                    }
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MongoDB cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """

  Scenario: Resources modify on local-nvme disk without feature flag fails
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "local-nvme",
                        "diskSize": 107374182400
                    }
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
            "zoneId": "myt"
        }, {
            "zoneId": "vla"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    And we PATCH "/mdb/mongodb/1.0/clusters/cid2" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.compute.2"
                    }
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Requested feature is not available"
    }
    """

  @stop @events
  Scenario: Stop cluster works
    When we POST "/mdb/mongodb/1.0/clusters/cid1:stop"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Stop MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.StopClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.StopCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/1.0/operations/worker_task_id2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Stop MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.StopClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STOPPING"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STOPPED"
    }
    """

  @stop
  Scenario: Modifying stopped cluster fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @stop
  Scenario: Labels are no longer metadata-only operations
    When we POST "/mdb/mongodb/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "labels": {
            "acid": "yes"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @stop
  Scenario: Stop stopped cluster fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1:stop"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @stop
  Scenario: Delete stopped cluster works
    When we POST "/mdb/mongodb/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we DELETE "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200

  @start @events
  Scenario: Start stopped cluster works
    When we POST "/mdb/mongodb/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1:start"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start MongoDB cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.StartClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mongodb.StartCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STARTING"
    }
    """
    When we GET "/mdb/1.0/operations/worker_task_id3"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start MongoDB cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.StartClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """

  @start
  Scenario: Start running cluster fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:start"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @security_groups
  Scenario: Removing security groups works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": []
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" is empty list
    And "worker_task_id2" acquired and finished by worker
    And worker clear security groups for "cid1"
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": null
    }
    """

  @security_groups
  Scenario: Modify security groups works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" containing:
      |sg_id1|
      |sg_id2|
    And worker clear security groups for "cid1"
    And worker set "sg_id1,sg_id2" security groups on "cid1"
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """

  @security_groups
  Scenario: Modify security groups with same groups show no changes
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": ["sg_id2", "sg_id3", "sg_id4", "sg_id2", "sg_id3", "sg_id4"]
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """

  @security_groups
  Scenario: Modify security groups with more than limit show error
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": ["sg_id1", "sg_id2", "sg_id3", "sg_id4", "sg_id5", "sg_id6", "sg_id7", "sg_id8", "sg_id9", "sg_id10", "sg_id11"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "too many security groups (10 is the maximum)"
    }
    """
