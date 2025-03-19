@kafka
@grpc_api
Feature: Apache Kafka console API
  Background:
    Given default headers

  Scenario: Cluster config handler works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.console.ClusterService"
    Then we get gRPC response with body
    """
    {
      "cluster_name": {
        "blacklist": [],
        "regexp": "^[a-zA-Z0-9_-]+$",
        "min": "1",
        "max": "63"
      },
      "topic_name": {
        "blacklist": [],
        "regexp": "^[-a-zA-Z0-9_\\.]+$",
        "min": "1",
        "max": "249"
      },
      "user_name": {
        "blacklist": [],
        "regexp": "(?!mdb_)[a-zA-Z0-9_-]+",
        "min": "1",
        "max": "256"
      },
      "password": {
        "blacklist": [],
        "regexp": ".*",
        "min": "8",
        "max": "128"
      },
      "broker_count_limits": {
        "min_broker_count": "1",
        "max_broker_count": "32"
      },
      "versions": [
        "3.2",
        "3.1",
        "3.0",
        "2.8",
        "2.6",
        "2.1"
      ],
      "available_versions": [
        {
          "id": "3.2",
          "name": "3.2",
          "deprecated": false,
          "updatable_to": []
        },
        {
          "id": "3.1",
          "name": "3.1",
          "deprecated": false,
          "updatable_to": []
        },
        {
          "id": "3.0",
          "name": "3.0",
          "deprecated": false,
          "updatable_to": ["3.1"]
        },
        {
          "id": "2.8",
          "name": "2.8",
          "deprecated": false,
          "updatable_to": ["3.0", "3.1"]
        },
        {
          "id": "2.6",
          "name": "2.6",
          "deprecated": true,
          "updatable_to": ["2.8", "3.0", "3.1"]
        },
        {
          "id": "2.1",
          "name": "2.1",
          "deprecated": true,
          "updatable_to": ["2.6", "2.8", "3.0", "3.1"]
        }
      ],
      "default_version": "3.0",
      "default_resources": {
        "disk_size":          "32212254720",
        "disk_type_id":       "network-ssd",
        "generation":         "1",
        "generation_name":    "Haswell",
        "resource_preset_id": "s2.compute.1"
      }
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.console.ClusterService" with data
        """
        {
        "folder_id": "folder1"
        }
        """
    Then we get gRPC response with body
        """
        {
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
        ]
        }
        """
    And we add feature flag "MDB_TEST_FLAG" for cloud "cloud1"
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.console.ClusterService" with data
        """
        {
        "folder_id": "folder1"
        }
        """
    Then we get gRPC response with body
        """
        {
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
        ]
        }
        """
    And we restore valid resources after shrinking


  Scenario: Billing cost estimation works
    When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.console.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
          }
        }
      }
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
                    "cluster_type": "kafka_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s2.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["kafka_cluster"],
                    "on_dedicated_host": "0",
                    "software_accelerated_network_cores": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "kafka_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s2.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["kafka_cluster"],
                    "on_dedicated_host": "0",
                    "software_accelerated_network_cores": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "kafka_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s2.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["zk"],
                    "on_dedicated_host": "0",
                    "software_accelerated_network_cores": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "kafka_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s2.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["zk"],
                    "on_dedicated_host": "0",
                    "software_accelerated_network_cores": "0"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "kafka_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s2.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["zk"],
                    "on_dedicated_host": "0",
                    "software_accelerated_network_cores": "0"
                }
            }
        ]
    }
    """
