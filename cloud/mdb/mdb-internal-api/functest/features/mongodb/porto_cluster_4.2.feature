Feature: MongoDB 4.2 Cluster

  Background:
    Given default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_2": {
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
        "description": "test cluster",
       "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """


  Scenario: Cluster creation of MongoDB 4.2 works
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with "create" data
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
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": false,
        "config": {
            "version": "4.2",
            "featureCompatibilityVersion": "4.2",
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
            "mongodb_4_2": {
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
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
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
        "networkId": "",
        "status": "CREATING"
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
                        }
                    }
                },
                "cluster_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
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
                "created_at": "**IGNORE**",
                "geo": "iva",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
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
                "feature_compatibility_version": "4.2",
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
                    "major_version": "4.2",
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
    Given health response
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
           }
       ]
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "health": "ALIVE",
        "status": "RUNNING"
    }
    """
