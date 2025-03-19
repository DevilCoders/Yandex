Feature: Support cluster info search tests

  Background: Use ClickHouse cluster as example
    Given default headers
    And health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "Alive"
           }
       ],
        "hosts": [
            {
                "fqdn": "myt-1.db.yandex.net",
                "cid": "cid1",
                "status": "Alive",
                "services": [
                    {
                        "name": "clickhouse",
                        "status": "Alive"
                    }
                ]
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
        }]
    }
    """
    And we run query
    """
    UPDATE dbaas.hosts set vtype_id = 'vid1'
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario Outline: Search is able to find cluster
    When we GET "/mdb/1.0/support/clusters/search?<query>"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "config": {
            "access": {
                "dataLens": false,
                "webSql": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "nanos": 100,
                "seconds": 30
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
                        "maxPartitionSizeToDrop": 53687091200,
                        "maxTableSizeToDrop": 53687091200,
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "timezone": "Europe/Moscow",
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
                        "maxPartitionSizeToDrop": 53687091200,
                        "maxTableSizeToDrop": 53687091200,
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "timezone": "Europe/Moscow",
                        "uncompressedCacheSize": 8589934592
                    },
                    "userConfig": {
                        "mergeTree": {}
                    }
                },
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                }
            },
            "version": "21.3",
            "serviceAccountId": null,
            "sqlUserManagement": false,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlDatabaseManagement": false,
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
        },
        "description": "",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "instanceId": "vid1",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shardName": "shard1",
                "subnetId": "",
                "type": "CLICKHOUSE",
                "zoneId": "myt"
            }
        ],
        "id": "cid1",
        "labels": {},
        "monitoring": [
            {
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm.yandex-team.ru/template/panel/dbaas_clickhouse_metrics/cid=cid1",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-clickhouse",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/clickhouse_cluster/cid1?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test",
        "networkId": ""
    }
    """
    Examples:
      | query                    |
      | fqdn=myt-1.db.yandex.net |
      | instanceId=vid1          |
      | clusterId=cid1           |
      | subclusterId=subcid1     |
      | shardId=shard_id1        |

  Scenario Outline: Search with invalid params fails
    When we GET "/mdb/1.0/support/clusters/search<query>"
    Then we get response with status <status> and body contains
    """
    {
        "code": <code>,
        "message": "<message>"
    }
    """
    Examples:
      | query                                     | status | code | message                                                                            |
      |                                           | 422    | 3    | No search params                                                                   |
      | ?fqdn=myt-1.db.yandex.net&instanceId=vid1 | 422    | 3    | Multiple search params                                                             |
      | ?fqdn=nonexistent                         | 403    | 7    | You do not have permission to access the requested object or object does not exist |
