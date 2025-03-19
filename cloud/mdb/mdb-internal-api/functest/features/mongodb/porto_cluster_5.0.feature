Feature: Create/Modify Porto MongoDB Cluster 5.0

  Background:
    Given default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
                }
            },
            "access": {
                "webSql": true
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
           }
       ]
    }
    """
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
    When "worker_task_id1" acquired and finished by worker

  @events
  Scenario: Cluster creation works
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": false,
        "config": {
            "version": "5.0",
            "featureCompatibilityVersion": "5.0",
            "access": {
                "webSql": true,
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
            "mongodb_5_0": {
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
        "health": "ALIVE",
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
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "access": {
                "web_sql": true
            },
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
                "feature_compatibility_version": "5.0",
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
                    "major_version": "5.0",
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
      | md-iva | md-myt | md-sas | status   | health   |
      | Alive  | Dead   | Alive  | Degraded | DEGRADED |
      | Dead   | Dead   | Dead   | Dead     | DEAD     |

  Scenario: Config for unmanaged cluster doesn't work
    When we GET "/api/v1.0/config_unmanaged/iva-1.db.yandex.net"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Unknown cluster for host iva-1.db.yandex.net"
    }
    """

  Scenario: Setting FCV to same value fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "featureCompatibilityVersion": "5.0"
        }
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """

  Scenario: Setting FCV to "4.4" value works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "featureCompatibilityVersion": "4.4"
        }
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": false,
        "config": {
            "version": "5.0",
            "featureCompatibilityVersion": "4.4",
            "access": {
                "webSql": true,
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
            "mongodb_5_0": {
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
        }
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "access": {
                "web_sql": true
            },
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
                "feature_compatibility_version": "4.4",
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
                    "major_version": "5.0",
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

  Scenario: Setting FCV to 4.2 fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "featureCompatibilityVersion": "4.2"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Error parsing configSpec: Invalid feature compatibility version for version 5.0, allowed values: 4.4, 5.0"
    }
    """

  Scenario: Folder operations list works
    When we run query
    """
    DELETE FROM dbaas.worker_queue
    """
    And we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "config": {
                        "net": {
                            "maxIncomingConnections": 256
                        }
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
            "mongodbSpec_5_0": {
                "mongod": {
                    "config": {
                        "net": {
                            "maxIncomingConnections": 128
                        }
                    }
                }
            }
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "config": {
                        "net": {
                            "maxIncomingConnections": 256
                        }
                    }
                }
            }
        }
    }
    """
    And "worker_task_id4" acquired and finished by worker
    And we run query
    """
    UPDATE dbaas.worker_queue
    SET
        start_ts = null,
        end_ts = null,
        result = null,
        create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    And we GET "/mdb/mongodb/1.0/operations?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "operations": [
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify MongoDB cluster",
                "done": false,
                "id": "worker_task_id4",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify MongoDB cluster",
                "done": false,
                "id": "worker_task_id3",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify MongoDB cluster",
                "done": false,
                "id": "worker_task_id2",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            }
        ]
    }
    """

  Scenario: Cluster list works
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_5_0": {
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
        "description": "another test cluster"
    }
    """
    And we run query
    """
    UPDATE dbaas.clusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    And we GET "/mdb/mongodb/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": [
            {
                "sharded": false,
                "config": {
                    "version": "5.0",
                    "featureCompatibilityVersion": "5.0",
                    "access": {
                        "webSql": true,
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
                    "mongodb_5_0": {
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
                "createdAt": "2000-01-01T00:00:00+00:00",
                "deletionProtection": false,
                "description": "test cluster",
                "environment": "PRESTABLE",
                "folderId": "folder1",
                "health": "ALIVE",
                "id": "cid1",
                "labels": {},
                "plannedOperation": null,
                "securityGroupIds": [],
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
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
                "status": "RUNNING"
            },
            {
                "sharded": false,
                "config": {
                    "version": "5.0",
                    "featureCompatibilityVersion": "5.0",
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
                    "mongodb_5_0": {
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
                "createdAt": "2000-01-01T00:00:00+00:00",
                "deletionProtection": false,
                "description": "another test cluster",
                "environment": "PRESTABLE",
                "folderId": "folder1",
                "health": "UNKNOWN",
                "id": "cid2",
                "labels": {},
                "plannedOperation": null,
                "securityGroupIds": [],
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
                "monitoring": [
                    {
                        "description": "YaSM (Golovan) charts",
                        "link": "https://yasm.yandex-team.ru/template/panel/dbaas_mongodb_metrics/cid=cid2",
                        "name": "YASM"
                    },
                    {
                        "description": "Solomon charts",
                        "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid2&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mongodb",
                        "name": "Solomon"
                    },
                    {
                        "description": "Console charts",
                        "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/mongodb_cluster/cid2?section=monitoring",
                        "name": "Console"
                    }
                ],
                "name": "test2",
                "networkId": "",
                "status": "CREATING"
            }
        ]
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters" with params
    """
    {
        "folderId": "folder1",
        "filter": "name = \"test2\""
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "clusters": [
            {
                "sharded": false,
                "config": {
                    "version": "5.0",
                    "featureCompatibilityVersion": "5.0",
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
                    "mongodb_5_0": {
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
                "createdAt": "2000-01-01T00:00:00+00:00",
                "deletionProtection": false,
                "description": "another test cluster",
                "environment": "PRESTABLE",
                "folderId": "folder1",
                "health": "UNKNOWN",
                "id": "cid2",
                "labels": {},
                "plannedOperation": null,
                "securityGroupIds": [],
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
                "monitoring": [
                    {
                        "description": "YaSM (Golovan) charts",
                        "link": "https://yasm.yandex-team.ru/template/panel/dbaas_mongodb_metrics/cid=cid2",
                        "name": "YASM"
                    },
                    {
                        "description": "Solomon charts",
                        "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid2&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mongodb",
                        "name": "Solomon"
                    },
                    {
                        "description": "Console charts",
                        "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/mongodb_cluster/cid2?section=monitoring",
                        "name": "Console"
                    }
                ],
                "name": "test2",
                "networkId": "",
                "status": "CREATING"
            }
        ]
    }
    """

  Scenario Outline: Cluster list with invalid filter fails
    When we GET "/mdb/mongodb/1.0/clusters" with params
    """
    {
        "folderId": "folder1",
        "filter": "<filter>"
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | filter          | status | error code | message                                                                                            |
      | name = 1        | 422    | 3          | Filter 'name = 1' has wrong 'name' attribute type. Expected a string.                              |
      | my = 1          | 422    | 3          | Filter by 'my' ('my = 1') is not supported.                                                        |
      | name =          | 422    | 3          | The request is invalid.\nfilter: Filter syntax error (missing value) at or near 6.\nname =\n     ^ |
      | name < \"test\" | 501    | 12         | Operator '<' not implemented.                                                                      |

  Scenario: Host list works
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
            }
        ]
    }
    """

  @events
  Scenario: Adding host works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-1.db.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.AddClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "man-1.db.yandex.net"
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
                "health": "UNKNOWN",
                "name": "man-1.db.yandex.net",
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
            }
        ]
    }
    """

  Scenario Outline: Adding host with invalid params fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": <hosts>
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | hosts                                  | status | error code | message                                                                                                               |
      | []                                     | 422    | 3          | No hosts to add are specified                                                                                         |
      | [{"zoneId": "man"}, {"zoneId": "man"}] | 501    | 12         | Adding multiple hosts at once is not supported yet                                                                    |
      | [{"zoneId": "nodc"}]                   | 422    | 3          | The request is invalid.\nhostSpecs.0.zoneId: Invalid value, valid value is one of ['iva', 'man', 'myt', 'sas', 'vla'] |

  @events
  Scenario: Deleting host works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete hosts from MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.DeleteClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.DeleteClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "sas-1.db.yandex.net"
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
            }
        ]
    }
    """

  Scenario Outline: Deleting host with invalid params fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": <host list>
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | host list                                      | status | error code | message                                              |
      | []                                             | 422    | 3          | No hosts to delete are specified                     |
      | ["sas-1.db.yandex.net", "myt-1.db.yandex.net"] | 501    | 12         | Deleting multiple hosts at once is not supported yet |
      | ["nohost"]                                     | 404    | 5          | Host 'nohost' does not exist                         |

  Scenario: Deleting host with exclusive constraint violation fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    And we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["iva-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Conflicting operation worker_task_id2 detected"
    }
    """

  Scenario: Deleting last host fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["iva-1.db.yandex.net"]
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["myt-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Mongod shard with resource preset 's1.porto.1' and disk type 'local-ssd' requires at least 1 host"
    }
    """

  Scenario: User list works
    When we GET "/mdb/mongodb/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["readWrite"]
                    }
                ]
            }
        ]
    }
    """

  Scenario: User get works
    When we GET "/mdb/mongodb/1.0/clusters/cid1/users/test"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "permissions": [
            {
                "databaseName": "testdb",
                "roles": ["readWrite"]
            }
        ]
    }
    """

  Scenario: Nonexistent user get fails
    When we GET "/mdb/mongodb/1.0/clusters/cid1/users/test2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "User 'test2' does not exist"
    }
    """

  Scenario Outline: Adding user <name> with invalid params fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "<name>",
            "password": "<password>",
            "permissions": [{
                "databaseName": "<database>",
                "roles": ["<role>"]
            }]
        }
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<error message>"
    }
    """
    Examples:
      | name       | password | database | role   | status | error code | error message                                                                                                                    |
      | test       | password | testdb   | read   | 409    | 6          | User 'test' already exists                                                                                                       |
      | test2      | password | nodb     | read   | 404    | 5          | Database 'nodb' does not exist                                                                                                   |
      | test3      | password | testdb   | norole | 422    | 3          | The request is invalid.\nuserSpec.permissions.0.roles.0: Invalid value 'norole', allowed values: mdbMonitor, mdbShardingManager, mdbReplication, mdbGlobalWriter, read, readWrite, mdbDbAdmin |
      | b@dn@me!   | password | testdb   | read   | 422    | 3          | The request is invalid.\nuserSpec.name: User name 'b@dn@me!' does not conform to naming rules                                    |
      | user       | short    | testdb   | read   | 422    | 3          | The request is invalid.\nuserSpec.password: Password must be between 8 and 128 characters long                                   |
      | admin      | password | testdb   | read   | 422    | 3          | The request is invalid.\nuserSpec.name: User name 'admin' is not allowed                                                         |
      | local_user | password | admin    | read   | 422    | 3          | Invalid permission combination: cannot assign 'read' to DB 'admin'                                                               |
      | local_user | password | local    | read   | 422    | 3          | Invalid permission combination: cannot assign 'read' to DB 'local'                                                               |
      | local_user | password | config   | read   | 422    | 3          | Invalid permission combination: cannot assign 'read' to DB 'config'                                                              |

  @events
  Scenario: Adding user with empty database acl works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.CreateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": []
    }
    """

  Scenario: Adding user with default database acl works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb",
            "roles": ["readWrite"]
        }]
    }
    """

  Scenario: Adding user with privileges on admin DB works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test3",
            "password": "password",
            "permissions": [
              {
                "databaseName": "admin",
                "roles": ["mdbMonitor"]
              }
            ]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test3"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/users/test3"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test3",
        "permissions": [{
            "databaseName": "admin",
            "roles": ["mdbMonitor"]
        }]
    }
    """

  @events
  Scenario: Modifying user with permission for database works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/mongodb/1.0/clusters/cid1/users/test2" with data
    """
    {
        "permissions": [{
            "databaseName": "testdb",
            "roles": ["read"]
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in MongoDB cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mongodb.UpdateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb",
            "roles": ["read"]
        }]
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we PATCH "/mdb/mongodb/1.0/clusters/cid1/users/test2" with data
    """
    {
        "permissions": []
    }
    """
    And we GET "/mdb/mongodb/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": []
    }
    """

  @events
  Scenario Outline: Granting permission on database works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "<name>",
            "password": "password",
            "permissions": []
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/users/<name>:grantPermission" with data
    """
    {
        "permission": {
            "databaseName": "<database>",
            "roles": ["<role>"]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Grant permission to user in MongoDB cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.GrantUserPermissionMetadata",
            "clusterId": "cid1",
            "userName": "<name>"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mongodb.GrantUserPermission" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "<name>"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/users/<name>"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "<name>",
        "permissions": [{
            "databaseName": "<database>",
            "roles": ["<role>"]
        }]
    }
    """
    Examples:
      | name   | database | role           |
      | test2  | testdb   | read           |
      | test3  | admin    | mdbMonitor |

  Scenario Outline: Granting permission on database with invalid params fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/users/<name>:grantPermission" with data
    """
    {
        "permission": {
            "databaseName": "<database>",
            "roles": ["<role>"]
        }
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | name   | database | role      | status | error code | message                                                                                                              |
      | nouser | testdb   | read      | 404    | 5          | User 'nouser' does not exist                                                                                         |
      | test   | nodb     | read      | 404    | 5          | Database 'nodb' does not exist                                                                                       |
      | test   | testdb   | readWrite | 409    | 6          | User 'test' already has access to the database 'testdb'                                                              |
      | test   | testdb   | norole    | 422    | 3          | The request is invalid.\npermission.roles.0: Invalid value 'norole', allowed values: mdbMonitor, mdbShardingManager, mdbReplication, mdbGlobalWriter, read, readWrite, mdbDbAdmin |

  @events
  Scenario Outline: Revoking permission for database works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "<name>",
            "password": "password",
            "permissions": [{
                "databaseName": "<database>",
                "roles": ["<role>"]
            }]
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/users/<name>:revokePermission" with data
    """
    {
        "databaseName": "<database>"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Revoke permission from user in MongoDB cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.RevokeUserPermissionMetadata",
            "clusterId": "cid1",
            "userName": "<name>"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mongodb.RevokeUserPermission" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "<name>"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/users/<name>"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "<name>",
        "permissions": []
    }
    """
    Examples:
      | name   | database | role           |
      | test2  | testdb   | read           |
      | test3  | admin    | mdbMonitor |

  Scenario Outline: Revoking permission for database with invalid params fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    And we POST "/mdb/mongodb/1.0/clusters/cid1/users/<name>:revokePermission" with data
    """
    {
        "databaseName": "<database>"
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | name   | database | status | error code | message                                                     |
      | nouser | testdb   | 404    | 5          | User 'nouser' does not exist                                |
      | test   | nodb     | 404    | 5          | Database 'nodb' does not exist                              |
      | test2  | testdb   | 409    | 6          | User 'test2' has no access to the database 'testdb'         |

  @events
  Scenario: Deleting user works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we DELETE "/mdb/mongodb/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete user from MongoDB cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.DeleteUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mongodb.DeleteUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["readWrite"]
                    }
                ]
            }
        ]
    }
    """

  Scenario: Deleting nonexistent user fails
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1/users/test2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "User 'test2' does not exist"
    }
    """

  Scenario Outline: Deleting system user <name> fails
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1/users/<name>"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "<message>"
    }
    """
    Examples:
      | name        | message                       |
      | admin       | User 'admin' does not exist   |
      | monitor     | User 'monitor' does not exist |
      | root        | User 'root' does not exist    |

  @events
  Scenario: Changing user password works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1/users/test" with data
    """
    {
        "password": "changed password"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.UpdateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test"
        }
    }
    """

  Scenario Outline: Changing system user <name> password fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1/users/<name>" with data
    """
    {
        "password": "changed password"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "<message>"
    }
    """
    Examples:
      | name        | message                       |
      | admin       | User 'admin' does not exist   |
      | monitor     | User 'monitor' does not exist |
      | root        | User 'root' does not exist    |

  Scenario: Database get works
    When we GET "/mdb/mongodb/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "testdb"
    }
    """

  Scenario: Nonexistent database get fails
    When we GET "/mdb/mongodb/1.0/clusters/cid1/databases/testdb2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Database 'testdb2' does not exist"
    }
    """

  Scenario: Database list works
    When we GET "/mdb/mongodb/1.0/clusters/cid1/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": [{
            "clusterId": "cid1",
            "name": "testdb"
        }]
    }
    """

  Scenario Outline: Adding database <name> with invalid params fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1/databases" with data
    """
    {
       "databaseSpec": {
           "name": "<name>"
       }
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | name    | status | error code | message                                                                                              |
      | testdb  | 409    | 6          | Database 'testdb' already exists                                                                     |
      | b@dN@me | 422    | 3          | The request is invalid.\ndatabaseSpec.name: Database name 'b@dN@me' does not conform to naming rules |

  @events
  Scenario: Adding database works
    When we POST "/mdb/mongodb/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb2"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add database to MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.CreateDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.CreateDatabase" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "database_name": "testdb2"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/databases/testdb2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "testdb2"
    }
    """

  @events
  Scenario: Deleting database works
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete database from MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.DeleteDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.DeleteDatabase" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "database_name": "testdb"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": []
    }
    """

  Scenario: Deleting nonexistent database fails
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1/databases/nodb"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Database 'nodb' does not exist"
    }
    """

  @events
  Scenario: Label set works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "labels": {
            "webscale": "yes"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Update MongoDB cluster metadata",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.UpdateCluster" event with
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
        "labels": {
            "webscale": "yes"
        }
    }
    """

  @events
  Scenario: Description set works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "description": "my cool description"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MongoDB cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.UpdateCluster" event with
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
        "description": "my cool description"
    }
    """

  Scenario: Change disk size to invalid value fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "resources": {
                        "diskSize": 1
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
        "message": "Invalid disk_size, must be between or equal 10737418240 and 2199023255552"
    }
    """

  Scenario: Change disk size to same value fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "resources": {
                        "diskSize": 10737418240
                    }
                }
            }
        }
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """

  @events
  Scenario: Change disk size to valid value works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.UpdateCluster" event with
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
        "ssdSpaceUsed": 64424509440
    }
    """

  @events
  Scenario: Backup window start option change works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "backupWindowStart": {
                "hours": 23,
                "minutes": 10
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.UpdateCluster" event with
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
        "sharded": false,
        "config": {
            "version": "5.0",
            "featureCompatibilityVersion": "5.0",
            "access": {
                "webSql": true,
                "dataLens": false,
                "dataTransfer": false,
                "serverless": false
            },
            "backupWindowStart": {
                "hours": 23,
                "minutes": 10,
                "seconds": 0,
                "nanos": 0
            },
            "backupRetainPeriodDays": 7,
            "performanceDiagnostics": {
                "profilingEnabled": false
            },
            "mongodb_5_0": {
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
        }
    }
    """

  @events
  Scenario: Backup retain period option change works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "backupRetainPeriodDays": 10
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.UpdateCluster" event with
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
        "sharded": false,
        "config": {
            "version": "5.0",
            "featureCompatibilityVersion": "5.0",
            "access": {
                "webSql": true,
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
            "backupRetainPeriodDays": 10,
            "performanceDiagnostics": {
                "profilingEnabled": false
            },
            "mongodb_5_0": {
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
        }
    }
    """

  @events
  Scenario: Update access to Web SQL
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.access" contains
    """
    {
        "webSql": true,
        "dataLens": false,
        "dataTransfer": false,
        "serverless": false
    }
    """
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "webSql": false
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.access" contains
    """
    {
        "webSql": false,
        "dataLens": false,
        "dataTransfer": false,
        "serverless": false
    }
    """

  @events
  Scenario: Allow access to DataTransfer
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "dataTransfer": true
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.access" contains
    """
    {
        "webSql": true,
        "dataLens": false,
        "dataTransfer": true,
        "serverless": false
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.access" contains
    """
    {
        "data_transfer": true
    }
    """

  Scenario: Change wired tiger cache beyond resourcePresetId limit fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "config": {
                        "storage": {
                            "wiredTiger": {
                                "engineConfig": {
                                    "cacheSizeGB": 5.5
                                }
                            }
                        }
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
        "message": "Invalid mongod setting for you instance type: 'storage.wiredTiger.engineConfig.cacheSizeGB' must be between 0.25 and 3.6"
    }
    """

  @events
  Scenario: Change wired tiger cache works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "config": {
                        "storage": {
                            "wiredTiger": {
                                "engineConfig": {
                                    "cacheSizeGB": 1.0
                                }
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
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.UpdateCluster" event with
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
        "sharded": false,
        "config": {
            "version": "5.0",
            "featureCompatibilityVersion": "5.0",
            "access": {
                "webSql": true,
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
            "mongodb_5_0": {
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
                                        "cacheSizeGB": 1.0
                                    }
                                }
                            }
                        },
                        "userConfig": {
                            "storage": {
                                "wiredTiger": {
                                    "engineConfig": {
                                        "cacheSizeGB": 1.0
                                    }
                                }
                            }
                        }
                    },
                    "resources": {
                        "diskSize": 10737418240,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
                    }
                }
            }
        }
    }
    """


  Scenario: Change slowOpThreshold works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "config": {
                        "operationProfiling": {
                            "slowOpThreshold": 500
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
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
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
            "version": "5.0",
            "featureCompatibilityVersion": "5.0",
            "access": {
                "webSql": true,
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
            "mongodb_5_0": {
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
                                "slowOpThreshold": 500
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
                        "userConfig": {
                            "operationProfiling": {
                                "slowOpThreshold": 500
                            }
                        }
                    },
                    "resources": {
                        "diskSize": 10737418240,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
                    }
                }
            }
        }
    }
    """

  @events
  Scenario: Scaling cluster up works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.2"
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.UpdateCluster" event with
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
        "cpuUsed": 6.0,
        "memoryUsed": 25769803776,
        "ssdSpaceUsed": 32212254720
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": false,
        "config": {
            "version": "5.0",
            "featureCompatibilityVersion": "5.0",
            "access": {
                "webSql": true,
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
            "mongodb_5_0": {
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
                }
            }
        }
    }
    """

  Scenario: Scaling cluster down works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.2"
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
            "mongodbSpec_5_0": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1"
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
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": false,
        "config": {
            "version": "5.0",
            "featureCompatibilityVersion": "5.0",
            "access": {
                "webSql": true,
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
            "mongodb_5_0": {
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
        }
    }
    """

  @events
  Scenario: Cluster name change works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "name": "changed"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Update MongoDB cluster metadata",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.UpdateCluster" event with
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
        "name": "changed"
    }
    """

  Scenario: Cluster name change to same value fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "name": "test"
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """

  Scenario: Cluster name change with duplicate value fails
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_5_0": {
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
        "description": "another test cluster"
    }
    """
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "name": "test2"
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Cluster 'test2' already exists"
    }
    """

  @events
  Scenario: Cluster move works
    When we POST "/mdb/mongodb/1.0/clusters/cid1:move" with data
    """
    {
        "destinationFolderId": "folder2"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Move MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.MoveClusterMetadata",
            "clusterId": "cid1",
            "destinationFolderId": "folder2",
            "sourceFolderId": "folder1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.MoveCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/1.0/operations/worker_task_id3"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Move MongoDB cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.MoveClusterMetadata",
            "clusterId": "cid1",
            "destinationFolderId": "folder2",
            "sourceFolderId": "folder1"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder2"
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 0,
        "cpuUsed": 0.0,
        "memoryUsed": 0,
        "ssdSpaceUsed": 0
    }
    """
    When we GET "/mdb/v1/quota/cloud2"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud2",
        "clustersUsed": 1,
        "cpuUsed": 3.0,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 32212254720
    }
    """

  @delete @events
  Scenario: Cluster removal works
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.DeleteClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.DeleteCluster" event with
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
        "clustersUsed": 0,
        "cpuUsed": 0.0,
        "memoryUsed": 0,
        "ssdSpaceUsed": 0
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": []
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 403

  @delete
  Scenario: After cluster delete cluster.name can be reused
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200

  @delete
  Scenario: Cluster with running operations can not be deleted
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET result = NULL,
           end_ts = NULL
    """
    And we DELETE "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Cluster 'cid1' has active tasks"
    }
    """

  @delete
  Scenario: Cluster with failed operations can be deleted
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET result = false
    """
    And we DELETE "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200

  @delete @operations
  Scenario: After cluster delete cluster operations are shown
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "description": "changed"
    }
    """
    Then we get response with status 200
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET start_ts = CASE WHEN start_ts ISNULL THEN null
                      ELSE TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
                      END,
             end_ts = CASE WHEN end_ts ISNULL THEN null
                      ELSE TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
                      END,
          create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/mongodb/1.0/operations?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "operations": [
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Delete MongoDB cluster",
                "done": false,
                "id": "worker_task_id3",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mongodb.v1.DeleteClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify MongoDB cluster",
                "done": true,
                "id": "worker_task_id2",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00",
                "response": {}
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Create MongoDB cluster",
                "done": true,
                "id": "worker_task_id1",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mongodb.v1.CreateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00",
                "response": {}
            }
        ]
    }
    """

  @decommission @geo
  Scenario: Create cluster host in decommissioning geo
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "ugr"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "No new resources could be created in zone 'ugr'"
    }
    """

  @stop
  Scenario: Stop cluster not implemented
    When we POST "/mdb/mongodb/1.0/clusters/cid1:stop"
    Then we get response with status 501 and body contains
    """
    {
        "code": 12,
        "message": "Stop for mongodb_cluster not implemented in that installation"
    }
    """

  @status
  Scenario: Cluster status
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "UPDATING"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """

  Scenario: custom BackupWindowStart works properly on cluster creation
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
                }
            },
            "backupWindowStart": {
                "hours": 3,
                "minutes": 0
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
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.CreateClusterMetadata",
            "clusterId": "cid2"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid2"
    Then we get response with status 200
    And body at path "$.config.backupWindowStart" contains
    """
    {
        "hours": 3,
        "minutes": 0,
        "seconds": 0,
        "nanos": 0
    }
    """

  Scenario: custom backupRetainPeriodDays works properly on cluster creation
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
                }
            },
            "backupRetainPeriodDays": 10
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
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.CreateClusterMetadata",
            "clusterId": "cid2"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid2"
    Then we get response with status 200
    And body at path "$.config" contains
    """
    {
        "backupRetainPeriodDays": 10
    }
    """
