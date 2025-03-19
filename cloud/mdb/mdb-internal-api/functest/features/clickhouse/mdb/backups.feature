Feature: Backup ClickHouse cluster

  Background:
    Given default headers
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
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
        "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker

  @events
  Scenario: Backup creation works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:backup"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create a backup for ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.BackupClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Backup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create a backup for ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.BackupClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.BackupCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Create a backup for ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.BackupClusterMetadata",
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

  @events
  Scenario: Backup while backup is going fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:backup"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create a backup for ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.BackupClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Backup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create a backup for ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.BackupClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.BackupCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:backup"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Conflicting operation worker_task_id2 detected"
    }
    """
    When we "Backup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "conflicting operation "worker_task_id2" detected"

  Scenario: Backup list works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/shard1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/backups?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:11+00:00",
                "folderId": "folder1",
                "id": "cid1:1",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:10+00:00"
            },
            {
                "createdAt": "1970-01-01T00:00:01+00:00",
                "folderId": "folder1",
                "id": "cid1:0",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:00+00:00"
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "1970-01-01T00:00:11Z",
                "folder_id": "folder1",
                "id": "cid1:1",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:10Z"
            },
            {
                "created_at": "1970-01-01T00:00:01Z",
                "folder_id": "folder1",
                "id": "cid1:0",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:00Z"
            }
        ]
    }
    """

  Scenario: Backup list with legacy S3 layout works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/backups?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:11+00:00",
                "folderId": "folder1",
                "id": "cid1:1",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:10+00:00"
            },
            {
                "createdAt": "1970-01-01T00:00:01+00:00",
                "folderId": "folder1",
                "id": "cid1:0",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:00+00:00"
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "1970-01-01T00:00:11Z",
                "folder_id": "folder1",
                "id": "cid1:1",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:10Z"
            },
            {
                "created_at": "1970-01-01T00:00:01Z",
                "folder_id": "folder1",
                "id": "cid1:0",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:00Z"
            }
        ]
    }
    """

  Scenario: Backup list with ignoring entries works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/shard1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S %z",
                        "start_time": "1970-01-01 03:00:00 +0300",
                        "end_time": "1970-01-01 03:00:01 +0300",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "partially_deleted"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S %z",
                        "start_time": "1970-01-01 00:03:10 +0300",
                        "end_time": "1970-01-01 00:03:11 +0300",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "deleting"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/2/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S %z",
                        "start_time": "1970-01-01 03:00:20 +0300",
                        "end_time": "1970-01-01 03:00:21 +0300",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/3/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S %z",
                        "start_time": "1970-01-01 03:00:30 +0300",
                        "end_time": "1970-01-01 03:00:31 +0300",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "failed"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/4/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S %z",
                        "start_time": "1970-01-01 03:00:40 +0300",
                        "end_time": null,
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "creating"
                    }
                }
            }
        ]
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/backups?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "folderId": "folder1",
                "id": "cid1:2",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:20+00:00",
                "createdAt": "1970-01-01T00:00:21+00:00"
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "folder_id": "folder1",
                "id": "cid1:2",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:20Z",
                "created_at": "1970-01-01T00:00:21Z"
            }
        ]
    }
    """

  Scenario: Backup list with page size works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/shard1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/backups?folderId=folder1&pageSize=1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:11+00:00",
                "folderId": "folder1",
                "id": "cid1:1",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:10+00:00"
            }
        ],
        "nextPageToken": "Y2lkMTox"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "folder_id": "folder1",
        "page_size": 1
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "1970-01-01T00:00:11Z",
                "folder_id": "folder1",
                "id": "cid1:1",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:10Z"
            }
        ],
        "next_page_token": "eyJDbHVzdGVySUQiOiJjaWQxIiwiQmFja3VwSUQiOiIxIn0="
    }
    """

  Scenario: Backup list with page token works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/shard1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/backups?folderId=folder1&pageToken=Y2lkMTox"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:01+00:00",
                "folderId": "folder1",
                "id": "cid1:0",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:00+00:00"
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "folder_id": "folder1",
        "page_token": "eyJDbHVzdGVySUQiOiJjaWQxIiwiQmFja3VwSUQiOiIxIn0="
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "1970-01-01T00:00:01Z",
                "folder_id": "folder1",
                "id": "cid1:0",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:00Z"
            }
        ]
    }
    """

  Scenario: Backup list by cid works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/shard1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:11+00:00",
                "folderId": "folder1",
                "id": "cid1:1",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:10+00:00"
            },
            {
                "createdAt": "1970-01-01T00:00:01+00:00",
                "folderId": "folder1",
                "id": "cid1:0",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:00+00:00"
            }
        ]
    }
    """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "1970-01-01T00:00:11Z",
                "folder_id": "folder1",
                "id": "cid1:1",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:10Z"
            },
            {
                "created_at": "1970-01-01T00:00:01Z",
                "folder_id": "folder1",
                "id": "cid1:0",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:00Z"
            }
        ],
        "next_page_token": ""
    }
    """

  Scenario: Backup list on cluster with no backups works
    Given s3 response
    """
    {}
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": []
    }
    """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": []
    }
    """

  Scenario: Backup get by id works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/shard1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/backups/cid1:1"
    Then we get response with status 200 and body contains
    """
    {
        "createdAt": "1970-01-01T00:00:11+00:00",
        "folderId": "folder1",
        "id": "cid1:1",
        "sourceClusterId": "cid1",
        "sourceShardNames": ["shard1"],
        "startedAt": "1970-01-01T00:00:10+00:00"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_at": "1970-01-01T00:00:11Z",
        "folder_id": "folder1",
        "id": "cid1:1",
        "source_cluster_id": "cid1",
        "source_shard_names": ["shard1"],
        "started_at": "1970-01-01T00:00:10Z"
    }
    """

  @delete
  Scenario: After cluster delete its backups are shown
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/shard1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Delete ClickHouse cluster",
        "id": "worker_task_id2"
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "description": "Delete ClickHouse cluster",
        "id": "worker_task_id2"
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.DeleteCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:11+00:00",
                "folderId": "folder1",
                "id": "cid1:1",
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:10+00:00",
                "sourceShardNames": ["shard1"]
            },
            {
                "createdAt": "1970-01-01T00:00:01+00:00",
                "folderId": "folder1",
                "id": "cid1:0",
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:00+00:00",
                "sourceShardNames": ["shard1"]
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "folder_id": "folder1",
        "page_size": 5
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "1970-01-01T00:00:11Z",
                "folder_id": "folder1",
                "id": "cid1:1",
                "source_cluster_id": "cid1",
                "started_at": "1970-01-01T00:00:10Z",
                "source_shard_names": ["shard1"]
            },
            {
                "created_at": "1970-01-01T00:00:01Z",
                "folder_id": "folder1",
                "id": "cid1:0",
                "source_cluster_id": "cid1",
                "started_at": "1970-01-01T00:00:00Z",
                "source_shard_names": ["shard1"]
            }
        ],
        "next_page_token": ""
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/backups/cid1:1"
    Then we get response with status 200 and body contains
    """
    {
        "createdAt": "1970-01-01T00:00:11+00:00",
        "folderId": "folder1",
        "id": "cid1:1",
        "sourceClusterId": "cid1",
        "startedAt": "1970-01-01T00:00:10+00:00",
        "sourceShardNames": ["shard1"]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_at": "1970-01-01T00:00:11Z",
        "folder_id": "folder1",
        "id": "cid1:1",
        "source_cluster_id": "cid1",
        "started_at": "1970-01-01T00:00:10Z",
        "source_shard_names": ["shard1"]
    }
    """
    # backups for purged cluster not available
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/backups"
    Then we get response with status 403
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And "worker_task_id3" acquired and finished by worker
    And "worker_task_id4" acquired and finished by worker
    And we "GET" via REST at "/mdb/clickhouse/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": []
    }
    """
    And we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "folder_id": "folder1",
        "page_size": 5
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [],
        "next_page_token": ""
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/backups/cid1:1"
    Then we get response with status 403
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:1"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "cluster id "cid1" not found"

  Scenario: Backup list use light metadata file
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/shard1/0/backup_light_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/shard1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": "This file is not used"
                }
            }
        ]
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/backups?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:01+00:00",
                "folderId": "folder1",
                "id": "cid1:0",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:00+00:00"
            }
        ]
    }
    """
    And we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "1970-01-01T00:00:01Z",
                "folder_id": "folder1",
                "id": "cid1:0",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:00Z"
            }
        ]
    }
    """
