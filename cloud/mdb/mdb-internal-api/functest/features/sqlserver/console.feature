@sqlserver
@grpc_api
Feature: Microsoft SQLServer console API
  Background:
    Given default headers

  Scenario: Cluster config handler works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.console.ClusterService" with data
    """
    {
      "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "cluster_name": {
        "blacklist": [],
        "regexp": "^[a-zA-Z0-9_-]+$",
        "min": "1",
        "max": "63"
      },
      "db_name": {
        "blacklist": [
          "master",
          "msdb",
          "model",
          "tempdb"
        ],
        "regexp": "^[a-zA-Z0-9_-]+$",
        "min": "1",
        "max": "63"
      },
      "user_name": {
        "blacklist": [
          "sa",
          "public",
          "sysadmin",
          "securityadmin",
          "serveradmin",
          "setupadmin",
          "processadmin",
          "diskadmin",
          "dbcreator",
          "bulkadmin",
          "AG_LOGIN",
          "NT AUTHORITY\\SYSTEM",
          "NT Service\\*",
          "mdb_monitor"
        ],
        "regexp": "^[a-zA-Z0-9_][a-zA-Z0-9_-]*$",
        "min": "1",
        "max": "32"
      },
      "password": {
        "blacklist": [],
        "regexp": "^[^\\0\\'\\\"\\n\\r\\t\\x1A\\\\%]*$",
        "min": "8",
        "max": "128"
      },
      "host_count_limits": {
        "min_host_count": "1",
        "max_host_count": "32"
      },
      "versions": [
        "2016sp2std",
        "2016sp2ent"
      ],
      "available_versions": [
        {
          "id": "2016sp2std",
          "name": "2016 ServicePack 2 Standard Edition",
          "deprecated": false,
          "updatable_to": []
        },
        {
          "id": "2016sp2ent",
          "name": "2016 ServicePack 2 Enterprise Edition",
          "deprecated": false,
          "updatable_to": []
        }
      ],
      "default_version": "2016sp2ent",
      "default_resources": {
        "disk_size":          "10737418240",
        "disk_type_id":       "network-ssd",
        "generation":         "1",
        "generation_name":    "Haswell",
        "resource_preset_id": "s1.micro"
      }
    }
    """

  Scenario: Cluster config handler works, dev editions shown
    Given feature flags
    """
    ["MDB_SQLSERVER_ALLOW_DEV", "MDB_SQLSERVER_ALLOW_17_19"]
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.console.ClusterService" with data
    """
    {
      "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "cluster_name": {
        "blacklist": [],
        "regexp": "^[a-zA-Z0-9_-]+$",
        "min": "1",
        "max": "63"
      },
      "db_name": {
        "blacklist": [
          "master",
          "msdb",
          "model",
          "tempdb"
        ],
        "regexp": "^[a-zA-Z0-9_-]+$",
        "min": "1",
        "max": "63"
      },
      "user_name": {
        "blacklist": [
          "sa",
          "public",
          "sysadmin",
          "securityadmin",
          "serveradmin",
          "setupadmin",
          "processadmin",
          "diskadmin",
          "dbcreator",
          "bulkadmin",
          "AG_LOGIN",
          "NT AUTHORITY\\SYSTEM",
          "NT Service\\*",
          "mdb_monitor"
        ],
        "regexp": "^[a-zA-Z0-9_][a-zA-Z0-9_-]*$",
        "min": "1",
        "max": "32"
      },
      "password": {
        "blacklist": [],
        "regexp": "^[^\\0\\'\\\"\\n\\r\\t\\x1A\\\\%]*$",
        "min": "8",
        "max": "128"
      },
      "host_count_limits": {
        "min_host_count": "1",
        "max_host_count": "32"
      },
      "versions": [
        "2016sp2dev",
        "2016sp2std",
        "2016sp2ent",
        "2017dev",
        "2017std",
        "2017ent",
        "2019dev",
        "2019std",
        "2019ent"
      ],
      "available_versions": [
        {
          "id": "2016sp2dev",
          "name": "2016 ServicePack 2 Developer Edition",
          "deprecated": false,
          "updatable_to": []
        },
        {
          "id": "2016sp2std",
          "name": "2016 ServicePack 2 Standard Edition",
          "deprecated": false,
          "updatable_to": []
        },
        {
          "id": "2016sp2ent",
          "name": "2016 ServicePack 2 Enterprise Edition",
          "deprecated": false,
          "updatable_to": []
        },
        {
          "id": "2017dev",
          "name": "2017 Developer Edition",
          "deprecated": false,
          "updatable_to": []
        },
        {
          "id": "2017std",
          "name": "2017 Standard Edition",
          "deprecated": false,
          "updatable_to": []
        },
        {
          "id": "2017ent",
          "name": "2017 Enterprise Edition",
          "deprecated": false,
          "updatable_to": []
        },
        {
          "id": "2019dev",
          "name": "2019 Developer Edition",
          "deprecated": false,
          "updatable_to": []
        },
        {
          "id": "2019std",
          "name": "2019 Standard Edition",
          "deprecated": false,
          "updatable_to": []
        },
        {
          "id": "2019ent",
          "name": "2019 Enterprise Edition",
          "deprecated": false,
          "updatable_to": []
        }
      ],
      "default_version": "2016sp2ent",
      "default_resources": {
        "disk_size":          "10737418240",
        "disk_type_id":       "network-ssd",
        "generation":         "1",
        "generation_name":    "Haswell",
        "resource_preset_id": "s1.micro"
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.console.ClusterService" with data
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.console.ClusterService" with data
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
    When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.console.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "config_spec": {
        "version": "2016sp2ent",
        "resources": {
          "resourcePresetId": "s1.compute.1",
          "diskTypeId": "network-ssd",
          "diskSize": 10737418240
        }
      },
      "hostSpecs": [{
        "zoneId": "myt"
      }, {
        "zoneId": "sas",
        "assignPublicIp": true
      }]
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
                    "cluster_type": "sqlserver_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["sqlserver_cluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0",
                    "edition": "enterprise"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "sqlserver_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "1",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["sqlserver_cluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0",
                    "edition": "enterprise"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.license.v1",
                "tags": {
                    "cluster_type": "",
                    "disk_size": "0",
                    "disk_type_id": "",
                    "online": "0",
                    "public_ip": "0",
                    "resource_preset_id": "",
                    "platform_id": "",
                    "memory": "0",
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0",
                    "roles": ["sqlserver_cluster"],
                    "cores": "4",
                    "core_fraction": "100",
                    "edition": "enterprise"
                }
            }
        ]
    }
    """

  Scenario Outline: Billing cost estimation with unreadable replicas works
    When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.console.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "config_spec": {
        "version": "<version>",
        "secondary_connections": "SECONDARY_CONNECTIONS_OFF",
        "resources": {
          "resourcePresetId": "s1.compute.1",
          "diskTypeId": "network-ssd",
          "diskSize": 10737418240
        }
      },
      "hostSpecs": [{
        "zoneId": "myt"
      }, {
        "zoneId": "sas",
        "assignPublicIp": true
      }]
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
                    "cluster_type": "sqlserver_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["sqlserver_cluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0",
                    "edition": "<edition>"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "sqlserver_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "1",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["sqlserver_cluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0",
                    "edition": "<edition>"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.license.v1",
                "tags": {
                    "cluster_type": "",
                    "disk_size": "0",
                    "disk_type_id": "",
                    "online": "0",
                    "public_ip": "0",
                    "resource_preset_id": "",
                    "platform_id": "",
                    "memory": "0",
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0",
                    "roles": ["sqlserver_cluster"],
                    "cores": "4",
                    "core_fraction": "100",
                    "edition": "<edition>"
                }
            }
        ]
    }
    """
    Examples:
      | version     | edition    |
      | 2016sp2ent  | enterprise |
      | 2016sp2std  | standard   |
      | 2016sp2dev  | developer  |
      | 2017ent     | enterprise |
      | 2017std     | standard   |
      | 2017dev     | developer  |
      | 2019ent     | enterprise |
      | 2019std     | standard   |
      | 2019dev     | developer  |

  Scenario: Cluster create config handler works
    When we "GetCreateConfig" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.console.ClusterService"
    Then we get gRPC response with body
    """
    {
	    "schema": "{\"ClusterCreate\":{\"properties\":{\"configSpec\":{\"$ref\":\"#/SQLServerClusterConfigSpecSchemaV1\"},\"databaseSpecs\":{\"items\":{\"$ref\":\"#/SQLServerDatabaseSpecSchemaV1\"},\"type\":\"array\"},\"description\":{\"description\":\"Description.\",\"maxLength\":256,\"minLength\":0,\"type\":\"string\"},\"environment\":{\"enum\":[\"PRESTABLE\",\"PRODUCTION\"],\"type\":\"string\"},\"folderId\":{\"description\":\"Folder ID.\",\"type\":\"string\"},\"hostSpecs\":{\"items\":{\"$ref\":\"#/SQLServerHostSpecSchemaV1\"},\"type\":\"array\"},\"labels\":{\"description\":\"Labels.\",\"maxLength\":64,\"minLength\":0,\"type\":\"object\"},\"name\":{\"type\":\"string\"},\"networkId\":{\"default\":\"\",\"type\":\"string\"},\"userSpecs\":{\"items\":{\"$ref\":\"#/SQLServerUserSpecSchemaV1\"},\"type\":\"array\"}},\"required\":[\"configSpec\",\"databaseSpecs\",\"environment\",\"folderId\",\"hostSpecs\",\"name\",\"userSpecs\"],\"type\":\"object\"},\"ResourcesSchemaV1\":{\"properties\":{\"diskSize\":{\"format\":\"int32\",\"type\":\"integer\"},\"diskTypeId\":{\"type\":\"string\"},\"resourcePresetId\":{\"type\":\"string\"}},\"required\":[\"diskSize\",\"diskTypeId\",\"resourcePresetId\"],\"type\":\"object\"},\"SQLServer2016sp2devConfig\":{\"properties\":{\"auditLevel\":{\"format\":\"int16\",\"maximum\":3,\"minimum\":0,\"type\":\"integer\"},\"costThresholdForParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":5,\"type\":\"integer\"},\"fillFactorPercent\":{\"format\":\"int16\",\"maximum\":100,\"minimum\":0,\"type\":\"integer\"},\"maxDegreeOfParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":0,\"type\":\"integer\"},\"optimizeForAdHocWorkloads\":{\"type\":\"boolean\"}},\"type\":\"object\"},\"SQLServer2016sp2entConfig\":{\"properties\":{\"auditLevel\":{\"format\":\"int16\",\"maximum\":3,\"minimum\":0,\"type\":\"integer\"},\"costThresholdForParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":5,\"type\":\"integer\"},\"fillFactorPercent\":{\"format\":\"int16\",\"maximum\":100,\"minimum\":0,\"type\":\"integer\"},\"maxDegreeOfParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":0,\"type\":\"integer\"},\"optimizeForAdHocWorkloads\":{\"type\":\"boolean\"}},\"type\":\"object\"},\"SQLServer2016sp2stdConfig\":{\"properties\":{\"auditLevel\":{\"format\":\"int16\",\"maximum\":3,\"minimum\":0,\"type\":\"integer\"},\"costThresholdForParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":5,\"type\":\"integer\"},\"fillFactorPercent\":{\"format\":\"int16\",\"maximum\":100,\"minimum\":0,\"type\":\"integer\"},\"maxDegreeOfParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":0,\"type\":\"integer\"},\"optimizeForAdHocWorkloads\":{\"type\":\"boolean\"}},\"type\":\"object\"},\"SQLServer2017devConfig\":{\"properties\":{\"auditLevel\":{\"format\":\"int16\",\"maximum\":3,\"minimum\":0,\"type\":\"integer\"},\"costThresholdForParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":5,\"type\":\"integer\"},\"fillFactorPercent\":{\"format\":\"int16\",\"maximum\":100,\"minimum\":0,\"type\":\"integer\"},\"maxDegreeOfParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":0,\"type\":\"integer\"},\"optimizeForAdHocWorkloads\":{\"type\":\"boolean\"}},\"type\":\"object\"},\"SQLServer2017entConfig\":{\"properties\":{\"auditLevel\":{\"format\":\"int16\",\"maximum\":3,\"minimum\":0,\"type\":\"integer\"},\"costThresholdForParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":5,\"type\":\"integer\"},\"fillFactorPercent\":{\"format\":\"int16\",\"maximum\":100,\"minimum\":0,\"type\":\"integer\"},\"maxDegreeOfParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":0,\"type\":\"integer\"},\"optimizeForAdHocWorkloads\":{\"type\":\"boolean\"}},\"type\":\"object\"},\"SQLServer2017stdConfig\":{\"properties\":{\"auditLevel\":{\"format\":\"int16\",\"maximum\":3,\"minimum\":0,\"type\":\"integer\"},\"costThresholdForParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":5,\"type\":\"integer\"},\"fillFactorPercent\":{\"format\":\"int16\",\"maximum\":100,\"minimum\":0,\"type\":\"integer\"},\"maxDegreeOfParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":0,\"type\":\"integer\"},\"optimizeForAdHocWorkloads\":{\"type\":\"boolean\"}},\"type\":\"object\"},\"SQLServer2019devConfig\":{\"properties\":{\"auditLevel\":{\"format\":\"int16\",\"maximum\":3,\"minimum\":0,\"type\":\"integer\"},\"costThresholdForParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":5,\"type\":\"integer\"},\"fillFactorPercent\":{\"format\":\"int16\",\"maximum\":100,\"minimum\":0,\"type\":\"integer\"},\"maxDegreeOfParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":0,\"type\":\"integer\"},\"optimizeForAdHocWorkloads\":{\"type\":\"boolean\"}},\"type\":\"object\"},\"SQLServer2019entConfig\":{\"properties\":{\"auditLevel\":{\"format\":\"int16\",\"maximum\":3,\"minimum\":0,\"type\":\"integer\"},\"costThresholdForParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":5,\"type\":\"integer\"},\"fillFactorPercent\":{\"format\":\"int16\",\"maximum\":100,\"minimum\":0,\"type\":\"integer\"},\"maxDegreeOfParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":0,\"type\":\"integer\"},\"optimizeForAdHocWorkloads\":{\"type\":\"boolean\"}},\"type\":\"object\"},\"SQLServer2019stdConfig\":{\"properties\":{\"auditLevel\":{\"format\":\"int16\",\"maximum\":3,\"minimum\":0,\"type\":\"integer\"},\"costThresholdForParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":5,\"type\":\"integer\"},\"fillFactorPercent\":{\"format\":\"int16\",\"maximum\":100,\"minimum\":0,\"type\":\"integer\"},\"maxDegreeOfParallelism\":{\"format\":\"int16\",\"maximum\":32767,\"minimum\":0,\"type\":\"integer\"},\"optimizeForAdHocWorkloads\":{\"type\":\"boolean\"}},\"type\":\"object\"},\"SQLServerClusterConfigSpecSchemaV1\":{\"properties\":{\"backupWindowStart\":{\"$ref\":\"#/TimeOfDay\"},\"resources\":{\"$ref\":\"#/ResourcesSchemaV1\"},\"sqlserverConfig_2016sp2dev\":{\"$ref\":\"#/SQLServer2016sp2devConfig\"},\"sqlserverConfig_2016sp2ent\":{\"$ref\":\"#/SQLServer2016sp2entConfig\"},\"sqlserverConfig_2016sp2std\":{\"$ref\":\"#/SQLServer2016sp2stdConfig\"},\"sqlserverConfig_2017dev\":{\"$ref\":\"#/SQLServer2017devConfig\"},\"sqlserverConfig_2017ent\":{\"$ref\":\"#/SQLServer2017entConfig\"},\"sqlserverConfig_2017std\":{\"$ref\":\"#/SQLServer2017stdConfig\"},\"sqlserverConfig_2019dev\":{\"$ref\":\"#/SQLServer2019devConfig\"},\"sqlserverConfig_2019ent\":{\"$ref\":\"#/SQLServer2019entConfig\"},\"sqlserverConfig_2019std\":{\"$ref\":\"#/SQLServer2019stdConfig\"},\"version\":{\"type\":\"string\"}},\"required\":[\"resources\"],\"type\":\"object\"},\"SQLServerDatabaseRoleV1\":{\"enum\":[\"db_owner\",\"db_securityadmin\",\"db_accessadmin\",\"db_backupoperator\",\"db_ddladmin\",\"db_datawriter\",\"db_datareader\",\"db_denydatawriter\",\"db_denydatareader\"],\"type\":\"string\"},\"SQLServerDatabaseSpecSchemaV1\":{\"properties\":{\"name\":{\"type\":\"string\"}},\"required\":[\"name\"],\"type\":\"object\"},\"SQLServerHostSpecSchemaV1\":{\"properties\":{\"assignPublicIp\":{\"default\":false,\"type\":\"boolean\"},\"configSpec\":{\"$ref\":\"#/SQLServerHostConfigUpdateSpecSchemaV1\"},\"subnetId\":{\"type\":\"string\"},\"zoneId\":{\"description\":\"ID of availability zone.\",\"type\":\"string\"}},\"required\":[\"zoneId\"],\"type\":\"object\"},\"SQLServerPermissionSchemaV1\":{\"properties\":{\"databaseName\":{\"type\":\"string\"},\"roles\":{\"items\":{\"$ref\":\"#/SQLServerDatabaseRoleV1\"},\"type\":\"array\"}},\"required\":[\"databaseName\"],\"type\":\"object\"},\"SQLServerUserSpecSchemaV1\":{\"properties\":{\"name\":{\"type\":\"string\"},\"password\":{\"type\":\"string\"},\"permissions\":{\"items\":{\"$ref\":\"#/SQLServerPermissionSchemaV1\"},\"type\":\"array\"}},\"required\":[\"name\",\"password\"],\"type\":\"object\"},\"TimeOfDay\":{\"properties\":{\"hours\":{\"default\":0,\"format\":\"int32\",\"maximum\":23,\"minimum\":0,\"type\":\"integer\"},\"minutes\":{\"default\":0,\"format\":\"int32\",\"maximum\":59,\"minimum\":0,\"type\":\"integer\"},\"nanos\":{\"default\":0,\"format\":\"int32\",\"maximum\":999999999,\"minimum\":0,\"type\":\"integer\"},\"seconds\":{\"default\":0,\"format\":\"int32\",\"maximum\":59,\"minimum\":0,\"type\":\"integer\"}},\"type\":\"object\"}}"
    }
    """
Scenario: Cluster EstimateCreate with default config works for Standard Edition with two hosts and featureflag
    When we add default feature flag "MDB_SQLSERVER_TWO_NODE_CLUSTER"
    When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.console.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "config_spec": {
        "version": "2016sp2std",
        "resources": {
          "resourcePresetId": "s1.compute.1",
          "diskTypeId": "network-ssd",
          "diskSize": 10737418240
        }
      },
      "hostSpecs": [{
        "zoneId": "myt"
      }, {
        "zoneId": "sas",
        "assignPublicIp": true
      }]
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
                    "cluster_type": "sqlserver_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["sqlserver_cluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0",
                    "edition": "standard"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "sqlserver_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "1",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["sqlserver_cluster"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0",
                    "edition": "standard"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "sqlserver_cluster",
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "online": "1",
                    "public_ip": "0",
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": "1",
                    "core_fraction": "100",
                    "memory": "4294967296",
                    "roles": ["windows_witness"],
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0",
                    "edition": "standard"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.license.v1",
                "tags": {
                    "cluster_type": "",
                    "disk_size": "0",
                    "disk_type_id": "",
                    "online": "0",
                    "public_ip": "0",
                    "resource_preset_id": "",
                    "platform_id": "",
                    "memory": "0",
                    "software_accelerated_network_cores": "0",
                    "on_dedicated_host": "0",
                    "roles": ["sqlserver_cluster"],
                    "cores": "4",
                    "core_fraction": "100",
                    "edition": "standard"
                }
            }
        ]
    }
    """
