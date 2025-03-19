Feature: System metrics

  Background:
    Given feature flags
    """
    ["MDB_MONGODB_ALLOW_DEPRECATED_VERSIONS"]
    """
    And default headers
    And "create" data
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
        "description": "test cluster",
       "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
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
               ],
               "system": {
                   "cpu": {
                       "timestamp": 19292837,
                       "used": 0.45
                   },
                   "mem": {
                       "timestamp": 19292837,
                       "used": 9228372,
                       "total": 10293829
                   }
               }
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
               ],
               "system": {
                   "cpu": {
                       "timestamp": 19292293,
                       "used": 0.98
                   },
                   "mem": {
                       "timestamp": 19292293,
                       "used": 1928293,
                       "total": 10293849
                   },
                   "disk": {
                       "timestamp": 19229389,
                       "used": 19283910,
                       "total": 1110293899
                   }
               }
           }
       ]
    }
    """
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create MongoDB cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker

  @hosts @list
  Scenario: System metrics works
    When we GET "/mdb/mongodb/1.0/clusters/cid1/hosts"
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
                "role": "PRIMARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "iva",
                "system": {
                   "cpu": {
                       "timestamp": 19292837,
                       "used": 0.45
                   },
                   "memory": {
                       "timestamp": 19292837,
                       "used": 9228372,
                       "total": 10293829
                   }
               }
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
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
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
                "role": "SECONDARY",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MONGOD"
                    }
                ],
                "shardName": "rs01",
                "subnetId": "",
                "type": "MONGOD",
                "zoneId": "sas",
                "system": {
                   "cpu": {
                       "timestamp": 19292293,
                       "used": 0.98
                   },
                   "memory": {
                       "timestamp": 19292293,
                       "used": 1928293,
                       "total": 10293849
                   },
                   "disk": {
                       "timestamp": 19229389,
                       "used": 19283910,
                       "total": 1110293899
                   }
               }
            }
        ]
    }
    """
