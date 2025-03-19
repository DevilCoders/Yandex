Feature: Replication topology

  Background:
    Given default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
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
                       "name": "mysql",
                       "role": "Master",
                       "status": "Alive",
                       "replica_upstream": "",
                       "replica_lag": 0
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mysql",
                       "role": "Replica",
                       "status": "Alive",
                       "replicatype": "Quorum",
                       "replica_upstream": "iva-1.db.yandex.net",
                       "replica_lag": 1
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mysql",
                       "role": "Replica",
                       "status": "Alive",
                       "replicatype": "Async",
                       "replica_upstream": "myt-1.db.yandex.net",
                       "replica_lag": 3
                   }
               ]
           }
       ]
    }
    """
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create MySQL cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker

  @hosts @list
  Scenario: Replication topology works
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 0,
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 0,
                "role": "REPLICA",
                "replicaType": "QUORUM",
                "replicaUpstream": "iva-1.db.yandex.net",
                "replicaLag": 1,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 0,
                "role": "REPLICA",
                "replicaType": "ASYNC",
                "replicaUpstream": "myt-1.db.yandex.net",
                "replicaLag": 3,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
            }
        ]
    }
    """
