@grpc_api
Feature: MongoDB 4.4 Cluster Hosts operations

  Background:
    Given default headers
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
    Given health response
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
           }
       ]
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "health": "ALIVE",
        "status": "RUNNING"
    }
    """


  Scenario: Resetup of MongoDB 4.4 cluster works
    When we "ResetupHosts" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["myt-1.db.yandex.net"]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Resetup given hosts",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.mongodb.v1.ResetupHostsMetadata",
            "cluster_id": "cid1",
            "host_names": ["myt-1.db.yandex.net"]
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

  Scenario: Stepdown of MongoDB 4.4 cluster works
    When we "StepdownHosts" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["myt-1.db.yandex.net"]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Stepdown given hosts",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.mongodb.v1.StepdownHostsMetadata",
            "cluster_id": "cid1",
            "host_names": ["myt-1.db.yandex.net"]
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

  Scenario: Restart of MongoDB 4.4 cluster works
    When we "RestartHosts" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["myt-1.db.yandex.net"]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Restart given hosts",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.mongodb.v1.RestartHostsMetadata",
            "cluster_id": "cid1",
            "host_names": ["myt-1.db.yandex.net"]
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

