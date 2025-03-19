@greenplum
@grpc_api
Feature: Wizard for cluster configurations
    Background:
        Given default headers
        Given we add default feature flag "MDB_GREENPLUM_CLUSTER"
        Given we add default feature flag "MDB_ALLOW_NETWORK_SSD_NONREPLICATED"
    Scenario: Recommend cluster for 1 Tb data size with local-ssd success
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
        """
        {
          "database_size": 1099511627776,
          "type": "standard",
          "folder_id": "folder1",
          "disk_type_id": "local-ssd"
        }
        """
        Then we get gRPC response with body
        """
        {
          "master_hosts": "2",
          "segment_hosts": "4",
          "segments_per_host": "1",
          "master_config": {
            "resources": {
              "resource_preset_id": "s3.porto.2",
              "disk_type_id": "local-ssd",
              "disk_size": "42949672960",
              "generation": "3",
              "type": "standard"
            }
          },
          "segment_config": {
            "resources": {
              "resource_preset_id": "s3.porto.2",
              "disk_type_id": "local-ssd",
              "disk_size": "395136991232",
              "generation": "3",
              "type": "standard"
            }
          }
        }
        """

    Scenario: Recommend cluster for 1 byte data size with network-ssd-nonreplicated ( check minimal bound for NRD ) success
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
            """
            {
              "database_size": 1,
              "type": "standard",
              "folder_id": "folder1",
              "disk_type_id": "network-ssd-nonreplicated"
            }
            """
        Then we get gRPC response with body
            """
            {
              "master_hosts": "2",
              "segment_hosts": "4",
              "segments_per_host": "1",
              "master_config": {
                "resources": {
                  "resource_preset_id": "s3.compute.2",
                  "disk_type_id": "network-ssd-nonreplicated",
                  "disk_size": "42949672960",
                  "generation": "3",
                  "type": "standard"
                }
              },
              "segment_config": {
                "resources": {
                  "resource_preset_id": "s3.compute.2",
                  "disk_type_id": "network-ssd-nonreplicated",
                  "disk_size": "99857989632",
                  "generation": "3",
                  "type": "standard"
                }
              }
            }
            """

    Scenario: Recommend cluster for 1 byte data size with local-ssd ( check minimal bound for local-ssd ) success
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
            """
            {
              "database_size": 1,
              "type": "standard",
              "folder_id": "folder1",
              "disk_type_id": "local-ssd"
            }
            """
        Then we get gRPC response with body
            """
            {
              "master_hosts": "2",
              "segment_hosts": "4",
              "segments_per_host": "1",
              "master_config": {
                "resources": {
                  "resource_preset_id": "s3.porto.2",
                  "disk_type_id": "local-ssd",
                  "disk_size": "42949672960",
                  "generation": "3",
                  "type": "standard"
                }
              },
              "segment_config": {
                "resources": {
                  "resource_preset_id": "s3.porto.2",
                  "disk_type_id": "local-ssd",
                  "disk_size": "395136991232",
                  "generation": "3",
                  "type": "standard"
                }
              }
            }
            """

    Scenario: Recommend cluster on dedicated hosts when data size more then 20 Tb on local-ssd when user chose local-ssd success
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
            """
            {
              "database_size": 26189255811073,
              "type": "standard",
              "folder_id": "folder1",
              "disk_type_id": "local-ssd"
            }
            """
        Then we get gRPC response with body
            """
            {
              "master_hosts": "2",
              "segment_hosts": "11",
              "segments_per_host": "4",
              "need_dedicated_hosts": true,
              "master_config": {
                "resources": {
                  "resource_preset_id": "s3.porto.5",
                  "disk_type_id": "local-ssd",
                  "disk_size": "343597383680",
                  "generation": "3",
                  "type": "standard"
                }
              },
              "segment_config": {
                "resources": {
                  "resource_preset_id": "s3.porto.5",
                  "disk_type_id": "local-ssd",
                  "disk_size": "20615843020800",
                  "generation": "3",
                  "type": "standard"
                }
              }
            }
            """

    Scenario: Recommend cluster on dedicated hosts when data size more then 20 Tb on local-ssd when user chose network-ssd-nonreplicated success
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
            """
            {
              "database_size": 26189255811073,
              "type": "standard",
              "folder_id": "folder1",
              "disk_type_id": "network-ssd-nonreplicated"
            }
            """
        Then we get gRPC response with body
            """
            {
              "master_hosts": "2",
              "segment_hosts": "11",
              "segments_per_host": "4",
              "need_dedicated_hosts": true,
              "master_config": {
                "resources": {
                  "resource_preset_id": "s3.porto.5",
                  "disk_type_id": "local-ssd",
                  "disk_size": "343597383680",
                  "generation": "3",
                  "type": "standard"
                }
              },
              "segment_config": {
                "resources": {
                  "resource_preset_id": "s3.porto.5",
                  "disk_type_id": "local-ssd",
                  "disk_size": "20615843020800",
                  "generation": "3",
                  "type": "standard"
                }
              }
            }
            """

    Scenario: Recommend cluster on dedicated hosts when user wants dedicated hosts and data size is smaller than 20 Tb success
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
            """
            {
              "database_size": 1,
              "type": "standard",
              "folder_id": "folder1",
              "need_dedicated_hosts": true,
              "disk_type_id": "network-ssd-nonreplicated"
            }
            """
        Then we get gRPC response with body
            """
            {
              "master_hosts": "2",
              "segment_hosts": "4",
              "segments_per_host": "4",
              "need_dedicated_hosts": true,
              "master_config": {
                "resources": {
                  "resource_preset_id": "s3.porto.5",
                  "disk_type_id": "local-ssd",
                  "disk_size": "343597383680",
                  "generation": "3",
                  "type": "standard"
                }
              },
              "segment_config": {
                "resources": {
                  "resource_preset_id": "s3.porto.5",
                  "disk_type_id": "local-ssd",
                  "disk_size": "20615843020800",
                  "generation": "3",
                  "type": "standard"
                }
              }
            }
            """

    Scenario: Check disk size round up on recommended cluster for local-ssd success
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
            """
            {
              "database_size": 10995116277760,
              "type": "standard",
              "folder_id": "folder1",
              "disk_type_id": "local-ssd"
            }
            """
        Then we get gRPC response with body
            """
            {
              "master_hosts": "2",
              "segment_hosts": "4",
              "segments_per_host": "4",
              "master_config": {
                "resources": {
                  "resource_preset_id": "s3.porto.5",
                  "disk_type_id": "local-ssd",
                  "disk_size": "343597383680",
                  "generation": "3",
                  "type": "standard"
                }
              },
              "segment_config": {
                "resources": {
                  "resource_preset_id": "s3.porto.5",
                  "disk_type_id": "local-ssd",
                  "disk_size": "2765958938624",
                  "generation": "3",
                  "type": "standard"
                }
              }
            }
        """

    Scenario: Check disk size round up on recommended cluster for network-ssd-nonreplicated success
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
            """
            {
              "database_size": 10995116277760,
              "type": "standard",
              "folder_id": "folder1",
              "disk_type_id": "network-ssd-nonreplicated"
            }
            """
        Then we get gRPC response with body
            """
            {
              "master_hosts": "2",
              "segment_hosts": "4",
              "segments_per_host": "4",
              "master_config": {
                "resources": {
                  "resource_preset_id": "s3.compute.5",
                  "disk_type_id": "network-ssd-nonreplicated",
                  "disk_size": "343597383680",
                  "generation": "3",
                  "type": "standard"
                }
              },
              "segment_config": {
                "resources": {
                  "resource_preset_id": "s3.compute.5",
                  "disk_type_id": "network-ssd-nonreplicated",
                  "disk_size": "2396591751168",
                  "generation": "3",
                  "type": "standard"
                }
              }
            }
        """

    Scenario: Request configuration with wrong data_size <= 0 fails
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
            """
            {
              "database_size": 0,
              "type": "standard",
              "folder_id": "folder1",
              "disk_type_id": "network-ssd-nonreplicated"
            }
            """

        Then we get gRPC response error with code INVALID_ARGUMENT and message "database_size should be > 0 , got 0"

    Scenario: Request configuration with wrong flavor_type xyz fails
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
            """
            {
              "database_size": 12303030303,
              "type": "xyz",
              "folder_id": "folder1",
              "disk_type_id": "network-ssd-nonreplicated"
            }
            """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "unknown flavor_type expected: [standard, io-optimized] got xyz"

    Scenario: Request configuration with no folder_id fails
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
            """
            {
              "database_size": 1230303030300,
              "type": "standard",
              "disk_type_id": "network-ssd-nonreplicated"
            }
            """

        Then we get gRPC response error with code INVALID_ARGUMENT and message "no folder_id specified"

    Scenario: Request configuration with unknown disk type fails
        When we "GetRecommendedConfig" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.console.ClusterService" with data
            """
            {
              "database_size": 1230303030300,
              "type": "standard",
              "folder_id": "folder1",
              "disk_type_id": "xyz"
            }
            """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "unknown disk_type expected: [local-ssd, network-ssd-nonreplicated] got xyz"
