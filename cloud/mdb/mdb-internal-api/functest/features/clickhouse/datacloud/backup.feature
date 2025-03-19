@grpc_api
Feature: Backup DataCloud ClickHouse Cluster

  Background:
    Given default headers
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster",
        "description": "test cluster",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "replica_count": 3
            }
        }
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"

  Scenario: Backup creation works
    When we "Backup" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "manual_backup"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "labels" containing map:
      | name | manual_backup |

  Scenario: Backup while backup is going fails
    When we "Backup" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "manual_backup"
    }
    """
    Then we get DataCloud response OK
    When we "Backup" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "another"
    }
    """
    Then we get DataCloud response error with code FAILED_PRECONDITION and message "conflicting operation "worker_task_id2" detected"

  Scenario: Backup list works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/s1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "name": "manual_backup_name",
                            "shard_name": "s1"
                        },
                        "bytes": 777,
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/s1/1/backup_light_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "bytes": 1024,
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "List" via DataCloud at "datacloud.clickhouse.v1.BackupService" with data
    """
    {
        "project_id": "folder1"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "backups": [
            {
                "create_time": "1970-01-01T00:00:11Z",
                "project_id": "folder1",
                "id": "cid1:1",
                "source_cluster_id": "cid1",
                "start_time": "1970-01-01T00:00:10Z",
                "name": "cid1:1:s1",
                "size": "1024",
                "type": "TYPE_AUTOMATED"
            },
            {
                "create_time": "1970-01-01T00:00:01Z",
                "project_id": "folder1",
                "id": "cid1:0",
                "source_cluster_id": "cid1",
                "start_time": "1970-01-01T00:00:00Z",
                "name": "manual_backup_name:s1",
                "size": "777",
                "type": "TYPE_MANUAL"
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
                "Key": "ch_backup/cid1/s1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S %z",
                        "start_time": "1970-01-01 03:00:00 +0300",
                        "end_time": "1970-01-01 03:00:01 +0300",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "state": "partially_deleted"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/s1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S %z",
                        "start_time": "1970-01-01 00:03:10 +0300",
                        "end_time": "1970-01-01 00:03:11 +0300",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "state": "deleting"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/s1/2/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S %z",
                        "start_time": "1970-01-01 03:00:20 +0300",
                        "end_time": "1970-01-01 03:00:21 +0300",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "bytes": 1024,
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/s1/3/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S %z",
                        "start_time": "1970-01-01 03:00:30 +0300",
                        "end_time": "1970-01-01 03:00:31 +0300",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "state": "failed"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/s1/4/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S %z",
                        "start_time": "1970-01-01 03:00:40 +0300",
                        "end_time": null,
                        "labels": {
                            "shard_name": "s1"
                        },
                        "state": "creating"
                    }
                }
            }
        ]
    }
    """
    When we "List" via DataCloud at "datacloud.clickhouse.v1.BackupService" with data
    """
    {
        "project_id": "folder1"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "backups": [
            {
                "project_id": "folder1",
                "id": "cid1:2",
                "source_cluster_id": "cid1",
                "start_time": "1970-01-01T00:00:20Z",
                "create_time": "1970-01-01T00:00:21Z",
                "name": "cid1:2:s1",
                "size": "1024",
                "type": "TYPE_AUTOMATED"
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
                "Key": "ch_backup/cid1/s1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/s1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "s1"
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
                "sourceShardNames": ["s1"],
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
                "source_shard_names": ["s1"],
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
                "Key": "ch_backup/cid1/s1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "bytes": 1,
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/s1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "bytes": 2,
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "List" via DataCloud at "datacloud.clickhouse.v1.BackupService" with data
    """
    {
        "project_id": "folder1",
        "paging": {
            "page_token": "eyJDbHVzdGVySUQiOiJjaWQxIiwiQmFja3VwSUQiOiIxIn0="
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "backups": [
            {
                "create_time": "1970-01-01T00:00:01Z",
                "project_id": "folder1",
                "id": "cid1:0",
                "source_cluster_id": "cid1",
                "start_time": "1970-01-01T00:00:00Z",
                "name": "cid1:0:s1",
                "size": "1",
                "type": "TYPE_AUTOMATED"
            }
        ]
    }
    """

  Scenario: Backup list on cluster with no backups works
    Given s3 response
    """
    {}
    """
    When we "List" via DataCloud at "datacloud.clickhouse.v1.BackupService" with data
    """
    {
        "project_id": "folder1"
    }
    """
    Then we get DataCloud response with body
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
                "Key": "ch_backup/cid1/s1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "bytes": 1,
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/s1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "bytes": 1,
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "create_time": "1970-01-01T00:00:11Z",
        "project_id": "folder1",
        "id": "cid1:1",
        "source_cluster_id": "cid1",
        "started_at": "1970-01-01T00:00:10Z",
        "name": "cid1:1:s1",
        "size": "1",
        "type": "TYPE_AUTOMATED"
    }
    """

  @delete
  Scenario: After cluster delete its backups are shown
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/s1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "name": "manual_backup_name",
                            "shard_name": "s1"
                        },
                        "bytes": 777,
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/s1/1/backup_light_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "bytes": 1024,
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "Delete" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response OK
    And "worker_task_id2" acquired and finished by worker
    When we "List" via DataCloud at "datacloud.clickhouse.v1.BackupService" with data
    """
    {
        "project_id": "folder1"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "backups": [
            {
                "create_time": "1970-01-01T00:00:11Z",
                "project_id": "folder1",
                "id": "cid1:1",
                "source_cluster_id": "cid1",
                "start_time": "1970-01-01T00:00:10Z",
                "name": "cid1:1:s1",
                "size": "1024",
                "type": "TYPE_AUTOMATED"
            },
            {
                "create_time": "1970-01-01T00:00:01Z",
                "project_id": "folder1",
                "id": "cid1:0",
                "source_cluster_id": "cid1",
                "start_time": "1970-01-01T00:00:00Z",
                "name": "manual_backup_name:s1",
                "size": "777",
                "type": "TYPE_MANUAL"
            }
        ]
    }
    """
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "create_time": "1970-01-01T00:00:11Z",
        "project_id": "folder1",
        "id": "cid1:1",
        "source_cluster_id": "cid1",
        "start_time": "1970-01-01T00:00:10Z",
        "name": "cid1:1:s1",
        "size": "1024",
        "type": "TYPE_AUTOMATED"
    }
    """
    # backups for purged cluster not available
    When "worker_task_id3" acquired and finished by worker
    And "worker_task_id4" acquired and finished by worker
    Then we "List" via DataCloud at "datacloud.clickhouse.v1.BackupService" with data
    """
    {
        "project_id": "folder1"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "backups": []
    }
    """
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:1"
    }
    """
    Then we get DataCloud response error with code NOT_FOUND and message "cluster id "cid1" not found"

  Scenario: Restoring from backup works
    And s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/s1/0/backup_light_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "bytes": 1024,
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "Restore" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "name": "test_restored",
        "region_id": "eu-central1",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "replica_count": 3
            }
        },
        "backup_id": "cid1:0"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "cluster_id": "cid2",
        "operation_id": "worker_task_id2"
    }
    """
    When we GET via PillarConfig "/v1/config/ach-ec1a-s1-1.cid2.yadc.io?target-pillar-id=pillar_id1"
    Then we get response with status 200
    And body at path "$.data" contains
    """
    {
        "restore-from-pillar-data": {
            "backup": {
                "sleep": 7200,
                "start": {
                    "hours":0,
                    "minutes": 0,
                    "nanos": 0,
                    "seconds": 0
                },
                "use_backup_service": false
            },
            "clickhouse": {
                "admin_password": {
                    "hash": {
                        "data": "61a1438d394875ea4d8aec1ac5d8c66e46735ee5f5e3e23e3af558b49385586b",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "random_string1",
                        "encryption_version": 0
                    }
                },
                "ch_version": "22.5.1.2079",
                "cluster_name": "default",
                    "config": {
                    "merge_tree": {
                        "enable_mixed_granularity_parts": true
                    }
                },
                "databases": [],
                "embedded_keeper": true,
                "mysql_protocol": false,
                "postgresql_protocol": false,
                "format_schemas": {},
                "interserver_credentials": {
                    "password": {
                        "data": "random_string2",
                        "encryption_version": 0
                    },
                    "user": "interserver"
                },
                "models": {},
                "shards": {
                    "shard_id1": {
                        "weight": 100
                    }
                },
                "sql_database_management": true,
                "sql_user_management": true,
                "system_users": {
                    "mdb_backup_admin": {
                        "hash": {
                            "data": "0bfa31cac3b32d99936b6643c4d410717f1c82e4ba6a85cfbd175fe96fbbc818",
                            "encryption_version": 0
                        },
                        "password": {
                            "data": "random_string4",
                            "encryption_version": 0
                        }
                    }
                },
                "user_management_v2": true,
                "users": {},
                "keeper_hosts": {
                    "ach-ec1a-s1-1.cid1.yadc.io": 1,
                    "ach-ec1b-s1-2.cid1.yadc.io": 2,
                    "ach-ec1c-s1-3.cid1.yadc.io": 3
                },
                "zk_users": {
                    "clickhouse": {
                        "password": {
                            "data": "random_string3",
                            "encryption_version": 0
                        }
                    }
                }
            },
            "cloud_storage": {
                "enabled": true,
                "s3": {
  		            "bucket": "cloud-storage-cid1"
                },
                "settings": {
                    "data_cache_enabled": true,
                    "data_cache_max_size": 17179869184
                }
            },
            "cloud_type": "aws",
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "region_id": "eu-central1",
            "s3_bucket": "yandexcloud-dbaas-cid1",
            "testing_repos": false
        },
        "backup": {
            "sleep": 7200,
            "start": {
                "hours": 0,
                "nanos": 0,
                "minutes": 0,
                "seconds": 0
            },
            "use_backup_service": false
        },
        "versions": {},
        "region_id": "eu-central1",
        "s3_bucket": "yandexcloud-dbaas-cid2",
        "unmanaged": {
            "enable_zk_tls": true
        },
        "clickhouse": {
            "users": {},
            "databases": [],
            "config": {
                "timezone": "Europe/Moscow",
                "log_level": "information",
                "merge_tree": {
                    "enable_mixed_granularity_parts": true,
                    "replicated_deduplication_window": 100,
                    "replicated_deduplication_window_seconds": 604800
                },
                "compression": [],
                "mark_cache_size": 5368709120,
                "max_connections": 4096,
                "keep_alive_timeout": 3,
                "max_concurrent_queries": 500,
                "max_table_size_to_drop": 53687091200,
                "uncompressed_cache_size": 8589934592,
                "max_partition_size_to_drop": 53687091200,
                "builtin_dictionaries_reload_interval": 3600
            },
            "models": {},
            "shards": {
                "shard_id2": {
                    "weight": 100
                }
            },
            "keeper_hosts": {
                "ach-ec1a-s1-1.cid2.yadc.io": 1,
                "ach-ec1b-s1-2.cid2.yadc.io": 2,
                "ach-ec1c-s1-3.cid2.yadc.io": 3
            },
            "zk_users": {
                "clickhouse": {
                    "password": {
                        "data": "random_string7",
                        "encryption_version": 0
                    }
                }
            },
            "ch_version": "22.5.1.2079",
            "cluster_name": "default",
            "admin_password": {
                "hash": {
                    "data": "0df29e371e2ed1e651a42bd49980a66fed3918312113612de787874bd0b2384f",
                    "encryption_version": 0
                },
                "password": {
                    "data": "random_string5",
                    "encryption_version": 0
                }
            },
            "embedded_keeper": true,
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "format_schemas": {},
            "user_management_v2": true,
            "sql_user_management": true,
            "system_users": {
                "mdb_backup_admin": {
                    "hash": {
                        "data": "5a743285373d78e140493344940f9d4c065b624b932ed2ba1109169917345ca2",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "random_string8",
                        "encryption_version": 0
                    }
                }
            },
            "embedded_keeper": true,
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "interserver_credentials": {
                "user": "interserver",
                "password": {
                    "data": "random_string6",
                    "encryption_version": 0
                }
            },
            "sql_database_management": true
        },
        "cloud_type": "aws",
        "clickhouse default pillar": true,
        "cloud_storage": {
                "enabled": true,
                "s3": {
  		            "bucket": "cloud-storage-cid2"
                },
                "settings": {
                    "data_cache_enabled": true,
                    "data_cache_max_size": 17179869184
                }
        },
        "testing_repos": false,
        "default pillar": true,
        "cluster_private_key": {
            "data": "2",
            "encryption_version": 0
        }
    }
    """
    And body at path "$.data.dbaas" contains
    """
    {
        "fqdn": "ach-ec1a-s1-1.cid2.yadc.io",
        "geo": "eu-central-1a",
        "region": "eu-central1",
        "cloud_provider": "aws",
        "cloud": {
            "cloud_ext_id": "cloud1"
        },
        "vtype": "aws",
        "flavor": {
            "id": "00000000-0000-0000-0000-000000000046",
            "name": "a1.aws.1",
            "type": "aws-standard",
            "vtype": "aws",
            "io_limit": 20971520,
            "cpu_limit": 1.0,
            "gpu_limit": 0,
            "generation": 1,
            "description": "a1.aws.1",
            "platform_id": "mdb-v1",
            "cpu_fraction": 100,
            "memory_limit": 4294967296,
            "cpu_guarantee": 1.0,
            "network_limit": 16777216,
            "io_cores_limit": 0,
            "memory_guarantee": 4294967296,
            "network_guarantee": 16777216
        },
        "folder": {
            "folder_ext_id": "folder1"
        },
        "cluster": {
            "subclusters": {
                "subcid2": {
                    "name": "clickhouse_subcluster",
                    "hosts": {},
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "shards": {
                        "shard_id2": {
                            "name": "s1",
                            "hosts": {
                                "ach-ec1a-s1-1.cid2.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s1-2.cid2.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1c-s1-3.cid2.yadc.io": {"geo": "eu-central-1c"}
                            }
                        }
                    }
                }
            }
        },
        "shard_id": "shard_id2",
        "vtype_id": null,
        "cluster_id": "cid2",
        "shard_name": "s1",
        "shard_hosts": [
            "ach-ec1a-s1-1.cid2.yadc.io",
            "ach-ec1b-s1-2.cid2.yadc.io",
            "ach-ec1c-s1-3.cid2.yadc.io"
        ],
        "space_limit": 34359738368,
        "cluster_name": "test_restored",
        "cluster_type": "clickhouse_cluster",
        "disk_type_id": "local-ssd",
        "cluster_hosts": [
            "ach-ec1a-s1-1.cid2.yadc.io",
            "ach-ec1b-s1-2.cid2.yadc.io",
            "ach-ec1c-s1-3.cid2.yadc.io"
        ],
        "subcluster_id": "subcid2",
        "subcluster_name": "clickhouse_subcluster",
        "assign_public_ip": true,
        "on_dedicated_host": false
    }
    """

  Scenario: Restoring from backup with values from source cluster works
    And s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/s1/0/backup_light_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "s1"
                        },
                        "bytes": 1024,
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "Restore" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "name": "test_restored",
        "resources": {
            "clickhouse": {
                "disk_size": 34359738368,
                "replica_count": 3
            }
        },
        "backup_id": "cid1:0"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "cluster_id": "cid2",
        "operation_id": "worker_task_id2"
    }
    """
