@greenplum
@grpc_api
Feature: Greenplum_modify
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
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            "description": "another test cluster",
            "deletion_protection": true,
            "labels": {
                "label1": "value1"
            },
            "config": {
                "access": {
                    "data_lens": true,
                    "web_sql": true,
                    "data_transfer": true,
                    "serverless": true
                },
                "backup_window_start": {
                    "hours":23,
                    "minutes":20,
                    "seconds":40,
                    "nanos":200
                }
            },
            "update_mask": {
                "paths": [
                    "description",
                    "labels",
                    "deletion_protection",
                    "config.backup_window_start",
                    "config.access.data_lens",
                    "config.access.web_sql",
                    "config.access.data_transfer",
                    "config.access.serverless"
                ]
            }
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Modify the Greenplum cluster",
            "done": false,
            "id": "worker_task_id2",
            "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.UpdateClusterMetadata",
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
        And "worker_task_id2" acquired and finished by worker
        When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.OperationService" with data
        """
          {
            "operation_id": "worker_task_id2"
          }
        """
        Then we get gRPC response with body
        """
          {
            "created_by": "user",
            "description": "Modify the Greenplum cluster",
            "done": true,
            "id": "worker_task_id2",
            "metadata": {
              "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.UpdateClusterMetadata",
              "cluster_id": "cid1"
            },
            "response": {
              "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.Cluster",
              "id": "cid1",
              "folder_id": "folder1",
              "description": "another test cluster",
              "deletion_protection": true,
              "labels": {
                "label1": "value1"
              },
              "config": "**IGNORE**",
              "created_at": "**IGNORE**",
              "environment": "**IGNORE**",
              "name": "**IGNORE**",
              "description": "**IGNORE**",
              "network_id": "**IGNORE**",
              "health": "ALIVE",
              "status": "RUNNING",
              "master_host_count": "**IGNORE**",
              "segment_in_host": "**IGNORE**",
              "segment_host_count": "**IGNORE**",
              "user_name": "**IGNORE**",
              "master_config": "**IGNORE**",
              "segment_config": "**IGNORE**",
              "cluster_config": "**IGNORE**",
              "host_group_ids": "**IGNORE**",
              "security_group_ids": "**IGNORE**",
              "placement_group_id": "**IGNORE**",
              "planned_operation": "**IGNORE**",
              "monitoring": "**IGNORE**",
              "maintenance_window": "**IGNORE**"
            }
          }
        """
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
            "description": "another test cluster",
            "labels": {
                "label1": "value1"
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

    Scenario: Change params
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
                "assign_public_ip": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.2",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                },
                "config": {
                    "log_level": "WARNING",
                    "max_connections": "201",
                    "timezone": "AR"
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
        When we GET "/api/v1.0/config/myt-1.df.cloud.yandex.net"
        Then we get response with status 200
        And body at path "$.data.greenplum.users.usr1.password.data" contains only
        """
        ["Pa$$w0rd"]
        """
        When we run query
        """
        UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,gpsync}', '{"enable": true}'::jsonb)
        WHERE cid = 'cid1'
        """
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            "description": "another test cluster",
            "deletion_protection": true,
            "user_password": "AnotherPa$$w0rd",
            "labels": {
                "label1": "value1"
            },
            "config": {
                "access": {
                    "data_lens": true,
                    "web_sql": true
                },
                "backup_window_start": {
                    "hours":23,
                    "minutes":20,
                    "seconds":40,
                    "nanos":200
                }
            },
            "config_spec": {
                    "greenplum_config_6_19": {
                        "gp_workfile_compression": true,
                        "gp_workfile_limit_files_per_query": 15000,
                        "gp_workfile_limit_per_query": 10737418240,
                        "gp_workfile_limit_per_segment": 10737418240,
                        "log_statement": "ALL",
                        "max_connections": 300,
                        "max_prepared_transactions": 400,
                        "max_slot_wal_keep_size": 10737418240,
                        "max_statement_mem": "2684354560"
                    },
                    "pool": {
                        "mode":"TRANSACTION",
                        "size": 241,
                        "client_idle_timeout": 242
                    }
            },
            "update_mask": {
                "paths": [
                    "description",
                    "labels",
                    "deletion_protection",
                    "user_password",
                    "config.backup_window_start",
                    "config.access.data_lens",

                    "config_spec.pool.mode",
                    "config_spec.pool.size",
                    "config_spec.pool.client_idle_timeout",

                    "config_spec.greenplum_config_6_19.gp_workfile_compression",
                    "config_spec.greenplum_config_6_19.gp_workfile_limit_files_per_query",
                    "config_spec.greenplum_config_6_19.gp_workfile_limit_per_query",
                    "config_spec.greenplum_config_6_19.gp_workfile_limit_per_segment",
                    "config_spec.greenplum_config_6_19.log_statement",
                    "config_spec.greenplum_config_6_19.max_connections",
                    "config_spec.greenplum_config_6_19.max_prepared_transactions",
                    "config_spec.greenplum_config_6_19.max_slot_wal_keep_size",
                    "config_spec.greenplum_config_6_19.max_statement_mem"
                ]
            }
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Modify the Greenplum cluster",
            "done": false,
            "id": "worker_task_id2",
            "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.UpdateClusterMetadata",
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
        And in worker_queue exists "worker_task_id2" id with args "sync-passwords" set to "true"
        And "worker_task_id2" acquired and finished by worker
        When we GET "/api/v1.0/config/myt-1.df.cloud.yandex.net"
        Then we get response with status 200
        And body at path "$.data.greenplum.users.usr1.password.data" contains only
        """
        ["AnotherPa$$w0rd"]
        """
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
            "description": "another test cluster",
            "labels": {
                "label1": "value1"
            },
            "network_id": "network1",
            "config": {
                "access": {
                    "data_lens": true,
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
                        "client_idle_timeout": "242",
                        "mode": "TRANSACTION",
                        "size": "241"
                    },
                    "user_config": {
                        "size": "241",
                        "client_idle_timeout": "242",
                        "mode": "TRANSACTION"
                    }
                },
                "greenplum_config_set_6_19": {
                    "user_config": {
                        "gp_workfile_compression": true,
                        "gp_workfile_limit_files_per_query": "15000",
                        "gp_workfile_limit_per_query": "10737418240",
                        "gp_workfile_limit_per_segment": "10737418240",
                        "log_statement": "ALL",
                        "max_connections": "300",
                        "max_prepared_transactions": "400",
                        "max_slot_wal_keep_size": "10737418240",
                        "max_statement_mem": "2684354560"
                        },
                    "effective_config": {
                        "gp_workfile_compression": true,
                        "gp_workfile_limit_files_per_query": "15000",
                        "gp_workfile_limit_per_query": "10737418240",
                        "gp_workfile_limit_per_segment": "10737418240",
                        "log_statement": "ALL",
                        "max_connections": "300",
                        "max_prepared_transactions": "400",
                        "max_slot_wal_keep_size": "10737418240",
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

  Scenario: Update cluster name, description and labels
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "update_mask": {
            "paths": ["name", "description", "labels"]
        },
        "name" : "another cluster name",
        "description": "another description",
        "labels": {
            "key1": "val1",
            "key2": "val2"
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Modify the Greenplum cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.UpdateClusterMetadata",
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
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "description": "another description",
        "name": "another cluster name",
        "labels": {
            "key1": "val1",
            "key2": "val2"
        }
    }
    """

    Scenario Outline: Update cluster settings detects unchanged setting value
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
                    "<setting>": <create_value>
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
        And "worker_task_id1" acquired and finished by worker
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            "update_mask": {
                "paths": [
                    "config_spec.greenplum_config_6_19.<setting>"
                ]
            },
            "configSpec": {
                "greenplumConfig_6_19": {
                    "<setting>": <create_value>
                }
            }
        }
        """
        Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            "update_mask": {
                "paths": [
                    "config_spec.greenplum_config_6_19.<setting>"
                ]
            },
            "configSpec": {
                "greenplumConfig_6_19": {
                    "<setting>": <modify_value>
                }
            }
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Modify the Greenplum cluster",
            "done": false,
            "id": "worker_task_id2",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.UpdateClusterMetadata",
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
        And "worker_task_id2" acquired and finished by worker
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            "update_mask": {
                "paths": [
                    "config_spec.greenplum_config_6_19.<setting>"
                ]
            },
            "configSpec": {
                "greenplumConfig_6_19": {
                    "<setting>": <modify_value>
                }
            }
        }
        """
        Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"
        Examples:
            | setting           | create_value | modify_value |
            | log_statement     | "ALL"        | "DDL"        |
            | max_statement_mem | 2097152000   | 2684354560   |
