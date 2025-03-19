Feature: Validate adding and removing ZooKeeper hosts

  Background:
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
               "fqdn": "man-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "man-2.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "vla-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "vla-2.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "Alive"
                   }
               ]
           },
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
           },
           {
               "fqdn": "sas-1.db.yandex.net",
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
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
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
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with "create" data
    And "worker_task_id1" acquired and finished by worker

  Scenario: Adding zookeeper host works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-2.db.yandex.net"
            ]
        }
    }
    """
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add hosts to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.AddClusterHostsMetadata",
            "cluster_id": "cid1",
            "host_names": ["man-2.db.yandex.net"]
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
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
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
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
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "vla"
            }
        ]
    }
    """

  Scenario: Deleting and adding Zookeeper host works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["vla-1.db.yandex.net"]
    }
    """
    Then we get response with status 200
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["vla-1.db.yandex.net"]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
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
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
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
                "zoneId": "sas"
            }
        ]
    }
    """

  Scenario: Crossing the lower bound on hosts fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["vla-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Zookeeper subcluster with resource preset 's1.porto.1' and disk type 'local-ssd' requires at least 3 hosts"
    }
    """
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["vla-1.db.yandex.net"]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "shard ZooKeeper with resource preset "s1.porto.1" and disk type "local-ssd" requires at least 3 hosts"

  Scenario: Crossing the upper bound on hosts fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }]
    }
    """
    Then we get response with status 200
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }]
    }
    """
    Then we get gRPC response OK
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Zookeeper subcluster with resource preset 's1.porto.1' and disk type 'local-ssd' allows at most 5 hosts"
    }
    """
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "shard ZooKeeper with resource preset "s1.porto.1" and disk type "local-ssd" allows at most 5 hosts"
