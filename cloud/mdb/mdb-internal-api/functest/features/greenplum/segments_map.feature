@greenplum
@grpc_api
Feature: Greenplum create clusters and check segments pillar

  Background:
    Given default headers
    When we add default feature flag "MDB_GREENPLUM_CLUSTER"
    When we add default feature flag "MDB_GREENPLUM_ALLOW_LOW_MEM"

  Scenario: Create better cluster
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "folder_id": "folder3",
        "environment": "PRESTABLE",
        "name": "dbaas_e2e_func_tests",
        "network_id": "network3",
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
        "segment_in_host": 4,
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
    When we GET "/api/v1.0/config/myt-1.df.cloud.yandex.net"
    Then we get response with status 200
    And body at path "$.data.greenplum.segments" contains
    """
    {
        "-1": {
            "primary": {"fqdn": "myt-1.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-2.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "0": {
            "primary": {"fqdn": "myt-3.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-5.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "1": {
            "primary": {"fqdn": "myt-3.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-5.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "2": {
            "primary": {"fqdn": "myt-3.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-5.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "3": {
            "primary": {"fqdn": "myt-3.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-5.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "4": {
            "primary": {"fqdn": "myt-5.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-3.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "5": {
            "primary": {"fqdn": "myt-5.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-3.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "6": {
            "primary": {"fqdn": "myt-5.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-3.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "7": {
            "primary": {"fqdn": "myt-5.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-3.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "8": {
            "primary": {"fqdn": "myt-4.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-6.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "9": {
            "primary": {"fqdn": "myt-4.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-6.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "10": {
            "primary": {"fqdn": "myt-4.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-6.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "11": {
            "primary": {"fqdn": "myt-4.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-6.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "12": {
            "primary": {"fqdn": "myt-6.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-4.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "13": {
            "primary": {"fqdn": "myt-6.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-4.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "14": {
            "primary": {"fqdn": "myt-6.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-4.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        },
        "15": {
            "primary": {"fqdn": "myt-6.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"},
            "mirror": {"fqdn": "myt-4.df.cloud.yandex.net", "mount_point": "/var/lib/greenplum/data1"}
        }
    }
    """
