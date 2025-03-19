Feature: System metrics

  Background:
    Given default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    And "create_grpc" data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
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
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "iva"
        }],
        "description": "test cluster",
        "network_id": "IN-PORTO-NO-NETWORK-API"
    }
    """
    And health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "Alive"
           }
       ],
       "hosts": [
           {
               "fqdn": "man-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "vla-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ],
               "system": {
                   "mem": {
                       "timestamp": 292837999,
                       "used": 2938172,
                       "total": 293829382
                   },
                   "disk": {
                       "timestamp": 292837198,
                       "used": 192838293,
                       "total": 9829384144
                   }
               }
           },
           {
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "clickhouse",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "clickhouse",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ],
               "system": {
                   "cpu": {
                       "timestamp": 292837382,
                       "used": 0.74
                   },
                   "mem": {
                       "timestamp": 292837382,
                       "used": 1929282,
                       "total": 10000000
                   },
                   "disk": {
                       "timestamp": 292837382,
                       "used": 292938384,
                       "total": 3004039493
                   }
               }
           }
       ]
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create ClickHouse cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with "create_grpc" data
    Then we get gRPC response OK
    When "worker_task_id1" acquired and finished by worker

  @hosts @list
  Scenario: System metrics works
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shardName": "shard1",
                "subnetId": "",
                "type": "CLICKHOUSE",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shardName": "shard1",
                "subnetId": "",
                "type": "CLICKHOUSE",
                "zoneId": "sas",
                "system": {
                    "cpu": {
                        "timestamp": 292837382,
                        "used": 0.74
                    },
                    "memory": {
                        "timestamp": 292837382,
                        "used": 1929282,
                        "total": 10000000
                    },
                    "disk": {
                        "timestamp": 292837382,
                        "used": 292938384,
                        "total": 3004039493
                    }
                }
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "vla",
                "system": {
                    "memory": {
                        "timestamp": 292837999,
                        "used": 2938172,
                        "total": 293829382
                    },
                    "disk": {
                        "timestamp": 292837198,
                        "used": 192838293,
                        "total": 9829384144
                    }
                }
            }
        ]
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
        {
        "hosts": [
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shard_name": "",
                "subnet_id": "",
                "type": "ZOOKEEPER",
                "zone_id": "iva",
                "system": null
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shard_name": "",
                "subnet_id": "",
                "type": "ZOOKEEPER",
                "zone_id": "man",
                "system": null
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shard_name": "shard1",
                "subnet_id": "",
                "type": "CLICKHOUSE",
                "zone_id": "myt",
                "system": null
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shard_name": "shard1",
                "subnet_id": "",
                "type": "CLICKHOUSE",
                "zone_id": "sas",
                "system": {
                    "cpu": {
                        "timestamp": "292837382",
                        "used": 0.74
                    },
                    "memory": {
                        "timestamp": "292837382",
                        "used": "1929282",
                        "total": "10000000"
                    },
                    "disk": {
                        "timestamp": "292837382",
                        "used": "292938384",
                        "total": "3004039493"
                    }
                }
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shard_name": "",
                "subnet_id": "",
                "type": "ZOOKEEPER",
                "zone_id": "vla",
                "system": {
                    "cpu": null,
                    "memory": {
                        "timestamp": "292837999",
                        "used": "2938172",
                        "total": "293829382"
                    },
                    "disk": {
                        "timestamp": "292837198",
                        "used": "192838293",
                        "total": "9829384144"
                    }
                }
            }
        ],
        "next_page_token": ""
    }
    """
