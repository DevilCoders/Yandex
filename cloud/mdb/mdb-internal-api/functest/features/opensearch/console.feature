@opensearch
@grpc_api
Feature: OpenSearch console API
  Background:
    Given default headers

  Scenario: Cluster config handler works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.console.ClusterService"
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
          "7.17", "7.16", "7.15", "7.14", "7.13", "7.12", "7.11", "7.10"
      ],
      "available_versions": [
        {
            "id": "7.17",
            "name": "7.17",
            "deprecated": false,
            "updatable_to": []
        },
        {
            "id": "7.16",
            "name": "7.16",
            "deprecated": false,
            "updatable_to": ["7.17"]
        },
        {
            "id": "7.15",
            "name": "7.15",
            "deprecated": false,
            "updatable_to": ["7.17", "7.16"]
        },
        {
            "id": "7.14",
            "name": "7.14",
            "deprecated": false,
            "updatable_to": ["7.17", "7.16", "7.15"]
        },
        {
            "id": "7.13",
            "name": "7.13",
            "deprecated": false,
            "updatable_to": ["7.17", "7.16", "7.15", "7.14"]
        },
        {
            "id": "7.12",
            "name": "7.12",
            "deprecated": false,
            "updatable_to": ["7.17", "7.16", "7.15", "7.14", "7.13"]
        },
        {
            "id": "7.11",
            "name": "7.11",
            "deprecated": false,
            "updatable_to": ["7.17", "7.16", "7.15", "7.14", "7.13", "7.12"]
        },
        {
            "id": "7.10",
            "name": "7.10",
            "deprecated": false,
            "updatable_to": ["7.17", "7.16", "7.15", "7.14", "7.13", "7.12", "7.11"]
        }
      ],
      "connection_domain": "df.cloud.yandex.net"
    }
    """

  Scenario: Cluster config handler works with feature flags
    When we add cloud "cloud1"
    And we add folder "folder1" to cloud "cloud1"
    And we shrink valid resources in metadb for flavor "s2.compute.2"
    And we shrink valid resources in metadb for disk "local-nvme"
    And we shrink valid resources in metadb for geo "vla,sas"
    And we enable feature flag "MDB_TEST_FLAG" to valid resources described as
    """
    {
        "zone": "sas",
        "preset_id": "s2.compute.2"
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.console.ClusterService" with data
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
                                        "disk_sizes": {
                                            "sizes": [
                                                "107374182400",
                                                "214748364800"
                                            ]
                                        },
                                        "disk_type_id": "local-nvme",
                                        "max_hosts": "7",
                                        "min_hosts": "3"
                                    }
                                ],
                                "zone_id": "vla"
                            }
                        ]
                    }
                ],
                "type": "DATA_NODE"
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
                                        "disk_sizes": {
                                            "sizes": [
                                                "107374182400",
                                                "214748364800"
                                            ]
                                        },
                                        "disk_type_id": "local-nvme",
                                        "max_hosts": "5",
                                        "min_hosts": "3"
                                    }
                                ],
                                "zone_id": "vla"
                            }
                        ]
                    }
                ],
                "type": "MASTER_NODE"
            }
        ]
    }
    """
    And we add feature flag "MDB_TEST_FLAG" for cloud "cloud1"
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.console.ClusterService" with data
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
                                        "disk_sizes": {
                                            "sizes": [
                                                "107374182400",
                                                "214748364800"
                                            ]
                                        },
                                        "disk_type_id": "local-nvme",
                                        "max_hosts": "7",
                                        "min_hosts": "3"
                                    }
                                ],
                                "zone_id": "sas"
                            },
                            {
                                "disk_types": [
                                    {
                                        "disk_sizes": {
                                            "sizes": [
                                                "107374182400",
                                                "214748364800"
                                            ]
                                        },
                                        "disk_type_id": "local-nvme",
                                        "max_hosts": "7",
                                        "min_hosts": "3"
                                    }
                                ],
                                "zone_id": "vla"
                            }
                        ]
                    }
                ],
                "type": "DATA_NODE"
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
                                        "disk_sizes": {
                                            "sizes": [
                                                "107374182400",
                                                "214748364800"
                                            ]
                                        },
                                        "disk_type_id": "local-nvme",
                                        "max_hosts": "5",
                                        "min_hosts": "3"
                                    }
                                ],
                                "zone_id": "sas"
                            },
                            {
                                "disk_types": [
                                    {
                                        "disk_sizes": {
                                            "sizes": [
                                                "107374182400",
                                                "214748364800"
                                            ]
                                        },
                                        "disk_type_id": "local-nvme",
                                        "max_hosts": "5",
                                        "min_hosts": "3"
                                    }
                                ],
                                "zone_id": "vla"
                            }
                        ]
                    }
                ],
                "type": "MASTER_NODE"
            }
        ]
    }
    """
    And we add feature flag "MDB_OPENSEARCH_ALLOW_UNLIMITED_HOSTS" for cloud "cloud1"
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.console.ClusterService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "host_types":[
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
                                        "disk_sizes": {
                                            "sizes": [
                                                "107374182400",
                                                "214748364800"
                                            ]
                                        },
                                        "disk_type_id": "local-nvme",
                                        "max_hosts": "100500",
                                        "min_hosts": "3"
                                    }
                                ],
                                "zone_id": "sas"
                            },
                            {
                                "disk_types": [
                                    {
                                        "disk_sizes": {
                                            "sizes": [
                                                "107374182400",
                                                "214748364800"
                                            ]
                                        },
                                        "disk_type_id": "local-nvme",
                                        "max_hosts": "100500",
                                        "min_hosts": "3"
                                    }
                                ],
                                "zone_id": "vla"
                            }
                        ]
                    }
                ],
                "type": "DATA_NODE"
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
                                        "disk_sizes": {
                                            "sizes": [
                                                "107374182400",
                                                "214748364800"
                                            ]
                                        },
                                        "disk_type_id": "local-nvme",
                                        "max_hosts": "5",
                                        "min_hosts": "3"
                                    }
                                ],
                                "zone_id": "sas"
                            },
                            {
                                "disk_types": [
                                    {
                                        "disk_sizes": {
                                            "sizes": [
                                                "107374182400",
                                                "214748364800"
                                            ]
                                        },
                                        "disk_type_id": "local-nvme",
                                        "max_hosts": "5",
                                        "min_hosts": "3"
                                    }
                                ],
                                "zone_id": "vla"
                            }
                        ]
                    }
                ],
                "type": "MASTER_NODE"
            }
        ]
    }
    """
    And we restore valid resources after shrinking


  Scenario: Billing cost estimation works
    When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.console.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.10",
            "opensearch_spec": {
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
        },
        {
            "zone_id": "sas",
            "type": "MASTER_NODE"
        }],
        "description": "test cluster",
        "networkId": "network1"
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
                    "cluster_type": "opensearch_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["opensearch_cluster.datanode"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "opensearch_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["opensearch_cluster.masternode"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0"
                }
            }
        ]
    }
    """
  Scenario: Billing cost estimation works without master nodes
    When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.console.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.10",
            "opensearch_spec": {
                "data_node": {
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
    Then we get response with status 200
# TODO: test resource presets output

  Scenario: Cluster create config handler works
    When we "GetCreateConfig" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.console.ClusterService"
    Then we get response with status 200

  Scenario: Billing cost estimation error on missing resources
    When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.console.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.10",
            "opensearch_spec": {
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
        },
        {
            "zone_id": "sas",
            "type": "MASTER_NODE"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "data node resources not defined"
    When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.console.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.10",
            "opensearch_spec": {
                "data_node": {
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
        },
        {
            "zone_id": "sas",
            "type": "MASTER_NODE"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "master node resources not defined"
