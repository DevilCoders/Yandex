@sqlserver
@grpc_api
Feature: Get system metrics

  Background:
    Given default headers
    And health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "cid": "cid2",
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
                        "name": "sqlserver",
                        "role": "Master",
                        "status": "Alive",
                        "timestamp": 1600860350
                    }
                ],
                "system": {
                    "cpu": {
                        "timestamp": 1600860283,
                        "used": 0.81
                    },
                    "mem": {
                        "timestamp": 1600860283,
                        "used": 9182744,
                        "total": 10000000
                    },
                    "disk": {
                        "timestamp": 1600860111,
                        "used": 918274638,
                        "total": 2000000000
                    }
               }
           },
           {
               "fqdn": "myt-2.db.yandex.net",
               "cid": "cid2",
               "status": "Alive",
               "services": [
                    {
                        "name": "sqlserver",
                        "role": "Master",
                        "status": "Alive",
                        "timestamp": 1600860350
                    }
                ]
           }
       ]
    }
    """
    When we add default feature flag "MDB_SQLSERVER_CLUSTER"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "labels": {
        "foo": "bar"
      },
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
          "maxDegreeOfParallelism": 30,
          "auditLevel": 3
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
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateClusterMetadata",
        "cluster_id": "cid1"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: Host list works
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "SQLSERVER"
                    }
                ],
                "role": "MASTER",
                "subnet_id": "network1-myt",
                "zone_id": "myt",
                "system": {
                    "cpu": {
                        "timestamp": "1600860283",
                        "used": 0.81
                    },
                    "memory": {
                        "timestamp": "1600860283",
                        "used": "9182744",
                        "total": "10000000"
                    },
                    "disk": {
                        "timestamp": "1600860111",
                        "used": "918274638",
                        "total": "2000000000"
                    }
               }
            }
        ]
    }
    """
