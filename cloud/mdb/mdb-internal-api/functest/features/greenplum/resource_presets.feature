Feature: Get/List Resource Presets

  Background:
    Given default headers

  Scenario: Get resource preset works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ResourcePresetService" with data
    """
    {
      "resource_preset_id": "s1.porto.1",
      "type": "MASTER"
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "s1.porto.1",
      "cores": "1",
      "type": "MASTER",
      "memory": "4294967296",
      "disk_type_ids": ["local-ssd"],
      "host_count_divider": "1",
      "zone_ids": ["iva", "man", "myt", "sas", "vla"]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ResourcePresetService" with data
    """
    {
      "resource_preset_id": "s1.porto.1",
      "type": "SEGMENT"
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "s1.porto.1",
      "cores": "1",
      "type": "SEGMENT",
      "memory": "4294967296",
      "disk_type_ids": ["local-ssd"],
      "max_segment_in_host_count": "1",
      "host_count_divider": "2",
      "zone_ids": ["iva", "man", "myt", "sas", "vla"]
    }
    """

  Scenario: Get resource preset without specified ype
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ResourcePresetService" with data
    """
    {
      "resource_preset_id": "2.4.8.15.16.23.42"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "resource preset type must be MASTER or SEGMENT"

  Scenario: Get non-existing resource preset fails
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ResourcePresetService" with data
    """
    {
      "resource_preset_id": "2.4.8.15.16.23.42",
      "type": "SEGMENT"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "resource preset "2.4.8.15.16.23.42" not found"

  Scenario: List resource presets works
    When we "List" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ResourcePresetService" with data
    """
    {
      "type": "MASTER"
    }
    """
    Then we get gRPC response with body
    """
    {
      "resource_presets": [
        {
          "cores": "1",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.compute.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.compute.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.compute.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.compute.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.compute.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.porto.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.porto.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.porto.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.porto.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.porto.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.compute.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.compute.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.compute.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.compute.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.compute.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.porto.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.porto.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.porto.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.porto.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.porto.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.compute.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.compute.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.compute.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.compute.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.compute.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.porto.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.porto.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.porto.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.porto.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.porto.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        }
      ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ResourcePresetService" with data
    """
    {
      "type": "SEGMENT"
    }
    """
    Then we get gRPC response with body
    """
    {
      "resource_presets": [
        {
          "cores": "1",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s1.compute.1",
          "max_segment_in_host_count": "1",
          "memory": "4294967296",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s1.compute.2",
          "max_segment_in_host_count": "1",
          "memory": "8589934592",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s1.compute.3",
          "max_segment_in_host_count": "2",
          "memory": "17179869184",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s1.compute.4",
          "max_segment_in_host_count": "4",
          "memory": "34359738368",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s1.compute.5",
          "max_segment_in_host_count": "8",
          "memory": "68719476736",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s1.porto.1",
          "max_segment_in_host_count": "1",
          "memory": "4294967296",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s1.porto.2",
          "max_segment_in_host_count": "1",
          "memory": "8589934592",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s1.porto.3",
          "max_segment_in_host_count": "2",
          "memory": "17179869184",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s1.porto.4",
          "max_segment_in_host_count": "4",
          "memory": "34359738368",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s1.porto.5",
          "max_segment_in_host_count": "8",
          "memory": "68719476736",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s2.compute.1",
          "max_segment_in_host_count": "1",
          "memory": "4294967296",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s2.compute.2",
          "max_segment_in_host_count": "1",
          "memory": "8589934592",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s2.compute.3",
          "max_segment_in_host_count": "2",
          "memory": "17179869184",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s2.compute.4",
          "max_segment_in_host_count": "4",
          "memory": "34359738368",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s2.compute.5",
          "max_segment_in_host_count": "8",
          "memory": "68719476736",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s2.porto.1",
          "max_segment_in_host_count": "1",
          "memory": "4294967296",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s2.porto.2",
          "max_segment_in_host_count": "1",
          "memory": "8589934592",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s2.porto.3",
          "max_segment_in_host_count": "2",
          "memory": "17179869184",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s2.porto.4",
          "max_segment_in_host_count": "4",
          "memory": "34359738368",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s2.porto.5",
          "max_segment_in_host_count": "8",
          "memory": "68719476736",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s3.compute.1",
          "max_segment_in_host_count": "1",
          "memory": "4294967296",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s3.compute.2",
          "max_segment_in_host_count": "1",
          "memory": "8589934592",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s3.compute.3",
          "max_segment_in_host_count": "2",
          "memory": "17179869184",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s3.compute.4",
          "max_segment_in_host_count": "4",
          "memory": "34359738368",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "2",
          "id": "s3.compute.5",
          "max_segment_in_host_count": "8",
          "memory": "68719476736",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s3.porto.1",
          "max_segment_in_host_count": "1",
          "memory": "4294967296",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s3.porto.2",
          "max_segment_in_host_count": "1",
          "memory": "8589934592",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s3.porto.3",
          "max_segment_in_host_count": "2",
          "memory": "17179869184",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s3.porto.4",
          "max_segment_in_host_count": "4",
          "memory": "34359738368",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "2",
          "id": "s3.porto.5",
          "max_segment_in_host_count": "8",
          "memory": "68719476736",
          "type": "SEGMENT",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        }
      ]
    }
    """
  Scenario: List resource presets with pagination works
    When we "List" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ResourcePresetService" with data
    """
    {
        "page_size": 2,
        "type": "MASTER"
    }
    """
    Then we get gRPC response with body
    """
    {
      "resource_presets": [
        {
          "cores": "1",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.compute.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.compute.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        }
      ],
      "next_page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ResourcePresetService" with data
    """
    {
        "page_size": 1,
        "page_token": "eyJPZmZzZXQiOjJ9",
        "type": "MASTER"
    }
    """
    Then we get gRPC response with body
    """
    {
      "resource_presets": [
        {
          "cores": "4",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.compute.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        }
      ],
      "next_page_token": "eyJPZmZzZXQiOjN9"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ResourcePresetService" with data
    """
    {
        "page_token": "eyJPZmZzZXQiOjN9",
        "type": "MASTER"
    }
    """
    Then we get gRPC response with body
    """
    {
      "resource_presets": [
        {
          "cores": "8",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.compute.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.compute.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.porto.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.porto.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.porto.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.porto.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s1.porto.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.compute.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.compute.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.compute.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.compute.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.compute.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.porto.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.porto.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.porto.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.porto.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s2.porto.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.compute.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.compute.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.compute.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.compute.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-nvme", "local-ssd", "network-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.compute.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "1",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.porto.1",
          "memory": "4294967296",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "2",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.porto.2",
          "memory": "8589934592",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "4",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.porto.3",
          "memory": "17179869184",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "8",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.porto.4",
          "memory": "34359738368",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        },
        {
          "cores": "16",
          "disk_type_ids": ["local-ssd"],
          "host_count_divider": "1",
          "max_segment_in_host_count": "0",
          "id": "s3.porto.5",
          "memory": "68719476736",
          "type": "MASTER",
          "zone_ids": ["iva", "man", "myt", "sas", "vla"]
        }
      ],
      "next_page_token": ""
    }
    """
