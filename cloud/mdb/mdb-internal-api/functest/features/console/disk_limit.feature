@grpc_api
Feature: Disk limits
  Background:
    Given default headers

  Scenario Outline: Get disk IO limits
    When we "GetDiskIOLimit" via gRPC at "yandex.cloud.priv.mdb.v1.console.ClusterService" with data
    """
    {
      "space_limit": "<space_limit>",
      "disk_type": "<disk_type>",
      "resource_preset": "<resource_preset>"
    }
    """
    Then we get gRPC response with body
    """
    {
      "io_limit": "<expected>"
    }
    """
    Examples:
      | space_limit  | disk_type   | resource_preset | expected |
      | 137438953472 | limit-ssd   | s1.porto.1      | 20971520 |
      | 137438953472 | limit-ssd   | s2.porto.1      | 20971520 |
      | 137438953472 | limit-ssd   | s3.porto.1      | 20971520 |
      | 137438953472 | limit-ssd   | s4.porto.1      | 67108864 |

      | 137438953472 | network-ssd | s3.porto.1      | 20971520 |
      | 137438953472 | network-ssd | s4.porto.1      | 20971520 |

      | 137438953472 | limit-ssd   | s1.compute.1    | 67108864 |

  Scenario: Unknown disk_type
    When we "GetDiskIOLimit" via gRPC at "yandex.cloud.priv.mdb.v1.console.ClusterService" with data
    """
    {
      "space_limit": 42,
      "disk_type": "ssd",
      "resource_preset": "s1.porto.1"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "Unknown disk_type: ssd"

  Scenario: Unknown flavor_name
    When we "GetDiskIOLimit" via gRPC at "yandex.cloud.priv.mdb.v1.console.ClusterService" with data
      """
      {
        "space_limit": 42,
        "disk_type": "local-ssd",
        "resource_preset": "flavor"
      }
      """
    Then we get gRPC response error with code NOT_FOUND and message "Unknown resource_preset: flavor"
