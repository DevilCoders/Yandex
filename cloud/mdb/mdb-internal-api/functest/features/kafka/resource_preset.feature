Feature: Get/List Resource Presets

  Background:
    Given default headers

  Scenario: Get resource preset works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ResourcePresetService" with data
    """
    {
        "resource_preset_id": "s1.porto.1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "id": "s1.porto.1",
        "zone_ids": ["iva", "man", "myt", "sas", "vla"],
        "cores": "1",
        "memory": "4294967296"
    }
    """

  Scenario: Get non-existing resource preset fails
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ResourcePresetService" with data
    """
    {
        "resource_preset_id": "2.4.8.15.16.23.42"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "resource preset "2.4.8.15.16.23.42" not found"

  Scenario: List resource presets works
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ResourcePresetService"
    Then we get gRPC response with body
    """
    {
        "resource_presets": [
            {
                "id": "a1.aws.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "4294967296"
            },
            {
                "id": "a1.aws.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "8589934592"
            },
            {
                "id": "a1.aws.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "17179869184"
            },
            {
                "id": "a1.aws.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "34359738368"
            },
            {
                "id": "a1.aws.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "68719476736"
            },
            {
                "id": "b1.compute.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "2147483648"
            },
            {
                "id": "b1.compute.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "4294967296"
            },
            {
                "id": "b1.compute.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "8589934592"
            },
            {
                "id": "b1.compute.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "17179869184"
            },
            {
                "id": "b1.compute.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "34359738368"
            },
            {
                "id": "b1.porto.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "2147483648"
            },
            {
                "id": "b1.porto.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "4294967296"
            },
            {
                "id": "b1.porto.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "8589934592"
            },
            {
                "id": "b1.porto.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "17179869184"
            },
            {
                "id": "b1.porto.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "34359738368"
            },
            {
                "id": "b2.compute.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "2147483648"
            },
            {
                "id": "b2.compute.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "4294967296"
            },
            {
                "id": "b2.compute.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "8589934592"
            },
            {
                "id": "b2.compute.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "17179869184"
            },
            {
                "id": "b2.compute.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "34359738368"
            },
            {
                "id": "b2.porto.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "2147483648"
            },
            {
                "id": "b2.porto.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "4294967296"
            },
            {
                "id": "b2.porto.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "8589934592"
            },
            {
                "id": "b2.porto.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "17179869184"
            },
            {
                "id": "b2.porto.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "34359738368"
            },
            {
                "id": "s1.compute.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "4294967296"
            },
            {
                "id": "s1.compute.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "8589934592"
            },
            {
                "id": "s1.compute.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "17179869184"
            },
            {
                "id": "s1.compute.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "34359738368"
            },
            {
                "id": "s1.compute.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "68719476736"
            },
            {
                "id": "s1.porto.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "4294967296"
            },
            {
                "id": "s1.porto.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "8589934592"
            },
            {
                "id": "s1.porto.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "17179869184"
            },
            {
                "id": "s1.porto.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "34359738368"
            },
            {
                "id": "s1.porto.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "68719476736"
            },
            {
                "id": "s2.compute.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "4294967296"
            },
            {
                "id": "s2.compute.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "8589934592"
            },
            {
                "id": "s2.compute.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "17179869184"
            },
            {
                "id": "s2.compute.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "34359738368"
            },
            {
                "id": "s2.compute.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "68719476736"
            },
            {
                "id": "s2.porto.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "4294967296"
            },
            {
                "id": "s2.porto.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "8589934592"
            },
            {
                "id": "s2.porto.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "17179869184"
            },
            {
                "id": "s2.porto.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "34359738368"
            },
            {
                "id": "s2.porto.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "68719476736"
            }
        ]
    }
    """

  Scenario: List resource presets with pagination works
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ResourcePresetService" with data
    """
    {
        "page_size": 2
    }
    """
    Then we get gRPC response with body
    """
    {
        "resource_presets": [
            {
                "id": "a1.aws.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "4294967296"
            },
            {
                "id": "a1.aws.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "8589934592"
            }
        ],
        "next_page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ResourcePresetService" with data
    """
    {
        "page_size": 1,
        "page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    Then we get gRPC response with body
    """
    {
        "resource_presets": [
            {
                "id": "a1.aws.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "17179869184"
            }
        ],
        "next_page_token": "eyJPZmZzZXQiOjN9"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ResourcePresetService" with data
    """
    {
        "page_token": "eyJPZmZzZXQiOjN9"
    }
    """
    Then we get gRPC response with body
    """
    {
        "resource_presets": [
            {
                "id": "a1.aws.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "34359738368"
            },
            {
                "id": "a1.aws.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "68719476736"
            },
            {
                "id": "b1.compute.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "2147483648"
            },
            {
                "id": "b1.compute.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "4294967296"
            },
            {
                "id": "b1.compute.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "8589934592"
            },
            {
                "id": "b1.compute.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "17179869184"
            },
            {
                "id": "b1.compute.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "34359738368"
            },
            {
                "id": "b1.porto.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "2147483648"
            },
            {
                "id": "b1.porto.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "4294967296"
            },
            {
                "id": "b1.porto.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "8589934592"
            },
            {
                "id": "b1.porto.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "17179869184"
            },
            {
                "id": "b1.porto.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "34359738368"
            },
            {
                "id": "b2.compute.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "2147483648"
            },
            {
                "id": "b2.compute.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "4294967296"
            },
            {
                "id": "b2.compute.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "8589934592"
            },
            {
                "id": "b2.compute.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "17179869184"
            },
            {
                "id": "b2.compute.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "34359738368"
            },
            {
                "id": "b2.porto.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "2147483648"
            },
            {
                "id": "b2.porto.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "4294967296"
            },
            {
                "id": "b2.porto.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "8589934592"
            },
            {
                "id": "b2.porto.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "17179869184"
            },
            {
                "id": "b2.porto.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "34359738368"
            },
            {
                "id": "s1.compute.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "4294967296"
            },
            {
                "id": "s1.compute.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "8589934592"
            },
            {
                "id": "s1.compute.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "17179869184"
            },
            {
                "id": "s1.compute.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "34359738368"
            },
            {
                "id": "s1.compute.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "68719476736"
            },
            {
                "id": "s1.porto.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "4294967296"
            },
            {
                "id": "s1.porto.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "8589934592"
            },
            {
                "id": "s1.porto.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "17179869184"
            },
            {
                "id": "s1.porto.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "34359738368"
            },
            {
                "id": "s1.porto.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "68719476736"
            },
            {
                "id": "s2.compute.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "4294967296"
            },
            {
                "id": "s2.compute.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "8589934592"
            },
            {
                "id": "s2.compute.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "17179869184"
            },
            {
                "id": "s2.compute.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "34359738368"
            },
            {
                "id": "s2.compute.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "68719476736"
            },
            {
                "id": "s2.porto.1",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "1",
                "memory": "4294967296"
            },
            {
                "id": "s2.porto.2",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "2",
                "memory": "8589934592"
            },
            {
                "id": "s2.porto.3",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "4",
                "memory": "17179869184"
            },
            {
                "id": "s2.porto.4",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "8",
                "memory": "34359738368"
            },
            {
                "id": "s2.porto.5",
                "zone_ids": ["iva", "man", "myt", "sas", "vla"],
                "cores": "16",
                "memory": "68719476736"
            }
        ],
        "next_page_token": ""
    }
    """
