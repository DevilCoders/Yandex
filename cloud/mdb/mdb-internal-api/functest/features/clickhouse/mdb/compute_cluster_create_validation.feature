Feature: Create Compute ClickHouse Cluster Validation
  Background:
    Given default headers

  Scenario: Create cluster with 1 host and local disk fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "local-nvme",
                    "diskSize": 107374182400
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "ClickHouse shard with resource preset 's1.compute.1' and disk type 'local-nvme' requires at least 2 hosts"
    }
    """

  Scenario: Create cluster without network id fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Unable to find subnet in zone 'myt'"
    }
    """

  Scenario: Create cluster with nonexistent subnet in network fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Unable to find subnet in zone 'man'"
    }
    """

  Scenario: Create cluster with nonuniq subnet in network fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "iva"
        }],
        "description": "test cluster",
        "networkId": "network2"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Multiple subnets in zone 'iva'"
    }
    """

  Scenario: Create cluster with invalid subnet fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt",
            "subnetId": "nosubnet"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Unable to find subnet with id 'nosubnet' in zone 'myt'"
    }
    """

  Scenario: Create cluster with incorrect disk unit fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418245
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Disk size must be a multiple of 4194304 bytes"
    }
    """

  Scenario: Create cluster with multiple subnets in net fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "vla"
        }],
        "description": "test cluster",
        "network_id": "network2",
        "service_account_id": "sa1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "geo "vla" has multiple subnets, need to specify one"

  Scenario: Create cluster with one of multiple subnets in net passes
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "vla",
            "subnet_id": "network2-vla"
        }],
        "description": "test cluster",
        "network_id": "network2",
        "service_account_id": "sa1"
    }
    """
    Then we get gRPC response OK

  Scenario: Create cluster with generated ZK subcluster and multiple subnets in zone with CH passed
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "folder_id": "folder1",
        "name": "test_subnets",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas",
            "subnet_id": "network2-sas"
        }, {
            "type": "CLICKHOUSE",
            "zone_id": "vla",
            "subnet_id": "network2-vla"
        }],
        "description": "test cluster",
        "network_id": "network2",
        "service_account_id": "sa1"
    }
    """
    Then we get gRPC response OK

  Scenario: Create cluster with generated ZK subcluster and multiple subnets in zone without CH passed
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "folder_id": "folder1",
        "name": "test_subnets",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas",
            "subnet_id": "network2-sas"
        }, {
            "type": "CLICKHOUSE",
            "zone_id": "myt",
            "subnet_id": "network2-myt"
        }],
        "description": "test cluster",
        "network_id": "network2",
        "service_account_id": "sa1"
    }
    """
    Then we get gRPC response OK
