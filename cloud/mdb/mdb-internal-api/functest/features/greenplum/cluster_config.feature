@greenplum
@grpc_api
Feature: Greenplum_config
    Background:
        Given default headers
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
                "fqdn": "myt-1.df.cloud.yandex.net",
                "cid": "cid1",
                "status": "Alive",
                "services": [
                    {
                        "name": "greenplum",
                        "role": "Master",
                        "status": "Alive",
                        "timestamp": 1600860350
                    }
                ]
            }
        ]
        }
        """
        When we add cloud "cloud1"
        And we add folder "folder1" to cloud "cloud1"
        When we add default feature flag "MDB_GREENPLUM_CLUSTER"
        When we add default feature flag "MDB_GREENPLUM_ALLOW_LOW_MEM"
    Scenario: Create with params
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "folder_id": "folder1",
            "environment": "PRESTABLE",
            "name": "test",
            "description": "test cluster",
            "labels": {
                "foo": "bar"
            },
            "network_id": "network1",
            "config": {
                "version": "6.19",
                "zone_id": "myt",
                "subnet_id": "",
                "assign_public_ip": false,
                "backup_window_start": {
                    "hours":23,
                    "minutes":20,
                    "seconds":40,
                    "nanos":200
                },
                "access": {
                    "data_lens": true,
                    "web_sql": true,
                    "data_transfer": true,
                    "serverless": true
                }
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.2",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "config_spec": {
                "greenplum_config_6_19": {
                    "max_connections": 989
                },
                "pool": {
                    "size": 1489
                }
            },
            "master_host_count": 2,
            "segment_in_host": 3,
            "segment_host_count": 4,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Create Greenplum cluster",
            "done": false,
            "id": "worker_task_id1",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.CreateClusterMetadata",
                "cluster_id": "cid1"
            },
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "code": 0,
                "details": [],
                "message": "OK"
            }
        }
        """
        And "worker_task_id1" acquired and finished by worker
        When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "id": "cid1",
            "environment": "PRESTABLE",
            "name": "test",
            "description": "test cluster",
            "labels": {
                "foo": "bar"
            },
            "network_id": "network1",
            "config": {
                "access": {
                    "data_lens": true,
                    "web_sql": true,
                    "data_transfer": true,
                    "serverless": true
                },
                "version": "6.19",
                "zone_id": "myt",
                "subnet_id": "",
                "assign_public_ip": false,
                "segment_mirroring_enable": true,
                "segment_auto_rebalance": null,
                "backup_window_start": {
                    "hours":23,
                    "minutes":20,
                    "seconds":40,
                    "nanos":200
                }
            },
            "health": "ALIVE",
            "status": "RUNNING",
            "master_host_count": "2",
            "segment_in_host": "3",
            "segment_host_count": "4",
            "user_name": "usr1",
            "master_config": {
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                },
                "config": null
            },
            "segment_config": {
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "config": null
            },
            "cluster_config": {
                "pool": {
                    "default_config": {
                        "client_idle_timeout": "1489",
                        "mode": "SESSION",
                        "size": "1480"
                    },
                    "effective_config": {
                        "client_idle_timeout": "1489",
                        "mode": "SESSION",
                        "size": "1489"
                    },
                    "user_config": {
                        "size": "1489",
                        "client_idle_timeout": null,
                        "mode": "POOL_MODE_UNSPECIFIED"
                    }
                },
                "greenplum_config_set_6_19": {
                    "user_config": {
                        "gp_workfile_compression": null,
                        "gp_workfile_limit_files_per_query": null,
                        "gp_workfile_limit_per_query": null,
                        "gp_workfile_limit_per_segment": null,
                        "log_statement": "LOG_STATEMENT_UNSPECIFIED",
                        "max_connections": "989",
                        "max_prepared_transactions": null,
                        "max_slot_wal_keep_size": null,
                        "max_statement_mem": null
                        },
                    "effective_config": {
                        "gp_workfile_compression": null,
                        "gp_workfile_limit_files_per_query": null,
                        "gp_workfile_limit_per_query": null,
                        "gp_workfile_limit_per_segment": null,
                        "log_statement": "LOG_STATEMENT_UNSPECIFIED",
                        "max_connections": "989",
                        "max_prepared_transactions": null,
                        "max_slot_wal_keep_size": null,
                        "max_statement_mem": null
                        },
                    "default_config": {
                        "gp_workfile_compression": null,
                        "gp_workfile_limit_files_per_query": null,
                        "gp_workfile_limit_per_query": null,
                        "gp_workfile_limit_per_segment": null,
                        "log_statement": "LOG_STATEMENT_UNSPECIFIED",
                        "max_connections": "250",
                        "max_prepared_transactions": null,
                        "max_slot_wal_keep_size": null,
                        "max_statement_mem": null
                        }
                }
            }
        }
        """
    Scenario: Create with empty params
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "folder_id": "folder1",
            "environment": "PRESTABLE",
            "name": "test",
            "description": "test cluster",
            "labels": {
                "foo": "bar"
            },
            "network_id": "network1",
            "config": {
                "version": "6.19",
                "zone_id": "myt",
                "subnet_id": "",
                "assign_public_ip": false,
                "segment_mirroring_enable": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.2",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                },
                "config": {
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                },
                "config": {
                }
            },
            "master_host_count": 2,
            "segment_in_host": 3,
            "segment_host_count": 4,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Create Greenplum cluster",
            "done": false,
            "id": "worker_task_id1",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.CreateClusterMetadata",
                "cluster_id": "cid1"
            },
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "code": 0,
                "details": [],
                "message": "OK"
            }
        }
        """
        And "worker_task_id1" acquired and finished by worker
        When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "id": "cid1",
            "environment": "PRESTABLE",
            "name": "test",
            "description": "test cluster",
            "labels": {
                "foo": "bar"
            },
            "network_id": "network1",
            "config": {
                "access": {
                    "data_lens": false,
                    "web_sql": false,
                    "data_transfer": false,
                    "serverless": false
                },
                "version": "6.19",
                "zone_id": "myt",
                "subnet_id": "",
                "segment_mirroring_enable": false,
                "assign_public_ip": false,
                "segment_auto_rebalance": null,
                "backup_window_start": {
                    "hours":22,
                    "minutes":15,
                    "seconds":30,
                    "nanos":100
                }
            },
            "health": "ALIVE",
            "status": "RUNNING",
            "master_host_count": "2",
            "segment_in_host": "3",
            "segment_host_count": "4",
            "user_name": "usr1",
            "master_config": {
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                },
                "config": null
            },
            "segment_config": {
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "config": null
            }
        }
        """
    Scenario: Create all params
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "folder_id": "folder1",
            "environment": "PRESTABLE",
            "name": "test",
            "description": "test cluster",
            "labels": {
                "foo": "bar"
            },
            "network_id": "network1",
            "config": {
                "version": "6.19",
                "zone_id": "myt",
                "subnet_id": "",
                "assign_public_ip": false,
                "backup_window_start": {
                    "hours":23,
                    "minutes":20,
                    "seconds":40,
                    "nanos":200
                }
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.2",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "config_spec": {
                "greenplum_config_6_19": {
                    "gp_workfile_compression": true,
                    "gp_workfile_limit_files_per_query": 104,
                    "gp_workfile_limit_per_query": 10800332800,
                    "gp_workfile_limit_per_segment": 106954752,
                    "log_statement": "ALL",
                    "max_connections": 989,
                    "max_prepared_transactions": 500,
                    "max_slot_wal_keep_size": 105906176,
                    "max_statement_mem": 2684354560
                },
                "pool": {
                    "size": 228,
                    "mode": "TRANSACTION",
                    "client_idle_timeout": 666
                }
            },
            "master_host_count": 2,
            "segment_in_host": 3,
            "segment_host_count": 4,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Create Greenplum cluster",
            "done": false,
            "id": "worker_task_id1",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.CreateClusterMetadata",
                "cluster_id": "cid1"
            },
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "code": 0,
                "details": [],
                "message": "OK"
            }
        }
        """
        And "worker_task_id1" acquired and finished by worker
        When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "id": "cid1",
            "environment": "PRESTABLE",
            "name": "test",
            "description": "test cluster",
            "labels": {
                "foo": "bar"
            },
            "network_id": "network1",
            "config": {
                "access": {
                    "data_lens": false,
                    "web_sql": false,
                    "data_transfer": false,
                    "serverless": false
                },
                "version": "6.19",
                "zone_id": "myt",
                "subnet_id": "",
                "assign_public_ip": false,
                "segment_mirroring_enable": true,
                "segment_auto_rebalance": null,
                "backup_window_start": {
                    "hours":23,
                    "minutes":20,
                    "seconds":40,
                    "nanos":200
                }
            },
            "health": "ALIVE",
            "status": "RUNNING",
            "master_host_count": "2",
            "segment_in_host": "3",
            "segment_host_count": "4",
            "user_name": "usr1",
            "master_config": {
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                },
                "config": null
            },
            "segment_config": {
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "config": null
            },
            "cluster_config": {
                "pool": {
                    "default_config": {
                        "client_idle_timeout": "1489",
                        "mode": "SESSION",
                        "size": "1480"
                    },
                    "effective_config": {
                        "client_idle_timeout": "666",
                        "mode": "TRANSACTION",
                        "size": "228"
                    },
                    "user_config": {
                        "size": "228",
                        "client_idle_timeout": "666",
                        "mode": "TRANSACTION"
                    }
                },
                "greenplum_config_set_6_19": {
                    "user_config": {
                        "gp_workfile_compression": true,
                        "gp_workfile_limit_files_per_query": "104",
                        "gp_workfile_limit_per_query": "10800332800",
                        "gp_workfile_limit_per_segment": "106954752",
                        "log_statement": "ALL",
                        "max_connections": "989",
                        "max_prepared_transactions": "500",
                        "max_slot_wal_keep_size": "105906176",
                        "max_statement_mem": "2684354560"
                        },
                    "effective_config": {
                        "gp_workfile_compression": true,
                        "gp_workfile_limit_files_per_query": "104",
                        "gp_workfile_limit_per_query": "10800332800",
                        "gp_workfile_limit_per_segment": "106954752",
                        "log_statement": "ALL",
                        "max_connections": "989",
                        "max_prepared_transactions": "500",
                        "max_slot_wal_keep_size": "105906176",
                        "max_statement_mem": "2684354560"
                        },
                    "default_config": {
                        "gp_workfile_compression": null,
                        "gp_workfile_limit_files_per_query": null,
                        "gp_workfile_limit_per_query": null,
                        "gp_workfile_limit_per_segment": null,
                        "log_statement": "LOG_STATEMENT_UNSPECIFIED",
                        "max_connections": "250",
                        "max_prepared_transactions": null,
                        "max_slot_wal_keep_size": null,
                        "max_statement_mem": null
                        }
                }
            }
        }
        """

    Scenario Outline: Create greenplum fails on config
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "folder_id": "folder1",
            "environment": "PRESTABLE",
            "name": "test",
            "description": "test cluster",
            "labels": {
                "foo": "bar"
            },
            "network_id": "network1",
            "config": {
                "zone_id": "myt",
                "subnet_id": "",
                "assign_public_ip": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.2",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "config_spec": {
                "greenplum_config_6_19": {
                    "<param>": <value>
                }
            },
            "master_host_count": 2,
            "segment_in_host": 3,
            "segment_host_count": 4,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "<message>"
        Examples:
            | param                       | value         | message                                                                       |
            | max_connections             | 228           | invalid max_connections value 228: must be 250 or greater                     |
            | max_connections             | 14800         | invalid max_connections value 14800: must be 1000 or lower                   |
            | max_slot_wal_keep_size      | 148000        | invalid max_slot_wal_keep_size 148000: must be multiple by 1048576 (1MB)      |
            | gp_workfile_limit_per_query | -1489         | invalid gp_workfile_limit_per_query value -1489: must be 0 or greater         |
            | max_statement_mem           | 12345         | invalid max_statement_mem value 12345: must be 134217728 or greater           |
            | max_statement_mem           | 134217729     | invalid max_statement_mem value 134217729: must be multiple by 1048576 (1MB)  |
            | max_statement_mem           | 2199023255552 | invalid max_statement_mem value 2199023255552: must be 1099511627776 or lower |
            | log_statement               | 100           | Invalid enum value 100 for LogStatement                                       |
