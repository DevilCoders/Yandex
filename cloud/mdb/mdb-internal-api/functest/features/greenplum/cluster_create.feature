@greenplum
@grpc_api
Feature: Greenplum-create
    Background:
        Given default headers
        When we add default feature flag "MDB_GREENPLUM_CLUSTER"
    Scenario: Create greenplum no config
        When we add cloud "cloud1"
        And we add folder "folder1" to cloud "cloud1"
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
            "network_id": "network1"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "Greenplum cluster config should be set"
    Scenario: Create greenplum no master config
        When we add cloud "cloud1"
        And we add folder "folder1" to cloud "cloud1"
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
                "access":null,
                "version": "6.19",
                "zone_id": "myt",
                "subnet_id": "",
                "assign_public_ip": false,
                "backup_window_start": {
                    "hours":22,
                    "minutes":15,
                    "seconds":30,
                    "nanos":100
                }
            }
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "Greenplum master config should be set"
    Scenario: Create greenplum no segment config
        When we add cloud "cloud1"
        And we add folder "folder1" to cloud "cloud1"
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
                "access":null,
                "version": "6.19",
                "zone_id": "myt",
                "subnet_id": "",
                "assign_public_ip": false,
                "backup_window_start": {
                    "hours":22,
                    "minutes":15,
                    "seconds":30,
                    "nanos":100
                }
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.2",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            }
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "Greenplum segment config should be set"
    Scenario: Create greenplum no segment host count
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
            "master_host_count": 2,
            "segment_in_host": 3,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "segment host count must be 2 or more"
    Scenario: Create greenplum no master host count
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
            "segment_host_count": 4,
            "segment_in_host": 3,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "master host count must be 1 or 2"
    Scenario: Create greenplum no master host count local-ssd
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
                    "diskTypeId": "local-ssd",
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
            "master_host_count": 1,
            "segment_host_count": 4,
            "segment_in_host": 1,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "master host count must be 2 for local-ssd disk"
    Scenario: Create greenplum no segments count
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
            "master_host_count": 2,
            "segment_host_count": 4,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "segment in host must be 1 or more"
    Scenario: Create greenplum large segments count
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
            "master_host_count": 2,
            "segment_in_host": 3,
            "segment_host_count": 4,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "the segment in host must be no more then ram/8gb: 1; RAM: 4294967296"

    Scenario: Create greenplum bad user name
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
            "master_host_count": 2,
            "segment_in_host": 1,
            "segment_host_count": 4,
            "user_name": "usr 1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "user name "usr 1" has invalid symbols"

    Scenario: Create greenplum on dedicated host different disk size in host group
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
            "host_group_ids": ["hg4", "hg5"],
            "config": {
                "zone_id": "vla",
                "subnet_id": "",
                "assign_public_ip": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s2.compute.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 14123730
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s2.compute.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 141237374
                }
            },
            "master_host_count": 2,
            "segment_in_host": 1,
            "segment_host_count": 2,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "different disk_size in host groups is not allowed"
    Scenario: Create greenplum on dedicated wrong disk size
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
            "host_group_ids": ["hg4"],
            "config": {
                "zone_id": "vla",
                "subnet_id": "",
                "assign_public_ip": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s2.compute.4",
                    "diskTypeId": "local-ssd",
                    "diskSize": 85899345920
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s2.compute.4",
                    "diskTypeId": "local-ssd",
                    "diskSize": 85899345920
                }
            },
            "master_host_count": 2,
            "segment_in_host": 1,
            "segment_host_count": 2,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "cannot use disk size 85899345920 on host group, max disk size is 68719476736"
    Scenario: Create greenplum on dedicated wrong step disk size
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
            "host_group_ids": ["hg4"],
            "config": {
                "zone_id": "vla",
                "subnet_id": "",
                "assign_public_ip": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s2.compute.4",
                    "diskTypeId": "local-ssd",
                    "diskSize": 17179861489
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s2.compute.4",
                    "diskTypeId": "local-ssd",
                    "diskSize": 17179861489
                }
            },
            "master_host_count": 2,
            "segment_in_host": 1,
            "segment_host_count": 2,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "on host group disk size should by multiple of 17179869184"
    Scenario: Create greenplum on dedicated cpu count
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
            "host_group_ids": ["hg6"],
            "config": {
                "zone_id": "vla",
                "subnet_id": "",
                "assign_public_ip": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s3.compute.3",
                    "diskTypeId": "local-ssd",
                    "diskSize": 34359738368
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s3.compute.3",
                    "diskTypeId": "local-ssd",
                    "diskSize": 34359738368
                }
            },
            "master_host_count": 2,
            "segment_in_host": 1,
            "segment_host_count": 2,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "cannot use resource preset "s3.compute.3" on dedicated host, max cpu limit is 1"
    Scenario: Create greenplum on dedicated memory
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
            "host_group_ids": ["hg7"],
            "config": {
                "zone_id": "vla",
                "subnet_id": "",
                "assign_public_ip": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s3.compute.3",
                    "diskTypeId": "local-ssd",
                    "diskSize": 34359738368
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s3.compute.3",
                    "diskTypeId": "local-ssd",
                    "diskSize": 34359738368
                }
            },
            "master_host_count": 2,
            "segment_in_host": 1,
            "segment_host_count": 2,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "cannot use resource preset "s3.compute.3" on dedicated host, max memory is 137438"

    Scenario: Create greenplum on dedicated wrong master flavor
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
            "host_group_ids": ["hg4"],
            "config": {
                "zone_id": "vla",
                "subnet_id": "",
                "assign_public_ip": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s3.compute.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 17179869184
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s2.compute.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 17179869184
                }
            },
            "master_host_count": 2,
            "segment_in_host": 1,
            "segment_host_count": 2,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "resource preset "s3.compute.1" and dedicated hosts group "hg4" have different platforms"

    Scenario: Create greenplum on dedicated with different groups platforms
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
            "host_group_ids": ["hg4", "hg6"],
            "config": {
                "zone_id": "vla",
                "subnet_id": "",
                "assign_public_ip": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s2.compute.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 17179869184
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s2.compute.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 17179869184
                }
            },
            "master_host_count": 2,
            "segment_in_host": 1,
            "segment_host_count": 2,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "resource preset "s2.compute.1" and dedicated hosts group "hg6" have different platforms"

    Scenario: Create greenplum on dedicated host
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
            "host_group_ids": ["hg4"],
            "config": {
                "zone_id": "vla",
                "subnet_id": "",
                "assign_public_ip": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s2.compute.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 17179869184
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s2.compute.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 17179869184
                }
            },
            "master_host_count": 2,
            "segment_in_host": 1,
            "segment_host_count": 2,
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

    Scenario: Create greenplum
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
            "master_host_count": 2,
            "segment_in_host": 1,
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
    Scenario: Delete greenplum
        When we add default feature flag "MDB_GREENPLUM_ALLOW_LOW_MEM"
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
        When we "Delete" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Delete Greenplum cluster",
            "done": false,
            "id": "worker_task_id2",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.DeleteClusterMetadata",
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
    Scenario: Create greenplum with NRD
        When we add default feature flag "MDB_ALLOW_NETWORK_SSD_NONREPLICATED"
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
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd-nonreplicated",
                    "diskSize": 10737418240
                }
            },
            "master_host_count": 2,
            "segment_in_host": 1,
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
