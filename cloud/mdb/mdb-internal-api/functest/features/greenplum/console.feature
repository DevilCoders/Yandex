@greenplum
@grpc_api
Feature: Greenplum console API
  Background:
    Given default headers

  Scenario: Cluster config handler works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService"
    Then we get gRPC response with body
    """
    {
      "cluster_name": {
        "blacklist": [],
        "regexp": "^[a-zA-Z0-9_-]+$",
        "min": "1",
        "max": "63"
      },
      "versions": [
        "6.19"
      ]
    }
    """

  Scenario: Cluster config handler works with feature flags
    When we add cloud "cloud1"
    And we add folder "folder1" to cloud "cloud1"
    And we shrink valid resources in metadb for flavor "s2.compute.2"
    And we shrink valid resources in metadb for disk "network-ssd"
    And we shrink valid resources in metadb for geo "vla,sas"
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
        """
        {
        "folder_id": "folder1"
        }
        """
    Then we get gRPC response with body
        """
        {
        "host_types": [
                    {
                        "default_resources": {
                                "disk_size": "1024",
                                "disk_type_id": "test_disk_type_id",
                                "generation": "1",
                                "generation_name": "Haswell",
                                "resource_preset_id": "test_preset_id"
                        },
                        "resource_presets": [
                                {
                                        "cpu_fraction": "100",
                                        "cpu_limit": "2",
                                        "decomissioning": false,
                                        "generation": "2",
                                        "generation_name": "Broadwell",
                                        "memory_limit": "8589934592",
                                        "preset_id": "s2.compute.2",
                                        "type": "standard",
                                        "zones": [
                                                {
                                                        "disk_types": [
                                                                {
                                                                        "disk_size_range": {
                                                                                "max": "2199023255552",
                                                                                "min": "10737418240"
                                                                        },
                                                                        "disk_type_id": "network-ssd",
                                                                        "max_hosts": "2",
                                                                        "min_hosts": "1",
                                                                        "host_count_divider":"1",
                                                                        "max_segment_in_host_count":"0"
                                                                }
                                                        ],
                                                        "zone_id": "sas"
                                                },
                                                {
                                                        "disk_types": [
                                                                {
                                                                        "disk_size_range": {
                                                                                "max": "2199023255552",
                                                                                "min": "10737418240"
                                                                        },
                                                                        "disk_type_id": "network-ssd",
                                                                        "max_hosts": "2",
                                                                        "min_hosts": "1",
                                                                        "host_count_divider":"1",
                                                                        "max_segment_in_host_count":"0"
                                                                }
                                                        ],
                                                        "zone_id": "vla"
                                                }
                                        ]
                                }
                        ],
                        "type": "MASTER"
                },
                {
                        "default_resources": {
                                "disk_size": "1024",
                                "disk_type_id": "test_disk_type_id",
                                "generation": "1",
                                "generation_name": "Haswell",
                                "resource_preset_id": "test_preset_id"
                        },
                        "resource_presets": [
                                {
                                        "cpu_fraction": "100",
                                        "cpu_limit": "2",
                                        "decomissioning": false,
                                        "generation": "2",
                                        "generation_name": "Broadwell",
                                        "memory_limit": "8589934592",
                                        "preset_id": "s2.compute.2",
                                        "type": "standard",
                                        "zones": [
                                                {
                                                        "disk_types": [
                                                                {
                                                                        "disk_size_range": {
                                                                                "max": "2199023255552",
                                                                                "min": "10737418240"
                                                                        },
                                                                        "disk_type_id": "network-ssd",
                                                                        "max_hosts": "8",
                                                                        "min_hosts": "2",
                                                                        "host_count_divider":"2",
                                                                        "max_segment_in_host_count":"1"
                                                                }
                                                        ],
                                                        "zone_id": "sas"
                                                },
                                                {
                                                        "disk_types": [
                                                                {
                                                                        "disk_size_range": {
                                                                                "max": "2199023255552",
                                                                                "min": "10737418240"
                                                                        },
                                                                        "disk_type_id": "network-ssd",
                                                                        "max_hosts": "8",
                                                                        "min_hosts": "2",
                                                                        "host_count_divider":"2",
                                                                        "max_segment_in_host_count":"1"
                                                                }
                                                        ],
                                                        "zone_id": "vla"
                                                }
                                        ]
                                }
                        ],
                        "type": "SEGMENT"
                }
        ]
        }
        """
    When we add default feature flag "MDB_GREENPLUM_ALLOW_LOW_MEM"
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
        """
        {
        "folder_id": "folder1"
        }
        """
    Then we get gRPC response with body
        """
        {
        "host_types": [
                    {
                        "default_resources": {
                                "disk_size": "1024",
                                "disk_type_id": "test_disk_type_id",
                                "generation": "1",
                                "generation_name": "Haswell",
                                "resource_preset_id": "test_preset_id"
                        },
                        "resource_presets": [
                                {
                                        "cpu_fraction": "100",
                                        "cpu_limit": "2",
                                        "decomissioning": false,
                                        "generation": "2",
                                        "generation_name": "Broadwell",
                                        "memory_limit": "8589934592",
                                        "preset_id": "s2.compute.2",
                                        "type": "standard",
                                        "zones": [
                                                {
                                                        "disk_types": [
                                                                {
                                                                        "disk_size_range": {
                                                                                "max": "2199023255552",
                                                                                "min": "10737418240"
                                                                        },
                                                                        "disk_type_id": "network-ssd",
                                                                        "max_hosts": "2",
                                                                        "min_hosts": "1",
                                                                        "host_count_divider":"1",
                                                                        "max_segment_in_host_count":"0"
                                                                }
                                                        ],
                                                        "zone_id": "sas"
                                                },
                                                {
                                                        "disk_types": [
                                                                {
                                                                        "disk_size_range": {
                                                                                "max": "2199023255552",
                                                                                "min": "10737418240"
                                                                        },
                                                                        "disk_type_id": "network-ssd",
                                                                        "max_hosts": "2",
                                                                        "min_hosts": "1",
                                                                        "host_count_divider":"1",
                                                                        "max_segment_in_host_count":"0"
                                                                }
                                                        ],
                                                        "zone_id": "vla"
                                                }
                                        ]
                                }
                        ],
                        "type": "MASTER"
                },
                {
                        "default_resources": {
                                "disk_size": "1024",
                                "disk_type_id": "test_disk_type_id",
                                "generation": "1",
                                "generation_name": "Haswell",
                                "resource_preset_id": "test_preset_id"
                        },
                        "resource_presets": [
                                {
                                        "cpu_fraction": "100",
                                        "cpu_limit": "2",
                                        "decomissioning": false,
                                        "generation": "2",
                                        "generation_name": "Broadwell",
                                        "memory_limit": "8589934592",
                                        "preset_id": "s2.compute.2",
                                        "type": "standard",
                                        "zones": [
                                                {
                                                        "disk_types": [
                                                                {
                                                                        "disk_size_range": {
                                                                                "max": "2199023255552",
                                                                                "min": "10737418240"
                                                                        },
                                                                        "disk_type_id": "network-ssd",
                                                                        "max_hosts": "8",
                                                                        "min_hosts": "2",
                                                                        "host_count_divider":"2",
                                                                        "max_segment_in_host_count":"10"
                                                                }
                                                        ],
                                                        "zone_id": "sas"
                                                },
                                                {
                                                        "disk_types": [
                                                                {
                                                                        "disk_size_range": {
                                                                                "max": "2199023255552",
                                                                                "min": "10737418240"
                                                                        },
                                                                        "disk_type_id": "network-ssd",
                                                                        "max_hosts": "8",
                                                                        "min_hosts": "2",
                                                                        "host_count_divider":"2",
                                                                        "max_segment_in_host_count":"10"
                                                                }
                                                        ],
                                                        "zone_id": "vla"
                                                }
                                        ]
                                }
                        ],
                        "type": "SEGMENT"
                }
        ]
        }
        """
    And we restore valid resources after shrinking

  Scenario: Billing cost estimation works
    When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
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
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.2",
                    "platform_id": "mdb-v1",
                    "cores": "2",
                    "core_fraction": "100",
                    "memory": "8589934592",
                    "roles": ["greenplum_cluster.master_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.2",
                    "platform_id": "mdb-v1",
                    "cores": "2",
                    "core_fraction": "100",
                    "memory": "8589934592",
                    "roles": ["greenplum_cluster.master_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            }
        ]
    }
    """
# TODO: test resource presets output

  Scenario: Billing cost with deprecated version estimation works
    When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
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
        "version": "6.17",
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
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.2",
                    "platform_id": "mdb-v1",
                    "cores": "2",
                    "core_fraction": "100",
                    "memory": "8589934592",
                    "roles": ["greenplum_cluster.master_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.2",
                    "platform_id": "mdb-v1",
                    "cores": "2",
                    "core_fraction": "100",
                    "memory": "8589934592",
                    "roles": ["greenplum_cluster.master_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            }
        ]
    }
    """

  Scenario: Cluster create config handler works
    When we "GetCreateConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService"
    Then we get gRPC response with body
    """
    {
    "schema": "{\"Access\":{\"properties\":{\"dataLens\":{\"format\":\"boolean\",\"type\":\"boolean\"},\"dataTransfer\":{\"format\":\"boolean\",\"type\":\"boolean\"},\"serverless\":{\"format\":\"boolean\",\"type\":\"boolean\"},\"webSql\":{\"format\":\"boolean\",\"type\":\"boolean\"}},\"type\":\"object\"},\"ClusterEnvironment\":{\"enum\":[\"PRODUCTION\",\"PRESTABLE\"],\"type\":\"string\"},\"ConfigSpec\":{\"properties\":{\"config\":{\"$ref\":\"#/GreenplumClusterConfigSpecSchemaV1\"},\"pool\":{\"$ref\":\"#/PoolConfigSpecSchemaV1\"}},\"type\":\"object\"},\"CreateClusterRequest\":{\"properties\":{\"config\":{\"$ref\":\"#/GreenplumConfig\"},\"configSpec\":{\"$ref\":\"#/ConfigSpec\"},\"description\":{\"type\":\"string\"},\"environment\":{\"$ref\":\"#/ClusterEnvironment\"},\"folderId\":{\"type\":\"string\"},\"labels\":{\"additionalProperties\":{\"type\":\"string\"},\"type\":\"object\"},\"masterConfig\":{\"$ref\":\"#/MasterSubclusterConfigSpec\"},\"masterHostCount\":{\"format\":\"int64\",\"type\":\"string\"},\"name\":{\"type\":\"string\"},\"networkId\":{\"type\":\"string\"},\"securityGroupIds\":{\"items\":{\"type\":\"string\"},\"type\":\"array\"},\"segmentConfig\":{\"$ref\":\"#/SegmentSubclusterConfigSpec\"},\"segmentHostCount\":{\"format\":\"int64\",\"type\":\"string\"},\"segmentInHost\":{\"format\":\"int64\",\"type\":\"string\"},\"userName\":{\"type\":\"string\"},\"userPassword\":{\"type\":\"string\"}},\"type\":\"object\"},\"Greenplum617Config\":{\"properties\":{\"gpWorkfileCompression\":{\"default\":false,\"type\":\"boolean\"},\"gpWorkfileLimitFilesPerQuery\":{\"format\":\"int32\",\"type\":\"integer\"},\"gpWorkfileLimitPerQuery\":{\"format\":\"int32\",\"type\":\"integer\"},\"gpWorkfileLimitPerSegment\":{\"format\":\"int32\",\"type\":\"integer\"},\"logStatement\":{\"enum\":[\"none\",\"ddl\",\"mod\",\"all\"],\"type\":\"string\"},\"maxConnections\":{\"format\":\"int32\",\"type\":\"integer\"},\"maxPreparedTransactions\":{\"format\":\"int32\",\"type\":\"integer\"},\"maxSlotWalKeepSize\":{\"format\":\"int32\",\"type\":\"integer\"},\"maxStatementMem\":{\"format\":\"int32\",\"type\":\"integer\"}},\"type\":\"object\"},\"Greenplum619Config\":{\"properties\":{\"gpWorkfileCompression\":{\"default\":false,\"type\":\"boolean\"},\"gpWorkfileLimitFilesPerQuery\":{\"format\":\"int32\",\"type\":\"integer\"},\"gpWorkfileLimitPerQuery\":{\"format\":\"int32\",\"type\":\"integer\"},\"gpWorkfileLimitPerSegment\":{\"format\":\"int32\",\"type\":\"integer\"},\"logStatement\":{\"enum\":[\"none\",\"ddl\",\"mod\",\"all\"],\"type\":\"string\"},\"maxConnections\":{\"format\":\"int32\",\"type\":\"integer\"},\"maxPreparedTransactions\":{\"format\":\"int32\",\"type\":\"integer\"},\"maxSlotWalKeepSize\":{\"format\":\"int32\",\"type\":\"integer\"},\"maxStatementMem\":{\"format\":\"int32\",\"type\":\"integer\"}},\"type\":\"object\"},\"GreenplumClusterConfigSpecSchemaV1\":{\"greenplumConfig_6_17\":{\"$ref\":\"#/Greenplum617Config\"},\"greenplumConfig_6_19\":{\"$ref\":\"#/Greenplum619Config\"},\"type\":\"object\"},\"GreenplumConfig\":{\"properties\":{\"access\":{\"$ref\":\"#/Access\"},\"assignPublicIp\":{\"format\":\"boolean\",\"type\":\"boolean\"},\"backupWindowStart\":{\"$ref\":\"#/TimeOfDay\"},\"subnetId\":{\"type\":\"string\"},\"version\":{\"type\":\"string\"},\"zoneId\":{\"type\":\"string\"}},\"type\":\"object\"},\"MasterSubclusterConfigSpec\":{\"properties\":{\"resources\":{\"$ref\":\"#/Resources\"}},\"type\":\"object\"},\"PoolConfigSpecSchemaV1\":{\"properties\":{\"clientIdleTimeout\":{\"format\":\"int32\",\"type\":\"integer\"},\"mode\":{\"enum\":[\"SESSION\",\"TRANSACTION\"],\"type\":\"string\"},\"size\":{\"format\":\"int32\",\"type\":\"integer\"}},\"type\":\"object\"},\"Resources\":{\"properties\":{\"diskSize\":{\"format\":\"int64\",\"type\":\"string\"},\"diskTypeId\":{\"type\":\"string\"},\"resourcePresetId\":{\"type\":\"string\"}},\"type\":\"object\"},\"SegmentSubclusterConfigSpec\":{\"properties\":{\"resources\":{\"$ref\":\"#/Resources\"}},\"type\":\"object\"},\"TimeOfDay\":{\"properties\":{\"hours\":{\"format\":\"int32\",\"type\":\"integer\"},\"minutes\":{\"format\":\"int32\",\"type\":\"integer\"},\"nanos\":{\"format\":\"int32\",\"type\":\"integer\"},\"seconds\":{\"format\":\"int32\",\"type\":\"integer\"}},\"type\":\"object\"}}"
    }
    """

   Scenario: Cluster config handler works with host group
    When we add cloud "cloud1"
    And we add folder "folder1" to cloud "cloud1"
    And we shrink valid resources in metadb for flavor "s2.compute.1,s2.compute.2,s3.compute.1,s3.compute.2"
    And we shrink valid resources in metadb for disk "local-ssd"
    And we shrink valid resources in metadb for geo "vla"
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
    """
    {
    "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
    "host_types": [
                {
                    "default_resources": {
                            "disk_size": "1024",
                            "disk_type_id": "test_disk_type_id",
                            "generation": "1",
                            "generation_name": "Haswell",
                            "resource_preset_id": "test_preset_id"
                    },
                    "resource_presets": [
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "1",
                                    "decomissioning": false,
                                    "generation": "2",
                                    "generation_name": "Broadwell",
                                    "memory_limit": "4294967296",
                                    "preset_id": "s2.compute.1",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_size_range": {
                                                                            "max": "2199023255552",
                                                                            "min": "10737418240"
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "2",
                                                                    "min_hosts": "1",
                                                                    "host_count_divider":"1",
                                                                    "max_segment_in_host_count":"0"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            },
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "2",
                                    "decomissioning": false,
                                    "generation": "2",
                                    "generation_name": "Broadwell",
                                    "memory_limit": "8589934592",
                                    "preset_id": "s2.compute.2",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_size_range": {
                                                                            "max": "2199023255552",
                                                                            "min": "10737418240"
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "2",
                                                                    "min_hosts": "1",
                                                                    "host_count_divider":"1",
                                                                    "max_segment_in_host_count":"0"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            },
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "1",
                                    "decomissioning": false,
                                    "generation": "3",
                                    "generation_name": "Cascade Lake",
                                    "memory_limit": "4294967296",
                                    "preset_id": "s3.compute.1",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_size_range": {
                                                                            "max": "2199023255552",
                                                                            "min": "10737418240"
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "2",
                                                                    "min_hosts": "1",
                                                                    "host_count_divider":"1",
                                                                    "max_segment_in_host_count":"0"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            },
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "2",
                                    "decomissioning": false,
                                    "generation": "3",
                                    "generation_name": "Cascade Lake",
                                    "memory_limit": "8589934592",
                                    "preset_id": "s3.compute.2",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_size_range": {
                                                                            "max": "2199023255552",
                                                                            "min": "10737418240"
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "2",
                                                                    "min_hosts": "1",
                                                                    "host_count_divider":"1",
                                                                    "max_segment_in_host_count":"0"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            }
                    ],
                    "type": "MASTER"
            },
            {
                    "default_resources": {
                            "disk_size": "1024",
                            "disk_type_id": "test_disk_type_id",
                            "generation": "1",
                            "generation_name": "Haswell",
                            "resource_preset_id": "test_preset_id"
                    },
                    "resource_presets": [
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "1",
                                    "decomissioning": false,
                                    "generation": "2",
                                    "generation_name": "Broadwell",
                                    "memory_limit": "4294967296",
                                    "preset_id": "s2.compute.1",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_size_range": {
                                                                            "max": "2199023255552",
                                                                            "min": "10737418240"
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "8",
                                                                    "min_hosts": "2",
                                                                    "host_count_divider":"2",
                                                                    "max_segment_in_host_count":"1"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            },
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "2",
                                    "decomissioning": false,
                                    "generation": "2",
                                    "generation_name": "Broadwell",
                                    "memory_limit": "8589934592",
                                    "preset_id": "s2.compute.2",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_size_range": {
                                                                            "max": "2199023255552",
                                                                            "min": "10737418240"
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "8",
                                                                    "min_hosts": "2",
                                                                    "host_count_divider":"2",
                                                                    "max_segment_in_host_count":"1"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            },
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "1",
                                    "decomissioning": false,
                                    "generation": "3",
                                    "generation_name": "Cascade Lake",
                                    "memory_limit": "4294967296",
                                    "preset_id": "s3.compute.1",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_size_range": {
                                                                            "max": "2199023255552",
                                                                            "min": "10737418240"
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "8",
                                                                    "min_hosts": "2",
                                                                    "host_count_divider":"2",
                                                                    "max_segment_in_host_count":"1"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            },
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "2",
                                    "decomissioning": false,
                                    "generation": "3",
                                    "generation_name": "Cascade Lake",
                                    "memory_limit": "8589934592",
                                    "preset_id": "s3.compute.2",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_size_range": {
                                                                            "max": "2199023255552",
                                                                            "min": "10737418240"
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "8",
                                                                    "min_hosts": "2",
                                                                    "host_count_divider":"2",
                                                                    "max_segment_in_host_count":"1"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            }
                    ],
                    "type": "SEGMENT"
            }
    ]
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
    """
    {
    "folder_id": "folder1",
    "host_group_ids": "hg6"
    }
    """
    Then we get gRPC response with body
    """
    {
    "host_types": [
                {
                    "default_resources": {
                            "disk_size": "1024",
                            "disk_type_id": "test_disk_type_id",
                            "generation": "1",
                            "generation_name": "Haswell",
                            "resource_preset_id": "test_preset_id"
                    },
                    "resource_presets": [
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "1",
                                    "decomissioning": false,
                                    "generation": "3",
                                    "generation_name": "Cascade Lake",
                                    "memory_limit": "4294967296",
                                    "preset_id": "s3.compute.1",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_sizes": {
                                                                        "sizes": [
                                                                            "34359738368",
                                                                            "68719476736",
                                                                            "103079215104",
                                                                            "137438953472"
                                                                        ]
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "2",
                                                                    "min_hosts": "1",
                                                                    "host_count_divider":"1",
                                                                    "max_segment_in_host_count":"0"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            }
                    ],
                    "type": "MASTER"
            },
            {
                    "default_resources": {
                            "disk_size": "1024",
                            "disk_type_id": "test_disk_type_id",
                            "generation": "1",
                            "generation_name": "Haswell",
                            "resource_preset_id": "test_preset_id"
                    },
                    "resource_presets": [
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "1",
                                    "decomissioning": false,
                                    "generation": "3",
                                    "generation_name": "Cascade Lake",
                                    "memory_limit": "4294967296",
                                    "preset_id": "s3.compute.1",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_sizes": {
                                                                        "sizes": [
                                                                            "34359738368",
                                                                            "68719476736",
                                                                            "103079215104",
                                                                            "137438953472"
                                                                        ]
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "8",
                                                                    "min_hosts": "2",
                                                                    "host_count_divider":"2",
                                                                    "max_segment_in_host_count":"1"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            }
                    ],
                    "type": "SEGMENT"
            }
    ]
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
    """
    {
    "folder_id": "folder1",
    "host_group_ids": "hg5"
    }
    """
    Then we get gRPC response with body
    """
    {
    "host_types": [
                {
                    "default_resources": {
                            "disk_size": "1024",
                            "disk_type_id": "test_disk_type_id",
                            "generation": "1",
                            "generation_name": "Haswell",
                            "resource_preset_id": "test_preset_id"
                    },
                    "resource_presets": [
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "1",
                                    "decomissioning": false,
                                    "generation": "2",
                                    "generation_name": "Broadwell",
                                    "memory_limit": "4294967296",
                                    "preset_id": "s2.compute.1",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_sizes": {
                                                                        "sizes": [
                                                                            "34359738368",
                                                                            "68719476736",
                                                                            "103079215104",
                                                                            "137438953472"
                                                                        ]
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "2",
                                                                    "min_hosts": "1",
                                                                    "host_count_divider":"1",
                                                                    "max_segment_in_host_count":"0"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            },
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "2",
                                    "decomissioning": false,
                                    "generation": "2",
                                    "generation_name": "Broadwell",
                                    "memory_limit": "8589934592",
                                    "preset_id": "s2.compute.2",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_sizes": {
                                                                        "sizes": [
                                                                            "34359738368",
                                                                            "68719476736",
                                                                            "103079215104",
                                                                            "137438953472"
                                                                        ]
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "2",
                                                                    "min_hosts": "1",
                                                                    "host_count_divider":"1",
                                                                    "max_segment_in_host_count":"0"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            }
                    ],
                    "type": "MASTER"
            },
            {
                    "default_resources": {
                            "disk_size": "1024",
                            "disk_type_id": "test_disk_type_id",
                            "generation": "1",
                            "generation_name": "Haswell",
                            "resource_preset_id": "test_preset_id"
                    },
                    "resource_presets": [
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "1",
                                    "decomissioning": false,
                                    "generation": "2",
                                    "generation_name": "Broadwell",
                                    "memory_limit": "4294967296",
                                    "preset_id": "s2.compute.1",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_sizes": {
                                                                        "sizes": [
                                                                            "34359738368",
                                                                            "68719476736",
                                                                            "103079215104",
                                                                            "137438953472"
                                                                        ]
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "8",
                                                                    "min_hosts": "2",
                                                                    "host_count_divider":"2",
                                                                    "max_segment_in_host_count":"1"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            },
                            {
                                    "cpu_fraction": "100",
                                    "cpu_limit": "2",
                                    "decomissioning": false,
                                    "generation": "2",
                                    "generation_name": "Broadwell",
                                    "memory_limit": "8589934592",
                                    "preset_id": "s2.compute.2",
                                    "type": "standard",
                                    "zones": [
                                            {
                                                    "disk_types": [
                                                            {
                                                                    "disk_sizes": {
                                                                        "sizes": [
                                                                            "34359738368",
                                                                            "68719476736",
                                                                            "103079215104",
                                                                            "137438953472"
                                                                        ]
                                                                    },
                                                                    "disk_type_id": "local-ssd",
                                                                    "max_hosts": "8",
                                                                    "min_hosts": "2",
                                                                    "host_count_divider":"2",
                                                                    "max_segment_in_host_count":"1"
                                                            }
                                                    ],
                                                    "zone_id": "vla"
                                            }
                                    ]
                            }
                    ],
                    "type": "SEGMENT"
            }
    ]
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
    """
    {
      "folder_id": "folder1",
      "host_group_ids": ["hg6", "hg5"]
    }
    """
    Then we get gRPC response with body
    """
    {
    "host_types": [
                {
                    "default_resources": {
                            "disk_size": "1024",
                            "disk_type_id": "test_disk_type_id",
                            "generation": "1",
                            "generation_name": "Haswell",
                            "resource_preset_id": "test_preset_id"
                    },
                    "resource_presets": [],
                    "type": "MASTER"
            },
            {
                    "default_resources": {
                            "disk_size": "1024",
                            "disk_type_id": "test_disk_type_id",
                            "generation": "1",
                            "generation_name": "Haswell",
                            "resource_preset_id": "test_preset_id"
                    },
                    "resource_presets": [],
                    "type": "SEGMENT"
            }
    ]
    }
    """
    And we restore valid resources after shrinking

  Scenario: Billing cost estimation works on dedicated
    When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
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
                "host_group_ids": [
                        "hg4"
                ],
                "config": {
                        "zone_id": "vla",
                        "subnet_id": "",
                        "assign_public_ip": false
                },
                "master_config": {
                        "resources": {
                                "resourcePresetId": "i1.compute.1",
                                "diskTypeId": "local-ssd",
                                "diskSize": 17179869184
                        }
                },
                "segment_config": {
                        "resources": {
                                "resourcePresetId": "s1.compute.1",
                                "diskTypeId": "local-ssd",
                                "diskSize": 17179869184
                        }
                },
                "master_host_count": 2,
                "segment_in_host": 1,
                "segment_host_count": 4,
                "user_name": "usr1",
                "user_password": "Pa$w0rd"
        }
        """
    Then we get gRPC response with body
    """
    {
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "i1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.master_subcluster"],
                    "software_accelerated_network_cores": "2",
                    "on_dedicated_host": "1"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "i1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.master_subcluster"],
                    "software_accelerated_network_cores": "2",
                    "on_dedicated_host": "1"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "1"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "1"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "1"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "greenplum_cluster",
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["greenplum_cluster.segment_subcluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "1"
                }
            }
        ]
    }
    """
# TODO: test resource presets output
