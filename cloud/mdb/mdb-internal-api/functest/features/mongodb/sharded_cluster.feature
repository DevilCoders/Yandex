@events
Feature: Sharded MongoDB Cluster

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
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Master",
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
                       "name": "mongod",
                       "role": "Replica",
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
                       "name": "mongod",
                       "role": "Replica",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "man-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongocfg",
                       "role": "Master",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "sas-2.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongocfg",
                       "role": "Replica",
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
                       "name": "mongocfg",
                       "role": "Replica",
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
                       "name": "mongos",
                       "role": "Master",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "sas-3.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongos",
                       "role": "Master",
                       "status": "Alive"
                   }
               ]
           }
       ]
    }
    """
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
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
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Enable sharding for MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.EnableClusterShardingMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.EnableClusterSharding" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "zoneId": "sas",
                "type": "MONGOD"
            }, {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add shard to MongoDB cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.AddClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mongodb.AddClusterShard" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "shard_name": "shard2"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/operations/worker_task_id3"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.mongodb.v1.Shard",
        "clusterId": "cid1",
        "name": "shard2"
    }
    """

  Scenario: Cluster update with one downscale and few noops
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.2"
                    }
                },
                "mongocfg": {
                    "resources": {
                        "diskSize": 42949672960
                    }
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MongoDB cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
          "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id4" acquired and finished by worker
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.2"
                    }
                },
                "mongocfg": {
                    "resources": {
                        "diskSize": 10737418240
                    }
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MongoDB cluster",
        "done": false,
        "id": "worker_task_id5",
        "metadata": {
          "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """

