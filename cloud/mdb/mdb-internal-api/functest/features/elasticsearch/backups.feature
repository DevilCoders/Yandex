@elasticsearch
@grpc_api
Feature: Backup Elasticsearch cluster
  Background:
    Given default headers
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.14",
            "edition": "platinum",
            "admin_password": "admin_password",
            "elasticsearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: Backup creation works
    When we "Backup" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create a backup for ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.BackupClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Create a backup for ElasticSearch cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.BackupClusterMetadata",
            "cluster_id": "cid1"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.Cluster",
            "created_at": "**IGNORE**",
            "description": "test cluster",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "id": "cid1",
            "name": "test",
            "network_id": "network1",
            "status": "RUNNING",
            "maintenance_window": { "anytime": {} },
            "config": {
                "version": "7.14",
                "edition": "platinum",
                "elasticsearch": {
                    "data_node": {
                        "elasticsearch_config_set_7": "**IGNORE**",
                        "resources": {
                            "resource_preset_id": "s1.compute.1",
                            "disk_type_id": "network-ssd",
                            "disk_size": "10737418240"
                        }
                    }
                }
            },
            "monitoring": "**IGNORE**"
        }
    }
    """

  Scenario: Backup while backup is going fails
    When we "Backup" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create a backup for ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.BackupClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    When we "Backup" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
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
                "Key": "backups/.yc-metadata.json",
                "LastModified": 1,
                "Body": {
                    "version" : 1,
                    "snapshots": [
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        }
                    ]
                }
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.BackupService" with data
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
               "id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g",
               "created_at": "2021-09-17T07:14:06.836Z",
               "folder_id": "folder1",
               "source_cluster_id": "cid1",
               "elasticsearch_version": "7.13.4",
               "indices": [
                   ".kibana_security_session_1",
                   ".kibana_task_manager_7.13.4_001", ".mdb_dns_upper",
                   ".kibana_7.13.4_001"
               ],
               "indices_total": "11",
               "size_bytes": "2916841",
               "started_at": "2021-09-17T07:13:59.833Z"
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
                "Key": "backups/.yc-metadata.json",
                "LastModified": 1,
                "Body": {
                    "version" : 1,
                    "snapshots": [
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        },
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g2",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        }
                    ]
                }
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.BackupService" with data
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
               "id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g",
               "created_at": "2021-09-17T07:14:06.836Z",
               "folder_id": "folder1",
               "source_cluster_id": "cid1",
               "elasticsearch_version": "7.13.4",
               "indices": [
                   ".kibana_security_session_1",
                   ".kibana_task_manager_7.13.4_001", ".mdb_dns_upper",
                   ".kibana_7.13.4_001"
               ],
               "indices_total": "11",
               "size_bytes": "2916841",
               "started_at": "2021-09-17T07:13:59.833Z"
           }
        ],
        "next_page_token": "eyJDbHVzdGVySUQiOiJjaWQxIiwiQmFja3VwSUQiOiJzbmFwc2hvdC0xZ3lubXgwd3NidXloeWg2bWI0aC1nIn0="
    }
    """

  Scenario: Backup list with page token works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "backups/.yc-metadata.json",
                "LastModified": 1,
                "Body": {
                    "version" : 1,
                    "snapshots": [
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        },
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g2",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        }
                    ]
                }
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.BackupService" with data
    """
    {
        "folder_id": "folder1",
        "page_token": "eyJDbHVzdGVySUQiOiJjaWQxIiwiQmFja3VwSUQiOiJzbmFwc2hvdC0xZ3lubXgwd3NidXloeWg2bWI0aC1nIn0="
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
               "id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g2",
               "created_at": "2021-09-17T07:14:06.836Z",
               "folder_id": "folder1",
               "source_cluster_id": "cid1",
               "elasticsearch_version": "7.13.4",
               "indices": [
                   ".kibana_security_session_1",
                   ".kibana_task_manager_7.13.4_001", ".mdb_dns_upper",
                   ".kibana_7.13.4_001"
               ],
               "indices_total": "11",
               "size_bytes": "2916841",
               "started_at": "2021-09-17T07:13:59.833Z"
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
                "Key": "backups/.yc-metadata.json",
                "LastModified": 1,
                "Body": {
                    "version" : 1,
                    "snapshots": [
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        },
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g2",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        }
                    ]
                }
            }
        ]
    }
    """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
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
               "id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g",
               "created_at": "2021-09-17T07:14:06.836Z",
               "folder_id": "folder1",
               "source_cluster_id": "cid1",
               "elasticsearch_version": "7.13.4",
               "indices": [
                   ".kibana_security_session_1",
                   ".kibana_task_manager_7.13.4_001", ".mdb_dns_upper",
                   ".kibana_7.13.4_001"
               ],
               "indices_total": "11",
               "size_bytes": "2916841",
               "started_at": "2021-09-17T07:13:59.833Z"
            },
            {
               "id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g2",
               "created_at": "2021-09-17T07:14:06.836Z",
               "folder_id": "folder1",
               "source_cluster_id": "cid1",
               "elasticsearch_version": "7.13.4",
               "indices": [
                   ".kibana_security_session_1",
                   ".kibana_task_manager_7.13.4_001", ".mdb_dns_upper",
                   ".kibana_7.13.4_001"
               ],
               "indices_total": "11",
               "size_bytes": "2916841",
               "started_at": "2021-09-17T07:13:59.833Z"
           }
        ]
    }
    """

  Scenario: Backup list on cluster with no backups works
    Given s3 response
    """
    {}
    """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
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
                "Key": "backups/.yc-metadata.json",
                "LastModified": 1,
                "Body": {
                    "version" : 1,
                    "snapshots": [
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        },
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g2",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        }
                    ]
                }
            }
        ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g"
    }
    """
    Then we get gRPC response with body
    """
    {
       "id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g",
       "created_at": "2021-09-17T07:14:06.836Z",
       "folder_id": "folder1",
       "source_cluster_id": "cid1",
       "elasticsearch_version": "7.13.4",
       "indices": [
           ".kibana_security_session_1",
           ".kibana_task_manager_7.13.4_001", ".mdb_dns_upper",
           ".kibana_7.13.4_001"
       ],
       "indices_total": "11",
       "size_bytes": "2916841",
       "started_at": "2021-09-17T07:13:59.833Z"
    }
    """

  @delete
  Scenario: After cluster delete its backups are shown
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "backups/.yc-metadata.json",
                "LastModified": 1,
                "Body": {
                    "version" : 1,
                    "snapshots": [
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        },
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g2",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        }
                    ]
                }
            }
        ]
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "description": "Delete ElasticSearch cluster",
        "id": "worker_task_id2"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.BackupService" with data
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
               "id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g",
               "created_at": "2021-09-17T07:14:06.836Z",
               "folder_id": "folder1",
               "source_cluster_id": "cid1",
               "elasticsearch_version": "7.13.4",
               "indices": [
                   ".kibana_security_session_1",
                   ".kibana_task_manager_7.13.4_001", ".mdb_dns_upper",
                   ".kibana_7.13.4_001"
               ],
               "indices_total": "11",
               "size_bytes": "2916841",
               "started_at": "2021-09-17T07:13:59.833Z"
            },
            {
               "id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g2",
               "created_at": "2021-09-17T07:14:06.836Z",
               "folder_id": "folder1",
               "source_cluster_id": "cid1",
               "elasticsearch_version": "7.13.4",
               "indices": [
                   ".kibana_security_session_1",
                   ".kibana_task_manager_7.13.4_001", ".mdb_dns_upper",
                   ".kibana_7.13.4_001"
               ],
               "indices_total": "11",
               "size_bytes": "2916841",
               "started_at": "2021-09-17T07:13:59.833Z"
           }
        ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g"
    }
    """
    Then we get gRPC response with body
    """
    {
       "id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g",
       "created_at": "2021-09-17T07:14:06.836Z",
       "folder_id": "folder1",
       "source_cluster_id": "cid1",
       "elasticsearch_version": "7.13.4",
       "indices": [
           ".kibana_security_session_1",
           ".kibana_task_manager_7.13.4_001", ".mdb_dns_upper",
           ".kibana_7.13.4_001"
       ],
       "indices_total": "11",
       "size_bytes": "2916841",
       "started_at": "2021-09-17T07:13:59.833Z"
    }
    """
    # backups for purged cluster not available
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "cluster id "cid1" not found"
    When "worker_task_id2" acquired and finished by worker
    And "worker_task_id3" acquired and finished by worker
    And "worker_task_id4" acquired and finished by worker
    And we "List" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.BackupService" with data
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:1"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "cluster id "cid1" not found"

  Scenario: Restore cluster from backup
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "backups/.yc-metadata.json",
                "LastModified": 1,
                "Body": {
                    "version" : 1,
                    "snapshots": [
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        },
                        {
                            "id": "snapshot-1gynmx0wsbuyhyh6mb4h-g2",
                            "version": "7.13.4",
                            "start_time_ms": 1631862839833,
                            "end_time_ms": 1631862846836,
                            "indices_total": 11,
                            "indices": [
                                ".kibana_security_session_1",
                                ".kibana_task_manager_7.13.4_001",
                                ".mdb_dns_upper",
                                ".kibana_7.13.4_001"
                            ],
                            "size": 2916841
                        }
                    ]
                }
            }
        ]
    }
    """  
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "backup_id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g",
        "folder_id": "folder1",
        "name": "restore",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.14",
            "edition": "platinum",
            "admin_password": "admin_password",
            "elasticsearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Restore ElasticSearch cluster from backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.RestoreClusterMetadata",
            "backup_id": "cid1:snapshot-1gynmx0wsbuyhyh6mb4h-g",
            "cluster_id": "cid2"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
