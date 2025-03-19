Feature: Get system metrics

  Background:
    Given default headers
    And "create" data
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
                       "name": "pgbouncer",
                       "role": "Unknown",
                       "status": "Alive"
                   },
                   {
                       "name": "pg_replication",
                       "role": "Master",
                       "status": "Alive"
                   }
               ],
               "system": {
                   "cpu": {
                       "timestamp": 383747,
                       "used": 0.32
                   },
                   "mem": {
                       "timestamp": 383747,
                       "used": 28273728,
                       "total": 83747200
                   },
                   "disk": {
                       "timestamp": 383938,
                       "used": 382937493,
                       "total": 1838473948
                   }
               }
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "pgbouncer",
                       "role": "Unknown",
                       "status": "Alive"
                   },
                   {
                       "name": "pg_replication",
                       "role": "Replica",
                       "replicatype": "Sync",
                       "status": "Alive"
                   }
               ],
               "system": {
                   "cpu": {
                       "timestamp": 293841,
                       "used": 0.21
                   },
                   "disk": {
                       "timestamp": 304832,
                       "used": 283947672,
                       "total": 1283927880
                   }
               }
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "pgbouncer",
                       "role": "Unknown",
                       "status": "Alive"
                   },
                   {
                       "name": "pg_replication",
                       "role": "Replica",
                       "replicatype": "Async",
                       "status": "Alive"
                   }
               ]
           }
       ]
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker

  Scenario: Host list works
    When we GET "/mdb/postgresql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "priority": 10,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "system": {
                    "cpu": {
                        "timestamp": 383747,
                        "used": 0.32
                    },
                    "memory": {
                        "timestamp": 383747,
                        "used": 28273728,
                        "total": 83747200
                    },
                    "disk": {
                        "timestamp": 383938,
                        "used": 382937493,
                        "total": 1838473948
                    }
                },
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "configSpec": {
                    "postgresqlConfig_14": {
                        "workMem": 65536
                    }
                },
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "priority": 9,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "SYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "system": {
                    "cpu": {
                        "timestamp": 293841,
                        "used": 0.21
                    },
                    "disk": {
                        "timestamp": 304832,
                        "used": 283947672,
                        "total": 1283927880
                    }
                },
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "priority": 5,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "ASYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
             }
         ]
    }
    """
