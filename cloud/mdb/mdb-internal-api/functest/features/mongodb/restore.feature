Feature: Restore MongoDB cluster from backup

  Background:
    Given default headers
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
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
        "description": "test cluster"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    When "worker_task_id1" acquired and finished by worker
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T03:00:30.000000+03:00'::timestamptz, DATE('1970-01-01T03:00:30.000000+03:00'), 'SCHEDULE', '{ "before_ts": {"TS": 30, "Inc": 1}, "after_ts": {"TS": 31, "Inc": 6 }, "shard_names": ["shard1"], "permanent": false,"data_size": 30, "name": "stream1", "root_path": "mongodb-backup/cid1/shard1"}'::jsonb),
		('man02', 'DONE', '1970-01-01T00:00:35+00:00'::timestamptz, NULL::DATE, 'USER', '{ "before_ts": {"TS": 35, "Inc": 10 }, "after_ts": { "TS": 36, "Inc": 60 }, "shard_names": ["shard1"],"data_size": 35, "name": "stream2", "root_path": "mongodb-backup/cid1/shard1", "Permanent": true}'::jsonb),
		('auto03', 'DONE', '1970-01-01T00:00:41+00:00'::timestamptz, DATE('1970-03-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"before_ts":{"TS": 40,"Inc": 1},"after_ts": {"TS": 41,"Inc": 1}, "shard_names": ["shard2"], "permanent": true, "data_size": 40, "name": "stream3", "root_path": "mongodb-backup/cid1/shard2"}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"

  @events
  Scenario: Restoring from backup to original folder works
    When we POST "/mdb/mongodb/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto03"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new MongoDB cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto03",
            "clusterId": "cid2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.RestoreCluster" event with
    """
    {
        "details": {
            "backup_id": "cid1:auto03",
            "cluster_id": "cid2"
        }
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": false,
        "config": {
            "version": "4.2",
            "featureCompatibilityVersion": "4.2",
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
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "UNKNOWN",
        "id": "cid2",
        "labels": {},
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
        "name": "test_restored",
        "networkId": "",
        "status": "RUNNING"
    }
    """
    When we GET "/api/v1.0/config/iva-2.db.yandex.net"
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
                "data": "2",
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
                        "subcid2": {
                            "hosts": {},
                            "name": "mongod_subcluster",
                            "roles": [
                                "mongodb_cluster.mongod"
                            ],
                            "shards": {
                                "shard_id2": {
                                    "hosts": {
                                        "iva-2.db.yandex.net": {"geo": "iva"},
                                        "myt-2.db.yandex.net": {"geo": "myt"},
                                        "sas-2.db.yandex.net": {"geo": "sas"}
                                    },
                                    "name": "rs01"
                                }
                            }
                        }
                    }
                },
                "cluster_hosts": [
                    "iva-2.db.yandex.net",
                    "myt-2.db.yandex.net",
                    "sas-2.db.yandex.net"
                ],
                "cluster_id": "cid2",
                "cluster_name": "test_restored",
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
                "fqdn": "iva-2.db.yandex.net",
                "geo": "iva",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-2.db.yandex.net",
                    "myt-2.db.yandex.net",
                    "sas-2.db.yandex.net"
                ],
                "shard_id": "shard_id2",
                "shard_name": "rs01",
                "space_limit": 10737418240,
                "subcluster_id": "subcid2",
                "subcluster_name": "mongod_subcluster",
                "vtype": "porto",
                "vtype_id": null
            },
            "default pillar": true,
            "mongodb": {
                "cluster_auth": "keyfile",
                "cluster_name": "test_restored",
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
            "s3_bucket": "yandexcloud-dbaas-cid2",
            "versions": {
                "mongodb": {
                    "edition": "default",
                    "major_version": "4.2",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                }
            },
            "walg": {}
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """


  Scenario: Restoring from backup to next version works
    When we POST "/mdb/mongodb/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
                }
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto03"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new MongoDB cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto03",
            "clusterId": "cid2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.RestoreCluster" event with
    """
    {
        "details": {
            "backup_id": "cid1:auto03",
            "cluster_id": "cid2"
        }
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": false,
        "config": {
            "version": "4.4",
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
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
                    }
                }
            }
        },
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "UNKNOWN",
        "id": "cid2",
        "labels": {},
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
        "name": "test_restored",
        "networkId": "",
        "status": "RUNNING"
    }
    """
    When we GET "/api/v1.0/config/iva-2.db.yandex.net"
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
                "data": "2",
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
                        "subcid2": {
                            "hosts": {},
                            "name": "mongod_subcluster",
                            "roles": [
                                "mongodb_cluster.mongod"
                            ],
                            "shards": {
                                "shard_id2": {
                                    "hosts": {
                                        "iva-2.db.yandex.net": {"geo": "iva"},
                                        "myt-2.db.yandex.net": {"geo": "myt"},
                                        "sas-2.db.yandex.net": {"geo": "sas"}
                                    },
                                    "name": "rs01"
                                }
                            }
                        }
                    }
                },
                "cluster_hosts": [
                    "iva-2.db.yandex.net",
                    "myt-2.db.yandex.net",
                    "sas-2.db.yandex.net"
                ],
                "cluster_id": "cid2",
                "cluster_name": "test_restored",
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
                "fqdn": "iva-2.db.yandex.net",
                "geo": "iva",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-2.db.yandex.net",
                    "myt-2.db.yandex.net",
                    "sas-2.db.yandex.net"
                ],
                "shard_id": "shard_id2",
                "shard_name": "rs01",
                "space_limit": 10737418240,
                "subcluster_id": "subcid2",
                "subcluster_name": "mongod_subcluster",
                "vtype": "porto",
                "vtype_id": null
            },
            "default pillar": true,
            "mongodb": {
                "cluster_auth": "keyfile",
                "cluster_name": "test_restored",
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
            "s3_bucket": "yandexcloud-dbaas-cid2",
            "versions": {
                "mongodb": {
                    "edition": "default",
                    "major_version": "4.4",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                }
            },
            "walg": {}
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """


  Scenario: Restoring from backup to different folder works
    When we POST "/mdb/mongodb/1.0/clusters:restore?folderId=folder2" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:man02"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new MongoDB cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.RestoreClusterMetadata",
            "backupId": "cid1:man02",
            "clusterId": "cid2"
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder2"
    }
    """

  Scenario: Restoring from backup with less disk size fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_2": {
                "mongod": {
                    "resources": {
                        "diskSize": 21474836480
                    }
                }
            }
        }
    }
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we POST "/mdb/mongodb/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:man02"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Insufficient diskSize, increase it to '21474836480'"
    }
    """

  Scenario: Restoring from backup with nonexistent backup fails
    When we POST "/mdb/mongodb/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:shard1_id:777"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Backup 'cid1:shard1_id:777' does not exist"
    }
    """

  Scenario: Restoring from backup with incorrect disk unit fails
    When we POST "/mdb/mongodb/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_2": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418245
                    }
                }
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:man02"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Disk size must be a multiple of 4194304 bytes"
    }
    """

  @delete
  Scenario: Restore from backup belongs to deleted cluster
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Delete MongoDB cluster",
        "id": "worker_task_id2"
    }
    """
    When all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we POST "/mdb/mongodb/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:man02"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new MongoDB cluster from the backup",
        "done": false,
        "id": "worker_task_id5",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.RestoreClusterMetadata",
            "backupId": "cid1:man02",
            "clusterId": "cid2"
        }
    }
    """


  @timetravel
  Scenario: Restoring cluster on time before database delete
    . restored cluster should have that deleted database
    When we POST "/mdb/mongodb/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "other_database"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we DELETE "/mdb/mongodb/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200
    When we POST "/mdb/mongodb/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{ "zoneId": "iva" }],
        "backupId": "cid1:man02"
    }
    """
    Then we get response with status 200
    When we GET "/mdb/mongodb/1.0/clusters/cid2/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": [{
            "clusterId": "cid2",
            "name": "testdb"
        }]
    }
    """

    Scenario: Restoring from backup with recovery point fails without feature flag
    When we POST "/mdb/mongodb/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:man02",
        "recoveryTargetSpec": {
            "timestamp": 1
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

    Scenario Outline: Restoring from backup with invalid recovery point fails
    Given feature flags
    """
    ["MDB_MONGODB_RS_PITR"]
    """
    When we POST "/mdb/mongodb/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "<backup>",
        "recoveryTargetSpec": {
            "timestamp": <timestamp>
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "<message>"
    }
    """
    Examples:
    | backup                                 | timestamp   | message |
    | cid1:auto01 | 1           | Unable to restore using this backup, recovery target must be greater than or equal to 32 (use older backup or increase 'recoveryTargetSpec') |
    | cid1:auto01 | 31          | Unable to restore using this backup, recovery target must be greater than or equal to 32 (use older backup or increase 'recoveryTargetSpec') |
    | cid1:auto03 | 31          | Unable to restore using this backup, recovery target must be greater than or equal to 42 (use older backup or increase 'recoveryTargetSpec') |
    | cid1:auto01 | 33145059145 | Invalid recovery target (in future) |


    Scenario Outline: Restoring from backup with valid recovery point works
    Given feature flags
    """
    ["MDB_MONGODB_RS_PITR"]
    """
    When we POST "/mdb/mongodb/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "<backup>",
        "recoveryTargetSpec": {
            "timestamp": <timestamp>
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new MongoDB cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.RestoreClusterMetadata",
            "backupId": "<backup>",
            "clusterId": "cid2"
        }
    }
    """
    Examples:
    | backup                                 | timestamp   |
    | cid1:auto01 | 32          |
    | cid1:auto03 | 42          |
    | cid1:auto03 | 152         |


    Scenario Outline: Restoring from backup without recovery point works
    Given feature flags
    """
    ["MDB_MONGODB_RS_PITR", "MDB_MONGODB_ALLOW_DEPRECATED_VERSIONS"]
    """
    When we POST "/mdb/mongodb/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_<spec_version_suffix>": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
                }
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01"
    }
    """
    Examples:
    | version | spec_version_suffix |
    | 3.6     | 3_6                 |
    | 4.0     | 4_0                 |
    | 4.2     | 4_2                 |


    Scenario Outline: Restoring from backup with recovery point on unsupported PITR new cluster version fails
    Given feature flags
    """
    ["MDB_MONGODB_RS_PITR", "MDB_MONGODB_ALLOW_DEPRECATED_VERSIONS"]
    """
    When we POST "/mdb/mongodb/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_<spec_version_suffix>": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
                }
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01",
        "recoveryTargetSpec": {
            "timestamp": 152
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Recovery target is not supported by new cluster version: <version>"
    }
    """
    Examples:
    | version | spec_version_suffix |
    | 4.0     | 4_0                 |


    Scenario Outline: Restoring from backup with recovery point on unsupported PITR source cluster version fails
    Given feature flags
    """
    ["MDB_MONGODB_RS_PITR", "MDB_MONGODB_ALLOW_DEPRECATED_VERSIONS"]
    """
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_<spec_version_suffix>": {
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
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And all "cid2" revs committed before "1970-01-01T00:00:00+00:00"
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto011', 'DONE', '1970-01-01T03:00:30.000000+03:00'::timestamptz, DATE('1970-01-01T03:00:30.000000+03:00'), 'SCHEDULE', '{ "before_ts": {"TS": 30, "Inc": 1}, "after_ts": {"TS": 31, "Inc": 6 }, "shard_names": ["shard1"], "permanent": false,"data_size": 30, "name": "stream1", "root_path": "mongodb-backup/cid1/shard1"}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid2';
    """
    When we POST "/mdb/mongodb/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid2:auto011",
        "recoveryTargetSpec": {
            "timestamp": 152
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Recovery target is not supported by source cluster version: <version>"
    }
    """
    Examples:
    | version | spec_version_suffix |
    | 4.0     | 4_0                 |

  Scenario: Restoring from backup with valid recovery point works and disable pitr feature flag fails
    Given feature flags
    """
    ["MDB_MONGODB_RS_PITR", "MDB_MONGODB_RESTORE_WITHOUT_REPLAY"]
    """
    When we POST "/mdb/mongodb/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01",
        "recoveryTargetSpec": {
            "timestamp": 32
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

  @security_groups
  Scenario: Restoring from backup to original folder with security groups works
    When we POST "/mdb/mongodb/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
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
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "securityGroupIds": ["sg_id2", "sg_id3"],
        "backupId": "cid1:man02"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new MongoDB cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.RestoreClusterMetadata",
            "backupId": "cid1:man02",
            "clusterId": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" containing:
      |sg_id2|
      |sg_id3|
    When "worker_task_id2" acquired and finished by worker
    And worker set "sg_id2,sg_id3" security groups on "cid2"
    When we GET "/mdb/mongodb/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": ["sg_id2", "sg_id3"]
    }
    """
