Feature: Management of ClickHouse users and/or databases with SQL

  Background:
    Given default headers

  Scenario: Create cluster with SQL user and database management
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "adminPassword": "admin_pass"
        },
        "databaseSpecs": [],
        "userSpecs": [],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "sql_user_management": true,
            "sql_database_management": true,
            "admin_password": "admin_pass"
        },
        "database_specs":[],
        "user_specs": [],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.4",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "access": {
                "webSql": false,
                "dataLens": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "clickhouse": {
                "config": {
                    "defaultConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "effectiveConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "userConfig": {
                        "mergeTree": {}
                    }
                },
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 0,
                    "diskTypeId": null,
                    "resourcePresetId": null
                }
            },
            "cloudStorage": {
                "enabled": false
            }
        }
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse" contains
    """
    {
        "sql_user_management": true,
        "admin_password": {
            "password": {
                "data": "admin_pass",
                "encryption_version": 0
            },
            "hash": {
                "data": "fc618395b55f38756c9a98ec058c267830f84d448ca4e0b893951784b163b617",
                "encryption_version": 0
            }
        },
        "sql_database_management": true,
        "users": {},
        "databases": []
    }
    """
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "adminPassword": "new_password"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then body at path "$.data.clickhouse.admin_password" contains
    """
    {
        "password": {
            "data": "new_password",
            "encryption_version": 0
        },
        "hash": {
            "data": "00b9e6622317a2fb628d5514b866d4e7c52b5b149027825645ae3fd72827e84e",
            "encryption_version": 0
        }
    }
    """

  Scenario: Attempt to create cluster with SQL user management and non-empty user specs fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": true,
            "adminPassword": "admin_pass"
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
        "message": "Users to create cannot be specified when SQL user management is enabled."
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "sql_user_management": true,
            "admin_password": "admin_pass"
        },
        "database_specs":[{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "users to create cannot be specified when SQL user management is enabled"

  Scenario: Attempt to create cluster with SQL database management and non-empty database specs fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "adminPassword": "admin_pass"
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [],
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
        "message": "Databases to create cannot be specified when SQL database management is enabled."
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "sql_user_management": true,
            "sql_database_management": true,
            "admin_password": "admin_pass"
        },
        "database_specs":[{
            "name": "testdb"
        }],
        "user_specs": [],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "databases to create cannot be specified when SQL database management is enabled"

  Scenario: Attempt to creating cluster with SQL database management and without SQL user management fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": false,
            "sqlDatabaseManagement": true,
            "adminPassword": "admin_pass"
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
        "message": "SQL database management is not supported without SQL user management."
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "sql_user_management": false,
            "sql_database_management": true,
            "admin_password": "admin_pass"
        },
        "database_specs":[{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "SQL database management is not supported without SQL user management"

  Scenario: Attempt to create cluster with SQL user management and without admin password fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": true
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [],
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
        "message": "Admin password must be specified in order to enable SQL user management."
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "sql_user_management": true
        },
        "database_specs":[{
            "name": "testdb"
        }],
        "user_specs": [],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "admin password must be specified in order to enable SQL user management"

  Scenario: User API in cluster with SQL user management works as expected
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
            "adminPassword": "admin_pass"
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,clickhouse,sql_user_management}', 'true')
        WHERE subcid = 'subcid1'
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test3",
            "password": "password",
            "permissions": []
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The requested functionality is not available for clusters with enabled SQL user management. Please use SQL interface to manage users."
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test3",
            "password": "password"
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "operation not permitted when SQL user management is enabled."
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test"
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The requested functionality is not available for clusters with enabled SQL user management. Please use SQL interface to manage users."
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "operation not permitted when SQL user management is enabled."
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test" with data
    """
    {
        "password": "changed password"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The requested functionality is not available for clusters with enabled SQL user management. Please use SQL interface to manage users."
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test",
        "password": "changed password"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "operation not permitted when SQL user management is enabled."
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": []
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "users": []
    }
    """
    When we GET "/mdb/clickhouse/1.0/operations/worker_task_id1"
    Then we get response with status 200
    When we GET "/mdb/clickhouse/1.0/operations/worker_task_id2"
    Then we get response with status 200

  Scenario: Database API in cluster with SQL database management works as expected
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
            "adminPassword": "admin_pass"
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb2"
        }
    }
    """
    Then we get response with status 200
    When we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,clickhouse,sql_user_management}', 'true')
        WHERE subcid = 'subcid1'
    """
    When we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,clickhouse,sql_database_management}', 'true')
        WHERE subcid = 'subcid1'
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb3"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The requested functionality is not available for clusters with enabled SQL database management. Please use SQL interface to manage databases."
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_spec": {
            "name": "testdb3"
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "operation not permitted when SQL database management is enabled."
    When we DELETE "/mdb/clickhouse/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The requested functionality is not available for clusters with enabled SQL database management. Please use SQL interface to manage databases."
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": []
    }
    """
    When we GET "/mdb/clickhouse/1.0/operations/worker_task_id1"
    Then we get response with status 200
    When we GET "/mdb/clickhouse/1.0/operations/worker_task_id2"
    Then we get response with status 200

  Scenario: Enable SQL user and database management
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "adminPassword": "admin_pass"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.4",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "access": {
                "webSql": false,
                "dataLens": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "clickhouse": {
                "config": {
                    "defaultConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "effectiveConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "userConfig": {
                        "mergeTree": {}
                    }
                },
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 0,
                    "diskTypeId": null,
                    "resourcePresetId": null
                }
            },
            "cloudStorage": {
                "enabled": false
            }
        }
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then body at path "$.data.clickhouse" contains
    """
    {
        "sql_user_management": true,
        "sql_database_management": true,
        "admin_password": {
            "password": {
                "data": "admin_pass",
                "encryption_version": 0
            },
            "hash": {
                "data": "fc618395b55f38756c9a98ec058c267830f84d448ca4e0b893951784b163b617",
                "encryption_version": 0
            }
        }
    }
    """

  Scenario: Enable SQL user management without SQL database management
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "sqlUserManagement": true,
            "adminPassword": "admin_pass"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.4",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": true,
            "sqlDatabaseManagement": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "clickhouse": {
                "config": {
                    "defaultConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "effectiveConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "userConfig": {
                        "mergeTree": {}
                    }
                },
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 0,
                    "diskTypeId": null,
                    "resourcePresetId": null
                }
            },
            "cloudStorage": {
                "enabled": false
            }
        }
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then body at path "$.data.clickhouse" contains
    """
    {
        "sql_user_management": true,
        "sql_database_management": false,
        "admin_password": {
            "password": {
                "data": "admin_pass",
                "encryption_version": 0
            },
            "hash": {
                "data": "fc618395b55f38756c9a98ec058c267830f84d448ca4e0b893951784b163b617",
                "encryption_version": 0
            }
        }
    }
    """

  Scenario: Attempt to disable SQL user and database management fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "adminPassword": "admin_pass"
        },
        "databaseSpecs": [],
        "userSpecs": [],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "sqlUserManagement": false
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "SQL user management cannot be disabled after it has been enabled."
    }
    """
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "sqlDatabaseManagement": false
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "SQL database management cannot be disabled after it has been enabled."
    }
    """



    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "adminPassword": "admin_pass"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.4",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "access": {
                "webSql": false,
                "dataLens": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "clickhouse": {
                "config": {
                    "defaultConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "effectiveConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "userConfig": {
                        "mergeTree": {}
                    }
                },
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 0,
                    "diskTypeId": null,
                    "resourcePresetId": null
                }
            },
            "cloudStorage": {
                "enabled": false
            }
        }
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then body at path "$.data.clickhouse" contains
    """
    {
        "sql_user_management": true,
        "sql_database_management": true,
        "admin_password": {
            "password": {
                "data": "admin_pass",
                "encryption_version": 0
            },
            "hash": {
                "data": "fc618395b55f38756c9a98ec058c267830f84d448ca4e0b893951784b163b617",
                "encryption_version": 0
            }
        }
    }
    """

  Scenario: Attempt to enable SQL user management without admin password fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "sqlUserManagement": true
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Admin password must be specified in order to enable SQL user management."
    }
    """

  Scenario: Attempt to enable SQL database management without SQL user management fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "sqlDatabaseManagement": true
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "SQL database management is not supported without SQL user management."
    }
    """

  Scenario: Restore cluster with enabling SQL user and database management
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
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
            "zone_id": "myt"
        }]
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "adminPassword": "admin_pass"
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 200
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
            "sql_user_management": true,
            "sql_database_management": true,
            "admin_password": "admin_pass",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }],
        "backup_id": "cid1:0"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/clickhouse/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.4",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "access": {
                "webSql": false,
                "dataLens": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "clickhouse": {
                "config": {
                    "defaultConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "effectiveConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "userConfig": {
                        "mergeTree": {}
                    }
                },
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 0,
                    "diskTypeId": null,
                    "resourcePresetId": null
                }
            },
            "cloudStorage": {
                "enabled": false
            }
        }
    }
    """
    When we GET "/api/v1.0/config/sas-1.db.yandex.net"
    Then body at path "$.data.clickhouse" contains
    """
    {
        "sql_user_management": true,
        "sql_database_management": true,
        "admin_password": {
            "password": {
                "data": "admin_pass",
                "encryption_version": 0
            },
            "hash": {
                "data": "fc618395b55f38756c9a98ec058c267830f84d448ca4e0b893951784b163b617",
                "encryption_version": 0
            }
        }
    }
    """

  Scenario: Attempt to restore cluster without admin password passed leads to use source cluster's one
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    # language=json
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "adminPassword": "admin_pass"
        },
        "databaseSpecs": [],
        "userSpecs": [],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "sql_user_management": true,
            "sql_database_management": true,
            "admin_password": "admin_pass"
        },
        "database_specs":[],
        "user_specs": [],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    Given s3 response
    # language=json
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore" with data
    # language=json
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 200
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
            "sql_user_management": true,
            "sql_database_management": true,
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }],
        "backup_id": "cid1:0"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/clickhouse/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    # language=json
    """
    {
        "config": {
            "version": "21.4",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "access": {
                "webSql": false,
                "dataLens": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "clickhouse": {
                "config": {
                    "defaultConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "effectiveConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "userConfig": {
                        "mergeTree": {}
                    }
                },
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 0,
                    "diskTypeId": null,
                    "resourcePresetId": null
                }
            },
            "cloudStorage": {
                "enabled": false
            }
        }
    }
    """
    When we GET "/api/v1.0/config/sas-1.db.yandex.net"
    Then body at path "$.data.clickhouse" contains
    # language=json
    """
    {
        "sql_user_management": true,
        "sql_database_management": true,
        "admin_password": {
            "password": {
                "data": "admin_pass",
                "encryption_version": 0
            },
            "hash": {
                "data": "fc618395b55f38756c9a98ec058c267830f84d448ca4e0b893951784b163b617",
                "encryption_version": 0
            }
        }
    }
    """

  Scenario: Attempt to restore cluster with SQL management and without admin password on source cluster fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    # language=json
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
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
            "zone_id": "myt"
        }]
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    Given s3 response
    # language=json
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore" with data
    # language=json
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 200
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
            "sql_user_management": true,
            "sql_database_management": true,
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }],
        "backup_id": "cid1:0"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "admin password must be specified in order to enable SQL user management"

  Scenario: Attempt to restore cluster with disabling SQL user management or SQL database management fails
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "adminPassword": "admin_pass"
        },
        "databaseSpecs": [],
        "userSpecs": [],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we POST "/mdb/clickhouse/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlUserManagement": false
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "SQL user management cannot be disabled after it has been enabled."
    }
    """
    When we POST "/mdb/clickhouse/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "sqlDatabaseManagement": false
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "SQL database management cannot be disabled after it has been enabled."
    }
    """
