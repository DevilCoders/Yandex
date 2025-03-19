Feature: Non-cluster-specific scenarios

  Scenario: Ping handle returns 200 if close file absent
    And we "GET" via REST at "/ping"
    Then we get response with status 200
    When we "Check" via gRPC at "grpc.health.v1.Health"
    Then we get gRPC response with body
    """
    {
        "status": "SERVING"
    }
    """

  Scenario: Close file works
    When we create flag "close"
    And we "GET" via REST at "/ping"
    Then we get response with status 405
    When we "Check" via gRPC at "grpc.health.v1.Health"
    Then we get gRPC response with body
    """
    {
        "status": "NOT_SERVING"
    }
    """

  Scenario: Read only mode enabled
    When we create flag "read-only"
    And we "Update" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": ""
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "read-only mode"

  Scenario: Health service and read only mode
    When we create flag "read-only"
    And we "Check" via gRPC at "grpc.health.v1.Health"
      When we "Check" via gRPC at "grpc.health.v1.Health"
      Then we get gRPC response with body
    """
    {
        "status": "SERVING"
    }
    """

  @stat-handle
  Scenario: Stat handle works
    When we GET "/stat"
    # Unfortunately we are not going to check full response body
    # because it is environment-dependent
    Then we get response with status 200
    And body matches schema
    """
    {
        "type": "array",
        "items": {
            "type": "array",
            "items": [
                {"type": "string"},
                {"type": "number"}
            ],
            "minItems": 2
        },
        "minItems": 3
    }
    """
    # Unfortunately we are not going to check explicit count_2xx,
    # because in A.y-t we don't have .featureset support,
    # so we can't rely on features order
    And body matches schema
    """
    {
        "type": "array",
        "contains": {
            "type": "array",
            "items": [
                {"type": "string", "const": "dbaas_internal_api_count_2xx_dmmm"},
                {"type": "number"}
            ],
            "minItems": 2,
            "maxItems": 2
        }
    }
    """

  Scenario: Posting with invalid json fails
    Given default headers
    When we POST "/mdb/redis/1.0/clusters" with data
    """
    {My cool
    invalid
    json
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "Request body is not a valid JSON"
    }
    """

  Scenario: Read-only file works
    Given default headers
    When we create flag "read-only"
    And we POST "/mdb/redis/1.0/clusters" with data
    """
    {}

    """
    Then we get response with status 503 and body contains
    """
    {
        "code": 14,
        "message": "Read-only"
    }
    """

  Scenario: Passwords are masked in logs
    Given default headers
    When we POST "/mdb/redis/1.0/clusters" with data
    """
    {
        "password": "q4f8T*3khy^D2f32i#tC7h7Qx",
        "clear": "xNRi9746E3rkwQWgCo2D7+)7["
    }
    """
    Then string "q4f8T*3khy^D2f32i#tC7h7Qx" is not in tskv log
    And string "xNRi9746E3rkwQWgCo2D7+)7[" is in tskv log

  Scenario Outline: Resource preset list works
    When we GET "/mdb/<type>/1.0/resourcePresets?pageSize=1"
    Then we get response with status 200 and body contains
    """
    {
        "resourcePresets": [
            {
                "cores": 1,
                "coreFraction": 5,
                "id": "b2.compute.1",
                "memory": 2147483648,
                "type": "burstable",
                "generation": 2,
                <host types>
                "zoneIds": [
                    "iva",
                    "man",
                    "myt",
                    "sas",
                    "vla"
                ]
            }
        ]
    }
    """
    Examples:
      | type       | host types                                              |
      | redis      |                                                         |
      | mysql      |                                                         |
      | postgresql |                                                         |
      | mongodb    | "hostTypes": ["MONGOD"],                                |
      | hadoop     | "hostTypes": ["MASTERNODE", "DATANODE", "COMPUTENODE"], |

  Scenario Outline: Resource preset get works
    When we GET "/mdb/<type>/1.0/resourcePresets/s1.porto.1"
    Then we get response with status 200 and body contains
    """
    {
        "cores": 1,
        "id": "s1.porto.1",
        "memory": 4294967296,
        "type": "standard",
        "generation": 1,
        <host types>
        "zoneIds": [
            "iva",
            "man",
            "myt",
            "sas",
            "vla"
        ]
    }
    """
    Examples:
      | type       | host types                                                   |
      | redis      |                                                              |
      | mysql      |                                                              |
      | postgresql |                                                              |
      | mongodb    | "hostTypes": ["MONGOD", "MONGOCFG", "MONGOS", "MONGOINFRA"], |
      | clickhouse | "hostTypes": ["CLICKHOUSE", "ZOOKEEPER"],                    |
      | hadoop     | "hostTypes": ["MASTERNODE", "DATANODE", "COMPUTENODE"],      |

  Scenario Outline: Console schema get works
    Given default headers
    When we GET "<handle>?folderId=folder1"
    Then we get response with status 200
    And body is valid JSON Schema
    Examples:
      | handle                                                             |
      | /mdb/postgresql/1.0/console/clusters:clusterCreateConfig           |
      | /mdb/postgresql/1.0/console/clusters:clusterModifyConfig           |
      | /mdb/postgresql/1.0/console/clusters:userCreateConfig              |
      | /mdb/postgresql/1.0/console/clusters:userModifyConfig              |
      | /mdb/postgresql/1.0/console/clusters:databaseCreateConfig          |
      | /mdb/postgresql/1.0/console/clusters:databaseModifyConfig          |
      | /mdb/clickhouse/1.0/console/clusters:clusterCreateConfig           |
      | /mdb/clickhouse/1.0/console/clusters:clusterModifyConfig           |
      | /mdb/clickhouse/1.0/console/clusters:userCreateConfig              |
      | /mdb/clickhouse/1.0/console/clusters:userModifyConfig              |
      | /mdb/clickhouse/1.0/console/clusters:databaseCreateConfig          |
      | /mdb/clickhouse/1.0/console/clusters:clusterCreateDictionaryConfig |
      | /mdb/mongodb/1.0/console/clusters:clusterCreateConfig              |
      | /mdb/mongodb/1.0/console/clusters:clusterModifyConfig              |
      | /mdb/mongodb/1.0/console/clusters:userCreateConfig                 |
      | /mdb/mongodb/1.0/console/clusters:userModifyConfig                 |
      | /mdb/mongodb/1.0/console/clusters:databaseCreateConfig             |
      | /mdb/mysql/1.0/console/clusters:clusterCreateConfig                |
      | /mdb/mysql/1.0/console/clusters:clusterModifyConfig                |
      | /mdb/mysql/1.0/console/clusters:userCreateConfig                   |
      | /mdb/mysql/1.0/console/clusters:userModifyConfig                   |
      | /mdb/mysql/1.0/console/clusters:databaseCreateConfig               |
      | /mdb/redis/1.0/console/clusters:clusterCreateConfig                |
      | /mdb/redis/1.0/console/clusters:clusterModifyConfig                |
      | /mdb/hadoop/1.0/console/clusters:clusterCreateConfig               |
      | /mdb/hadoop/1.0/console/clusters:clusterModifyConfig               |

  Scenario: Default quota on not created cloud works
    Given default headers
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersQuota": 2,
        "clustersUsed": 0,
        "cpuQuota": 24.0,
        "cpuUsed": 0.0,
        "memoryQuota": 103079215104,
        "memoryUsed": 0,
        "ssdSpaceQuota": 644245094400,
        "ssdSpaceUsed": 0,
        "hddSpaceQuota": 644245094400,
        "hddSpaceUsed": 0
    }
    """

  Scenario: Cluster types list by folder works
    Given default headers
    When we GET "/mdb/1.0/console/clusters:types?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusterTypes": [
            {
                "type": "clickhouse",
                "versions": [
                    "21.1",
                    "21.2",
                    "21.3",
                    "21.4",
                    "21.7",
                    "21.8",
                    "21.11",
                    "22.1",
                    "22.3",
                    "22.5"
                 ]
            },
            {
                "type": "hadoop",
                "versions": [
                    "2.0",
                    "1.4",
                    "1.3"
                ]
            },
            {
                "type": "mongodb",
                "versions": [
                    "3.6",
                    "4.0",
                    "4.2",
                    "4.4",
                    "5.0"
                ]
            },
            {
                "type": "mysql",
                "versions": [
                    "5.7"
                ]
            },
            {
                "type": "postgresql",
                "versions": [
                    "10",
		            "11",
		            "12",
		            "13",
		            "14"
                ]
            },
            {
                "type": "redis",
                "versions": [
                    "5.0",
                    "6.0"
                ]
            }
        ]
    }
    """
    Given feature flags
    """
    ["MDB_POSTGRESQL_11", "MDB_MYSQL_8_0", "MDB_DATAPROC_IMAGE_1_3", "MDB_REDIS_62"]
    """
    When we GET "/mdb/1.0/console/clusters:types?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusterTypes": [
            {
                "type": "clickhouse",
                "versions": [
                    "21.1",
                    "21.2",
                    "21.3",
                    "21.4",
                    "21.7",
                    "21.8",
                    "21.11",
                    "22.1",
                    "22.3",
                    "22.5"
                ]
            },
            {
                "type": "hadoop",
                "versions": [
                    "2.0",
                    "1.4",
                    "1.3"
                ]
            },
            {
                "type": "mongodb",
                "versions": [
                    "3.6",
                    "4.0",
                    "4.2",
                    "4.4",
                    "5.0"
                ]
            },
            {
                "type": "mysql",
                "versions": [
                    "5.7",
                    "8.0"
                ]
            },
            {
                "type": "postgresql",
                "versions": [
                    "10",
                    "11",
		    "12",
		    "13",
		    "14"

                ]
            },
            {
                "type": "redis",
                "versions": [
                    "5.0",
                    "6.0",
                    "6.2"
                ]
            }
        ]
    }
    """

  Scenario: Cluster types list by cloud works
    Given default headers
    When we GET "/mdb/1.0/console/cloud-clusters:types?cloudId=cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "clusterTypes": [
            {
                "type": "clickhouse",
                "versions": [
                    "21.1",
                    "21.2",
                    "21.3",
                    "21.4",
                    "21.7",
                    "21.8",
                    "21.11",
                    "22.1",
                    "22.3",
                    "22.5"
                ]
            },
            {
                "type": "hadoop",
                "versions": [
                    "2.0",
                    "1.4",
                    "1.3"
                ]
            },
            {
                "type": "mongodb",
                "versions": [
                    "3.6",
                    "4.0",
                    "4.2",
                    "4.4",
                    "5.0"
                ]
            },
            {
                "type": "mysql",
                "versions": [
                    "5.7"
                ]
            },
            {
                "type": "postgresql",
                "versions": [
                    "10",
                    "11",
                    "12",
                    "13",
                    "14"
                ]
            },
            {
                "type": "redis",
                "versions": [
                    "5.0",
                    "6.0"
                ]
            }
        ]
    }
    """
    Given feature flags
    """
    ["MDB_POSTGRESQL_11", "MDB_MYSQL_8_0", "MDB_DATAPROC_IMAGE_1_3", "MDB_REDIS_62"]
    """
    When we GET "/mdb/1.0/console/cloud-clusters:types?cloudId=cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "clusterTypes": [
            {
                "type": "clickhouse",
                "versions": [
                    "21.1",
                    "21.2",
                    "21.3",
                    "21.4",
                    "21.7",
                    "21.8",
                    "21.11",
                    "22.1",
                    "22.3",
                    "22.5"
                ]
            },
            {
                "type": "hadoop",
                "versions": [
                    "2.0",
                    "1.4",
                    "1.3"
                ]
            },
            {
                "type": "mongodb",
                "versions": [
                    "3.6",
                    "4.0",
                    "4.2",
                    "4.4",
                    "5.0"
                ]
            },
            {
                "type": "mysql",
                "versions": [
                    "5.7",
                    "8.0"
                ]
            },
            {
                "type": "postgresql",
                "versions": [
                    "10",
                    "11",
                    "12",
                    "13",
                    "14"
                ]
            },
            {
                "type": "redis",
                "versions": [
                    "5.0",
                    "6.0",
                    "6.2"
                ]
            }
        ]
    }
    """

  Scenario Outline: Adding quota via support handle works
    Given default headers
    When we POST "/mdb/1.0/support/quota/cloud1/<type>" with data
    """
    {
        "action": "add",
        <extra>
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        <expected>
    }
    """
    Examples:
      | type      | extra                                  | expected              |
      | clusters  | "clustersQuota": 1                     | "clustersQuota": 1    |
      | resources | "count": 1, "presetId": "s1.compute.1" | "cpuQuota": 1         |
      | ssd_space | "ssdSpaceQuota": 1024                  | "ssdSpaceQuota": 1024 |
      | hdd_space | "hddSpaceQuota": 2048                  | "hddSpaceQuota": 2048 |

  Scenario Outline: Substracting quota via support handle works
    Given default headers
    When we POST "/mdb/1.0/support/quota/cloud1/<type>" with data
    """
    {
        "action": "add",
        <extra>
    }
    """
    And we POST "/mdb/1.0/support/quota/cloud1/<type>" with data
    """
    {
        "action": "sub",
        <extra>
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        <expected>
    }
    """
    Examples:
      | type      | extra                                  | expected              |
      | clusters  | "clustersQuota": 1                     | "clustersQuota": 0    |
      | resources | "count": 1, "presetId": "s1.compute.1" | "cpuQuota": 0         |
      | ssd_space | "ssdSpaceQuota": 1024                  | "ssdSpaceQuota": 0    |
      | hdd_space | "hddSpaceQuota": 2048                  | "hddSpaceQuota": 0    |

  Scenario: Handler for getting information about platform works
    When we "GetPlatforms" via gRPC at "yandex.cloud.priv.mdb.v1.console.CUDService"
    Then we get gRPC response with body
    """
    {
        "platforms": [
            {
            "platform_id": "mdb-v1",
            "description": "Intel Broadwell",
            "generation": "1"
            },
            {
            "platform_id": "mdb-v2",
            "description": "Intel Cascade Lake",
            "generation": "2"
            },
            {
            "platform_id": "mdb-v3",
            "description": "Intel Ice Lake",
            "generation": "3"
            }
        ]
    }
    """

  Scenario: Read only mode does not affect read methods
    When we create flag "read-only"
    And we "GetPlatforms" via gRPC at "yandex.cloud.priv.mdb.v1.console.CUDService"
    Then we get gRPC response with body
    """
    {
        "platforms": [
            {
            "platform_id": "mdb-v1",
            "description": "Intel Broadwell",
            "generation": "1"
            },
            {
            "platform_id": "mdb-v2",
            "description": "Intel Cascade Lake",
            "generation": "2"
            },
            {
            "platform_id": "mdb-v3",
            "description": "Intel Ice Lake",
            "generation": "3"
            }
        ]
    }
    """

  Scenario: Handler for getting used resources handles errors
    When we "GetUsedResources" via gRPC at "yandex.cloud.priv.mdb.v1.console.ClusterService" with data
    """
    {
        "cloud_ids": []
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "clouds list should not be empty"

    When we "GetUsedResources" via gRPC at "yandex.cloud.priv.mdb.v1.console.ClusterService" with data
    """
    {
        "cloud_ids": ["cloud1"]
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "used resources for specified clouds not found"

    When we "GetUsedResources" via gRPC at "yandex.cloud.priv.mdb.v1.console.ClusterService" with data
    """
    {
        "cloud_ids": ["cloud1", "cloud2", "cloud4"]
    }
    """
    Then we get gRPC response error with code PERMISSION_DENIED and message "authorization failed"

  Scenario: Handler for getting used resources works
    Given default headers
    And "create_clickhouse" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
            "serviceAccountId": null,
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
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
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    And "create_postgresql" data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt",
           "priority": 9,
           "configSpec": {
                "postgresqlConfig_14": {
                    "workMem": 65536
                }
           }
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster",
       "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with "create_clickhouse" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create ClickHouse cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create_postgresql" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid2"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we "GetUsedResources" via gRPC at "yandex.cloud.priv.mdb.v1.console.ClusterService" with data
    """
    {
        "cloud_ids": ["cloud1"]
    }
    """
    Then we get gRPC response with body
    """
    {
        "resources": [
            {
                "cloud_id": "cloud1",
                "cluster_type": "POSTGRESQL",
                "role": "postgresql_cluster",
                "platform_id": "mdb-v1",
                "cores": "3",
                "memory": "12884901888"
            },
            {
                "cloud_id": "cloud1",
                "cluster_type": "CLICKHOUSE",
                "role": "clickhouse_cluster",
                "platform_id": "mdb-v1",
                "cores": "2",
                "memory": "8589934592"
            }
        ]
    }
    """

  Scenario: Handler for getting billing metrics works
    When we "GetBillingMetrics" via gRPC at "yandex.cloud.priv.mdb.v1.console.CUDService" with data
    """
    {
        "cluster_type": "MONGODB",
        "platform_id": "mdb-v1",
        "cores": "1000",
        "memory": "4294967296000"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "schema": "mdb.db.generic.v1",
        "tags": {
            "cluster_type": "mongodb_cluster",
            "roles": [
                "mongodb_cluster.mongod"
            ],
            "platform_id": "mdb-v1",
            "cores": "1000",
            "core_fraction": "100",
            "memory": "4294967296000",
            "online": "1"
        }
    }
    """
