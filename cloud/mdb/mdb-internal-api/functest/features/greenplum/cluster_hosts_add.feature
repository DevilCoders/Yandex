@greenplum
@grpc_api
Feature: Greenplum-hosts-add
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
                            "name": "greenplum_master",
                            "role": "Master",
                            "status": "Alive",
                            "timestamp": 1600860350
                        },
                        {
                            "name": "abc",
                            "role": "Master",
                            "status": "Alive",
                            "timestamp": 1600860351
                        }
                    ]
                },
                {
                    "fqdn": "myt-5.df.cloud.yandex.net",
                    "cid": "cid1",
                    "status": "Alive",
                    "services": [
                        {
                            "name": "greenplum_segments",
                            "role": "Replica",
                            "status": "Alive",
                            "timestamp": 1600860350
                        },
                        {
                            "name": "abc",
                            "role": "Master",
                            "status": "Alive",
                            "timestamp": 1600860351
                        }
                    ]
                }
            ]
        }
        """
        When we add default feature flag "MDB_GREENPLUM_CLUSTER"
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
    Scenario: Host master list works
        When we "ListMasterHosts" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "hosts": [
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "ALIVE",
                    "name": "myt-1.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.2"
                    },
                    "type": "MASTER",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                }
            ]
        }
        """
    Scenario: Host segment list works
        When we "ListSegmentHosts" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "hosts": [
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "UNKNOWN",
                    "name": "myt-2.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    },
                    "type": "TYPE_UNSPECIFIED",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                },
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "UNKNOWN",
                    "name": "myt-3.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    },
                    "type": "TYPE_UNSPECIFIED",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                },
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "UNKNOWN",
                    "name": "myt-4.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    },
                    "type": "TYPE_UNSPECIFIED",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                },
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "ALIVE",
                    "name": "myt-5.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    },
                    "type": "SEGMENT",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                }
            ]
        }
        """
    Scenario: Hosts add
        When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            "segment_host_count": 3,
            "master_host_count": 1
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Add host to Greenplum cluster",
            "done": false,
            "id": "worker_task_id2",
            "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.AddClusterHostsMetadata",
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
        When we "ListSegmentHosts" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "hosts": [
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "UNKNOWN",
                    "name": "myt-2.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    },
                    "type": "TYPE_UNSPECIFIED",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                },
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "UNKNOWN",
                    "name": "myt-3.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    },
                    "type": "TYPE_UNSPECIFIED",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                },
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "UNKNOWN",
                    "name": "myt-4.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    },
                    "type": "TYPE_UNSPECIFIED",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                },
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "ALIVE",
                    "name": "myt-5.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    },
                    "type": "SEGMENT",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                },
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "UNKNOWN",
                    "name": "myt-7.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    },
                    "type": "TYPE_UNSPECIFIED",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                },
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "UNKNOWN",
                    "name": "myt-8.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    },
                    "type": "TYPE_UNSPECIFIED",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                },
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "UNKNOWN",
                    "name": "myt-9.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    },
                    "type": "TYPE_UNSPECIFIED",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                }
            ]
        }
        """
        When we "ListMasterHosts" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "hosts": [
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "ALIVE",
                    "name": "myt-1.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.2"
                    },
                    "type": "MASTER",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                },
                {
                    "assign_public_ip": false,
                    "cluster_id": "cid1",
                    "health": "UNKNOWN",
                    "name": "myt-6.df.cloud.yandex.net",
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.2"
                    },
                    "type": "TYPE_UNSPECIFIED",
                    "subnet_id": "network1-myt",
                    "zone_id": "myt",
                    "system": null
                }
            ]
        }
        """
