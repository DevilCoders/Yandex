@events
Feature: Sharded Porto MongoDB 4.0 Cluster

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_MONGODB_ALLOW_DEPRECATED_VERSIONS"]
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
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Master",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Replica",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Replica",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "man-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongocfg",
                       "role": "Master",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "sas-2.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongocfg",
                       "role": "Replica",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "vla-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongocfg",
                       "role": "Replica",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "man-2.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongos",
                       "role": "Master",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "sas-3.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongos",
                       "role": "Master",
                       "status": "Alive"
                   }
               ]
           }
       ]
    }
    """
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_0": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
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
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Enable sharding for MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.EnableClusterShardingMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.EnableClusterSharding" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "zoneId": "sas",
                "type": "MONGOD"
            }, {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add shard to MongoDB cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.AddClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mongodb.AddClusterShard" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "shard_name": "shard2"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/operations/worker_task_id3"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.mongodb.v1.Shard",
        "clusterId": "cid1",
        "name": "shard2"
    }
    """

  Scenario: Sharded cluster creation works
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 19.0,
        "memoryUsed": 81604378624,
        "ssdSpaceUsed": 193273528320
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/shards"
    Then we get response with status 200 and body contains
    """
    {
        "shards": [
            {
                "clusterId": "cid1",
                "name": "rs01"
            },
            {
                "clusterId": "cid1",
                "name": "shard2"
            }
        ]
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/shards/shard2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "shard2"
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/hosts"
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-3.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "sas-4.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "vla"
            },
                        {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "vla"
            }
        ]
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                },
                "retain_period": 7,
                "use_backup_service": true
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "dbaas": {
                "assign_public_ip": false,
                "on_dedicated_host": false,
                "cloud": {
                    "cloud_ext_id": "cloud1"
                },
                "cluster": {
                    "subclusters": {
                        "subcid1": {
                            "hosts": {},
                            "name": "mongod_subcluster",
                            "roles": [
                                "mongodb_cluster.mongod"
                            ],
                            "shards": {
                                "shard_id1": {
                                    "hosts": {
                                        "iva-1.db.yandex.net": {"geo": "iva"},
                                        "myt-1.db.yandex.net": {"geo": "myt"},
                                        "sas-1.db.yandex.net": {"geo": "sas"}
                                    },
                                    "name": "rs01"
                                },
                                "shard_id2": {
                                    "hosts": {
                                        "sas-4.db.yandex.net": {"geo": "sas"},
                                        "vla-2.db.yandex.net": {"geo": "vla"}
                                    },
                                    "name": "shard2"
                                }
                            }
                        },
                        "subcid2": {
                            "hosts": {
                                "man-1.db.yandex.net": {"geo": "man"},
                                "sas-2.db.yandex.net": {"geo": "sas"},
                                "vla-1.db.yandex.net": {"geo": "vla"}
                            },
                            "name": "mongocfg_subcluster",
                            "roles": [
                                "mongodb_cluster.mongocfg"
                            ],
                            "shards": {}
                        },
                        "subcid3": {
                            "hosts": {
                                "man-2.db.yandex.net": {"geo": "man"},
                                "sas-3.db.yandex.net": {"geo": "sas"}
                            },
                            "name": "mongos_subcluster",
                            "roles": [
                                "mongodb_cluster.mongos"
                            ],
                            "shards": {}
                        }
                    }
                },
                "cluster_hosts": [
                    "iva-1.db.yandex.net",
                    "man-1.db.yandex.net",
                    "man-2.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net",
                    "sas-2.db.yandex.net",
                    "sas-3.db.yandex.net",
                    "sas-4.db.yandex.net",
                    "vla-1.db.yandex.net",
                    "vla-2.db.yandex.net"

                ],
                "cluster_id": "cid1",
                "cluster_name": "test",
                "cluster_type": "mongodb_cluster",
                "disk_type_id": "local-ssd",
                "flavor": {
                    "cpu_guarantee": 1.0,
                    "cpu_limit": 1.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.porto.1",
                    "id": "00000000-0000-0000-0000-000000000000",
                    "io_limit": 20971520,
                    "io_cores_limit": 0,
                    "memory_guarantee": 4294967296,
                    "memory_limit": 4294967296,
                    "name": "s1.porto.1",
                    "network_guarantee": 16777216,
                    "network_limit": 16777216,
                    "type": "standard",
                    "generation": 1,
                    "platform_id": "mdb-v1",
                    "vtype": "porto"
                },
                "folder": {
                    "folder_ext_id": "folder1"
                },
                "fqdn": "iva-1.db.yandex.net",
                "geo": "iva",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "shard_id": "shard_id1",
                "shard_name": "rs01",
                "space_limit": 10737418240,
                "subcluster_id": "subcid1",
                "subcluster_name": "mongod_subcluster",
                "vtype": "porto",
                "vtype_id": null
            },
            "default pillar": true,
            "mongodb": {
                "cluster_auth": "keyfile",
                "cluster_name": "test",
                "config": {
                    "mongocfg": {
                        "net": {
                            "maxIncomingConnections": 1024
                        },
                        "operationProfiling": {
                            "mode": "slowOp",
                            "slowOpThresholdMs": 300
                        }
                    },
                    "mongod": {
                        "net": {
                            "maxIncomingConnections": 1024
                        },
                        "operationProfiling": {
                            "mode": "slowOp",
                            "slowOpThresholdMs": 300
                        },
                        "storage": {
                            "journal": {
                                "commitInterval": 100,
                                "enabled": true
                            },
                            "wiredTiger": {
                                "collectionConfig": {
                                    "blockCompressor": "snappy"
                                }
                            }
                        }
                    },
                    "mongos": {
                        "net": {
                            "maxIncomingConnections": 1024
                        }
                    }
                },
                "databases": {
                    "testdb": {}
                },
                "keyfile": "dummy",
                "sharding_enabled": true,
                "shards": {},
                "use_mongod": true,
                "users": {
                    "admin": {
                        "dbs": {
                            "admin": [
                                "root",
                                "dbOwner",
                                "mdbInternalAdmin"
                            ],
                            "local": [
                                "dbOwner"
                            ],
                            "config": [
                                "dbOwner"
                            ]
                        },
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "services": [
                            "mongod",
                            "mongos",
                            "mongocfg"
                        ],
                        "internal": true
                    },
                    "monitor": {
                        "dbs": {
                            "admin": [
                                "clusterMonitor",
                                "mdbInternalMonitor"
                            ],
                            "mdb_internal": [
                                "readWrite"
                            ]
                        },
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "services": [
                            "mongod",
                            "mongos",
                            "mongocfg"
                        ],
                        "internal": true
                    },
                    "test": {
                        "dbs": {
                            "testdb": [
                                "readWrite"
                            ]
                        },
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "services": [
                            "mongod",
                            "mongos"
                        ]
                    }
                },
                "feature_compatibility_version": "4.0",
                "zk_hosts": [
                    "localhost"
                ]
            },
            "mongodb default pillar": true,
            "runlist": [
                "components.mongodb_cluster.mongod"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
            "versions": {
                "mongodb": {
                    "edition": "default",
                    "major_version": "4.0",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """
    When we GET "/api/v1.0/config/vla-2.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                },
                "retain_period": 7,
                "use_backup_service": true
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "dbaas": {
                "assign_public_ip": false,
                "on_dedicated_host": false,
                "cloud": {
                    "cloud_ext_id": "cloud1"
                },
                "cluster": {
                    "subclusters": {
                        "subcid1": {
                            "hosts": {},
                            "name": "mongod_subcluster",
                            "roles": [
                                "mongodb_cluster.mongod"
                            ],
                            "shards": {
                                "shard_id1": {
                                    "hosts": {
                                        "iva-1.db.yandex.net": {"geo": "iva"},
                                        "myt-1.db.yandex.net": {"geo": "myt"},
                                        "sas-1.db.yandex.net": {"geo": "sas"}
                                    },
                                    "name": "rs01"
                                },
                                "shard_id2": {
                                    "hosts": {
                                        "sas-4.db.yandex.net": {"geo": "sas"},
                                        "vla-2.db.yandex.net": {"geo": "vla"}
                                    },
                                    "name": "shard2"
                                }
                            }
                        },
                        "subcid2": {
                            "hosts": {
                                "man-1.db.yandex.net": {"geo": "man"},
                                "sas-2.db.yandex.net": {"geo": "sas"},
                                "vla-1.db.yandex.net": {"geo": "vla"}
                            },
                            "name": "mongocfg_subcluster",
                            "roles": [
                                "mongodb_cluster.mongocfg"
                            ],
                            "shards": {}
                        },
                        "subcid3": {
                            "hosts": {
                                "man-2.db.yandex.net": {"geo": "man"},
                                "sas-3.db.yandex.net": {"geo": "sas"}
                            },
                            "name": "mongos_subcluster",
                            "roles": [
                                "mongodb_cluster.mongos"
                            ],
                            "shards": {}
                        }
                    }
                },
                "cluster_hosts": [
                    "iva-1.db.yandex.net",
                    "man-1.db.yandex.net",
                    "man-2.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net",
                    "sas-2.db.yandex.net",
                    "sas-3.db.yandex.net",
                    "sas-4.db.yandex.net",
                    "vla-1.db.yandex.net",
                    "vla-2.db.yandex.net"

                ],
                "cluster_id": "cid1",
                "cluster_name": "test",
                "cluster_type": "mongodb_cluster",
                "disk_type_id": "local-ssd",
                "flavor": {
                    "cpu_guarantee": 1.0,
                    "cpu_limit": 1.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.porto.1",
                    "id": "00000000-0000-0000-0000-000000000000",
                    "io_limit": 20971520,
                    "io_cores_limit": 0,
                    "memory_guarantee": 4294967296,
                    "memory_limit": 4294967296,
                    "name": "s1.porto.1",
                    "network_guarantee": 16777216,
                    "network_limit": 16777216,
                    "type": "standard",
                    "generation": 1,
                    "platform_id": "mdb-v1",
                    "vtype": "porto"
                },
                "folder": {
                    "folder_ext_id": "folder1"
                },
                "fqdn": "vla-2.db.yandex.net",
                "geo": "vla",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "sas-4.db.yandex.net",
                    "vla-2.db.yandex.net"
                ],
                "shard_id": "shard_id2",
                "shard_name": "shard2",
                "space_limit": 10737418240,
                "subcluster_id": "subcid1",
                "subcluster_name": "mongod_subcluster",
                "vtype": "porto",
                "vtype_id": null
            },
            "default pillar": true,
            "mongodb": {
                "cluster_auth": "keyfile",
                "cluster_name": "test",
                "config": {
                    "mongocfg": {
                        "net": {
                            "maxIncomingConnections": 1024
                        },
                        "operationProfiling": {
                            "mode": "slowOp",
                            "slowOpThresholdMs": 300
                        }
                    },
                    "mongod": {
                        "net": {
                            "maxIncomingConnections": 1024
                        },
                        "operationProfiling": {
                            "mode": "slowOp",
                            "slowOpThresholdMs": 300
                        },
                        "storage": {
                            "journal": {
                                "commitInterval": 100,
                                "enabled": true
                            },
                            "wiredTiger": {
                                "collectionConfig": {
                                    "blockCompressor": "snappy"
                                }
                            }
                        }
                    },
                    "mongos": {
                        "net": {
                            "maxIncomingConnections": 1024
                        }
                    }
                },
                "databases": {
                    "testdb": {}
                },
                "keyfile": "dummy",
                "sharding_enabled": true,
                "shards": {},
                "use_mongod": true,
                "users": {
                    "admin": {
                        "dbs": {
                            "admin": [
                                "root",
                                "dbOwner",
                                "mdbInternalAdmin"
                            ],
                            "local": [
                                "dbOwner"
                            ],
                            "config": [
                                "dbOwner"
                            ]
                        },
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "services": [
                            "mongod",
                            "mongos",
                            "mongocfg"
                        ],
                        "internal": true
                    },
                    "monitor": {
                        "dbs": {
                            "admin": [
                                "clusterMonitor",
                                "mdbInternalMonitor"
                            ],
                            "mdb_internal": [
                                "readWrite"
                            ]
                        },
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "services": [
                            "mongod",
                            "mongos",
                            "mongocfg"
                        ],
                        "internal": true
                    },
                    "test": {
                        "dbs": {
                            "testdb": [
                                "readWrite"
                            ]
                        },
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "services": [
                            "mongod",
                            "mongos"
                        ]
                    }
                },
                "feature_compatibility_version": "4.0",
                "zk_hosts": [
                    "localhost"
                ]
            },
            "mongodb default pillar": true,
            "runlist": [
                "components.mongodb_cluster.mongod"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
            "versions": {
                "mongodb": {
                    "edition": "default",
                    "major_version": "4.0",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """

  Scenario: Adding new shard with a name that differs only by case from the existing fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "Shard2",
        "hostSpecs": [
            {
                "type": "MONGOD",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Cannot create shard 'Shard2': shard with name 'shard2' already exists"
    }
    """

  Scenario: Nonexistent shard deletion fails
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1/shards/shard99"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Shard 'shard99' does not exist"
    }
    """

  Scenario: Shard deletion works
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1/shards/shard2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete shard from MongoDB cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.DeleteClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard2"
        }
    }
    """
    And for "worker_task_id4" exists "yandex.cloud.events.mdb.mongodb.DeleteClusterShard" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "shard_name": "shard2"
        }
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cpuUsed": 17,
        "memoryUsed": 73014444032,
        "ssdSpaceUsed": 171798691840
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/shards"
    Then we get response with status 200 and body contains
    """
    {
        "shards": [
            {
                "clusterId": "cid1",
                "name": "rs01"
            }
        ]
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/hosts"
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-3.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "vla"
            }
        ]
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                },
                "retain_period": 7,
                "use_backup_service": true
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "dbaas": {
                "assign_public_ip": false,
                "on_dedicated_host": false,
                "cloud": {
                    "cloud_ext_id": "cloud1"
                },
                "cluster": {
                    "subclusters": {
                        "subcid1": {
                            "hosts": {},
                            "name": "mongod_subcluster",
                            "roles": [
                                "mongodb_cluster.mongod"
                            ],
                            "shards": {
                                "shard_id1": {
                                    "hosts": {
                                        "iva-1.db.yandex.net": {"geo": "iva"},
                                        "myt-1.db.yandex.net": {"geo": "myt"},
                                        "sas-1.db.yandex.net": {"geo": "sas"}
                                    },
                                    "name": "rs01"
                                }
                            }
                        },
                        "subcid2": {
                            "hosts": {
                                "man-1.db.yandex.net": {"geo": "man"},
                                "sas-2.db.yandex.net": {"geo": "sas"},
                                "vla-1.db.yandex.net": {"geo": "vla"}
                            },
                            "name": "mongocfg_subcluster",
                            "roles": [
                                "mongodb_cluster.mongocfg"
                            ],
                            "shards": {}
                        },
                        "subcid3": {
                            "hosts": {
                                "man-2.db.yandex.net": {"geo": "man"},
                                "sas-3.db.yandex.net": {"geo": "sas"}
                            },
                            "name": "mongos_subcluster",
                            "roles": [
                                "mongodb_cluster.mongos"
                            ],
                            "shards": {}
                        }
                    }
                },
                "cluster_hosts": [
                    "iva-1.db.yandex.net",
                    "man-1.db.yandex.net",
                    "man-2.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net",
                    "sas-2.db.yandex.net",
                    "sas-3.db.yandex.net",
                    "vla-1.db.yandex.net"
                ],
                "cluster_id": "cid1",
                "cluster_name": "test",
                "cluster_type": "mongodb_cluster",
                "disk_type_id": "local-ssd",
                "flavor": {
                    "cpu_guarantee": 1.0,
                    "cpu_limit": 1.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.porto.1",
                    "id": "00000000-0000-0000-0000-000000000000",
                    "io_limit": 20971520,
                    "io_cores_limit": 0,
                    "memory_guarantee": 4294967296,
                    "memory_limit": 4294967296,
                    "name": "s1.porto.1",
                    "network_guarantee": 16777216,
                    "network_limit": 16777216,
                    "type": "standard",
                    "generation": 1,
                    "platform_id": "mdb-v1",
                    "vtype": "porto"
                },
                "folder": {
                    "folder_ext_id": "folder1"
                },
                "fqdn": "iva-1.db.yandex.net",
                "geo": "iva",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "shard_id": "shard_id1",
                "shard_name": "rs01",
                "space_limit": 10737418240,
                "subcluster_id": "subcid1",
                "subcluster_name": "mongod_subcluster",
                "vtype": "porto",
                "vtype_id": null
            },
            "default pillar": true,
            "mongodb": {
                "cluster_auth": "keyfile",
                "cluster_name": "test",
                "config": {
                    "mongocfg": {
                        "net": {
                            "maxIncomingConnections": 1024
                        },
                        "operationProfiling": {
                            "mode": "slowOp",
                            "slowOpThresholdMs": 300
                        }
                    },
                    "mongod": {
                        "net": {
                            "maxIncomingConnections": 1024
                        },
                        "operationProfiling": {
                            "mode": "slowOp",
                            "slowOpThresholdMs": 300
                        },
                        "storage": {
                            "journal": {
                                "commitInterval": 100,
                                "enabled": true
                            },
                            "wiredTiger": {
                                "collectionConfig": {
                                    "blockCompressor": "snappy"
                                }
                            }
                        }
                    },
                    "mongos": {
                        "net": {
                            "maxIncomingConnections": 1024
                        }
                    }
                },
                "databases": {
                    "testdb": {}
                },
                "keyfile": "dummy",
                "sharding_enabled": true,
                "shards": {},
                "use_mongod": true,
                "users": {
                    "admin": {
                        "dbs": {
                            "admin": [
                                "root",
                                "dbOwner",
                                "mdbInternalAdmin"
                            ],
                            "local": [
                                "dbOwner"
                            ],
                            "config": [
                                "dbOwner"
                            ]
                        },
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "services": [
                            "mongod",
                            "mongos",
                            "mongocfg"
                        ],
                        "internal": true
                    },
                    "monitor": {
                        "dbs": {
                            "admin": [
                                "clusterMonitor",
                                "mdbInternalMonitor"
                            ],
                            "mdb_internal": [
                                "readWrite"
                            ]
                        },
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "services": [
                            "mongod",
                            "mongos",
                            "mongocfg"
                        ],
                        "internal": true
                    },
                    "test": {
                        "dbs": {
                            "testdb": [
                                "readWrite"
                            ]
                        },
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "services": [
                            "mongod",
                            "mongos"
                        ]
                    }
                },
                "feature_compatibility_version": "4.0",
                "zk_hosts": [
                    "localhost"
                ]
            },
            "mongodb default pillar": true,
            "runlist": [
                "components.mongodb_cluster.mongod"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
            "versions": {
                "mongodb": {
                    "edition": "default",
                    "major_version": "4.0",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """

  Scenario: Last shard deletion fails
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1/shards/shard2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete shard from MongoDB cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.DeleteClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard2"
        }
    }
    """
    When "worker_task_id4" acquired and finished by worker
    And we DELETE "/mdb/mongodb/1.0/clusters/cid1/shards/rs01"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Last shard in cluster cannot be removed"
    }
    """

  @decommission @geo
  Scenario: Create cluster shard in decommissioning geo fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard_bad",
        "hostSpecs": [
            {
                "zoneId": "ugr",
                "type": "MONGOD"
            }, {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "No new resources could be created in zone 'ugr'"
    }
    """

  Scenario: Cluster update resources work
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_0": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.2"
                    }
                },
                "mongocfg": {
                    "resources": {
                        "diskSize": 42949672960
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
        "id": "worker_task_id4",
        "metadata": {
          "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cpuUsed": 24.0,
        "memoryUsed": 103079215104,
        "ssdSpaceUsed": 225485783040
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": true,
        "config": {
            "version": "4.0",
            "featureCompatibilityVersion": "4.0",
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
            "mongodb_4_0": {
                "mongocfg": {
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
                                "wiredTiger": {
                                    "engineConfig": {
                                        "cacheSizeGB": 4.0
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
                                "wiredTiger": {
                                    "engineConfig": {
                                        "cacheSizeGB": 4.0
                                    }
                                }
                            }
                        },
                        "userConfig": {}
                    },
                    "resources": {
                        "diskSize": 42949672960,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.2"
                    }
                },
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
                                        "cacheSizeGB": 4.0
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
                                        "cacheSizeGB": 4.0
                                    }
                                }
                            }
                        },
                        "userConfig": {}
                    },
                    "resources": {
                        "diskSize": 10737418240,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.2"
                    }
                },
                "mongos": {
                    "config": {
                        "defaultConfig": {
                            "net": {
                                "maxIncomingConnections": 1024
                            }
                        },
                        "effectiveConfig": {
                            "net": {
                                "maxIncomingConnections": 1024
                            }
                        },
                        "userConfig": {}
                    },
                    "resources": {
                        "diskSize": 21474836480,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.3"
                    }
                }
            }
        }
    }
    """
    When "worker_task_id4" acquired and finished by worker
    When we GET "/mdb/mongodb/1.0/clusters/cid1/hosts"
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
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 42949672960,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
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
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
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
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "diskSize": 42949672960,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-3.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "sas-4.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 42949672960,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "vla"
            },
                        {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "vla"
            }
        ]
    }
    """
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_0": {
                "mongos": {
                    "config": {
                        "net": {
                            "maxIncomingConnections": 512
                        }
                    }
                },
                "mongocfg": {
                    "resources": {
                        "diskSize": 10737418240
                    }
                },
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskSize": 21474836480
                    },
                    "config": {
                        "operationProfiling": {
                            "mode": "ALL"
                        },
                        "storage": {
                            "journal": {
                                "commitInterval": 200
                            }
                        }
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
        "id": "worker_task_id5",
        "metadata": {
          "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cpuUsed": 19.0,
        "memoryUsed": 81604378624,
        "ssdSpaceUsed": 182536110080
    }
    """

    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": true,
        "config": {
            "version": "4.0",
            "featureCompatibilityVersion": "4.0",
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
            "mongodb_4_0": {
                "mongocfg": {
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
                                "wiredTiger": {
                                    "engineConfig": {
                                        "cacheSizeGB": 4.0
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
                                "wiredTiger": {
                                    "engineConfig": {
                                        "cacheSizeGB": 4.0
                                    }
                                }
                            }
                        },
                        "userConfig": {}
                    },
                    "resources": {
                        "diskSize": 10737418240,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.2"
                    }
                },
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
                                "mode": "ALL",
                                "slowOpThreshold": 300
                            },
                            "storage": {
                                "journal": {
                                    "commitInterval": 200
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
                        "userConfig": {
                            "operationProfiling": {
                                "mode": "ALL"
                            },
                            "storage": {
                                "journal": {
                                    "commitInterval": 200
                                }
                            }
                        }
                    },
                    "resources": {
                        "diskSize": 21474836480,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
                    }
                },
                "mongos": {
                    "config": {
                        "defaultConfig": {
                            "net": {
                                "maxIncomingConnections": 1024
                            }
                        },
                        "effectiveConfig": {
                            "net": {
                                "maxIncomingConnections": 512
                            }
                        },
                        "userConfig": {
                            "net": {
                                "maxIncomingConnections": 512
                            }
                        }
                    },
                    "resources": {
                        "diskSize": 21474836480,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.3"
                    }
                }
            }
        }
    }
    """

    When we GET "/mdb/mongodb/1.0/clusters/cid1/hosts"
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
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
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
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-3.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "sas-4.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "vla"
            },
                        {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-2.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "vla"
            }
        ]
    }
    """

  Scenario: Cluster update up-down resources fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_0": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.2"
                    }
                },
                "mongocfg": {
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
        "code": 3,
        "message": "Upscale of one type of hosts cannot be mixed with downscale of another type of hosts"
    }
    """

  Scenario: Removing hosts with constrait violation fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["man-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Only 2 MONGOCFG + MONGOINFRA hosts provided, but at least 3 needed"
    }
    """
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-3.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Only 1 MONGOS + MONGOINFRA hosts provided, but at least 2 needed"
    }
    """
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-4.db.yandex.net"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete hosts from MongoDB cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.DeleteClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
              "sas-4.db.yandex.net"
            ]
        }
    }
    """
    When "worker_task_id4" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["vla-2.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Mongod shard with resource preset 's1.porto.1' and disk type 'local-ssd' requires at least 1 host"
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 18.0,
        "memoryUsed": 77309411328,
        "ssdSpaceUsed": 182536110080
    }
    """

  Scenario: Adding mongocfg hosts works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "type": "MONGOCFG"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to MongoDB cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-3.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/hosts"
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "man-3.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-3.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "sas-4.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "vla"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "vla"
            }
        ]
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 21.0,
        "memoryUsed": 90194313216,
        "ssdSpaceUsed": 225485783040
    }
    """

  Scenario: Adding mongos hosts works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "vla",
            "type": "MONGOS"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to MongoDB cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "vla-3.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/hosts"
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-3.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "sas-4.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "vla"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "vla"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-3.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
                "zoneId": "vla"
            }
        ]
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 23.0,
        "memoryUsed": 98784247808,
        "ssdSpaceUsed": 214748364800
    }
    """

  Scenario: Adding mongod hosts to different shards works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "vla",
            "type": "MONGOD",
            "shardName": "rs01"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to MongoDB cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "vla-3.db.yandex.net"
            ]
        }
    }
    """
    When "worker_task_id4" acquired and finished by worker
    When we GET "/mdb/mongodb/1.0/clusters/cid1/hosts"
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-3.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "sas-4.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "vla"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "vla"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-3.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "vla"
            }
        ]
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 20.0,
        "memoryUsed": 85899345920,
        "ssdSpaceUsed": 204010946560
    }
    """
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "myt",
            "type": "MONGOD",
            "shardName": "shard2"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to MongoDB cluster",
        "done": false,
        "id": "worker_task_id5",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "myt-2.db.yandex.net"
            ]
        }
    }
    """
    When "worker_task_id5" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid1/hosts"
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "myt-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
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
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-3.db.yandex.net",
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.3"
                },
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOS"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOS",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "sas-4.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 32212254720,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOCFG"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "MONGOCFG",
                "zoneId": "vla"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard2",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "vla"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-3.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "vla"
            }
        ]
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 21.0,
        "memoryUsed": 90194313216,
        "ssdSpaceUsed": 214748364800
    }
    """

  Scenario: Adding 8th mongod host fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "myt",
            "type": "MONGOD",
            "shardName": "rs01"
        }]
    }
    """
    And "worker_task_id4" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "vla",
            "type": "MONGOD",
            "shardName": "rs01"
        }]
    }
    """
    And "worker_task_id5" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "iva",
            "type": "MONGOD",
            "shardName": "rs01"
        }]
    }
    """
    And "worker_task_id6" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "type": "MONGOD",
            "shardName": "rs01"
        }]
    }
    """
    And "worker_task_id7" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "iva",
            "type": "MONGOD",
            "shardName": "rs01"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Mongod shard with resource preset 's1.porto.1' and disk type 'local-ssd' allows at most 7 hosts"
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 23.0,
        "memoryUsed": 98784247808,
        "ssdSpaceUsed": 236223201280
    }
    """

  Scenario Outline: Cluster health deduction works
    Given health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "<status>"
           }
       ],
       "hosts": [
           {
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "<md-iva>",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Master",
                       "status": "<md-iva>"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "<md-myt>",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Replica",
                       "status": "<md-myt>"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "<md-sas>",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Replica",
                       "status": "<md-sas>"
                   }
               ]
           },
           {
               "fqdn": "man-1.db.yandex.net",
               "cid": "cid1",
               "status": "<mc-man>",
               "services": [
                   {
                       "name": "mongocfg",
                       "role": "Master",
                       "status": "<mc-man>"
                   }
               ]
           },
           {
               "fqdn": "sas-2.db.yandex.net",
               "cid": "cid1",
               "status": "<mc-sas>",
               "services": [
                   {
                       "name": "mongocfg",
                       "role": "Replica",
                       "status": "<mc-sas>"
                   }
               ]
           },
           {
               "fqdn": "vla-1.db.yandex.net",
               "cid": "cid1",
               "status": "<mc-vla>",
               "services": [
                   {
                       "name": "mongocfg",
                       "role": "Replica",
                       "status": "<mc-vla>"
                   }
               ]
           },
           {
               "fqdn": "vla-2.db.yandex.net",
               "cid": "cid1",
               "status": "<md-vla>",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Master",
                       "status": "<md-vla>"
                   }
               ]
           },
           {
               "fqdn": "man-2.db.yandex.net",
               "cid": "cid1",
               "status": "<ms-man>",
               "services": [
                   {
                       "name": "mongos",
                       "role": "Master",
                       "status": "<ms-man>"
                   }
               ]
           },
           {
               "fqdn": "sas-3.db.yandex.net",
               "cid": "cid1",
               "status": "<ms-sas>",
               "services": [
                   {
                       "name": "mongos",
                       "role": "Master",
                       "status": "<ms-sas>"
                   }
               ]
           },
           {
               "fqdn": "sas-4.db.yandex.net",
               "cid": "cid1",
               "status": "<md-sas>",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Replica",
                       "status": "<md-sas>"
                   }
               ]
           }
       ]
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "health": "<health>"
    }
    """
    Examples:
      | md-iva | md-myt | md-sas | md-vla | mc-man | mc-vla | mc-sas | ms-man | ms-sas | status   | health   |
      | Alive  | Dead   | Alive  | Alive  | Alive  | Alive  | Alive  | Alive  | Alive  | Degraded | DEGRADED |
      | Alive  | Alive  | Alive  | Alive  | Dead   | Alive  | Alive  | Alive  | Alive  | Degraded | DEGRADED |
      | Alive  | Alive  | Alive  | Alive  | Alive  | Alive  | Alive  | Dead   | Alive  | Degraded | DEGRADED |
      | Dead   | Dead   | Dead   | Dead   | Dead   | Dead   | Dead   | Dead   | Dead   | Dead     | DEAD     |
