@greenplum
@grpc_api
Feature: Greenplum_modify_flavor
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

    Scenario: Change flavor for segment
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
                        "max_connections": 989
                    },
                    "pool": {
                        "mode":"TRANSACTION",
                        "size":241,
                        "client_idle_timeout":242
                    }
            },
            "segment_config": {
                "resources": {
                    "disk_size": "42949672960",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                }
            },
            "update_mask": {
                "paths": [
                    "description",
                    "labels",
                    "deletion_protection",
                    "config.backup_window_start",
                    "config.access.data_lens",

                    "segment_config.resources.resource_preset_id",
                    "segment_config.resources.disk_size",

                    "config_spec.pool.mode",
                    "config_spec.pool.size",
                    "config_spec.pool.client_idle_timeout",
                    "config_spec.greenplum_config_6_19.max_connections"
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
                    "disk_size": "42949672960",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                },
                "config": null
            }
        }
        """

    Scenario: Change flavor for master
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
                },
                "config": {
                    "log_level": "INFORMATION",
                    "max_connections": "301"
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
                    "web_sql": true
                },
                "backup_window_start": {
                    "hours":23,
                    "minutes":20,
                    "seconds":40,
                    "nanos":200
                }
            },
            "master_config": {
                "resources": {
                    "disk_size": "42949672960",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                }
            },
            "segment_config": {
                "resources": {
                    "disk_size": "42949672960",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                }
            },
            "update_mask": {
                "paths": [
                    "description",
                    "labels",
                    "deletion_protection",
                    "config.backup_window_start",
                    "config.access.data_lens",

                    "master_config.resources.resource_preset_id",
                    "master_config.resources.disk_size"
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
                    "disk_size": "42949672960",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
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

    Scenario: Change flavor for 1 master
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
                },
                "config": {
                    "log_level": "INFORMATION",
                    "max_connections": "301"
                }
            },
            "master_host_count": 1,
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
            "master_host_count": "1",
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
                    "web_sql": true
                },
                "backup_window_start": {
                    "hours":23,
                    "minutes":20,
                    "seconds":40,
                    "nanos":200
                }
            },
            "master_config": {
                "resources": {
                    "disk_size": "42949672960",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                }
            },
            "segment_config": {
                "resources": {
                    "disk_size": "42949672960",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                }
            },
            "update_mask": {
                "paths": [
                    "description",
                    "labels",
                    "deletion_protection",
                    "config.backup_window_start",
                    "config.access.data_lens",

                    "master_config.resources.resource_preset_id",
                    "master_config.resources.disk_size"
                ]
            }
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "need 2 master hosts for cluster "cid1""
