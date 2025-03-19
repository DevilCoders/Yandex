Feature: Create/MongoDB Cluster with extended storage FF

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
                        "diskSize": 10733223936
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
            "zoneId": "iva"
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
               ]
           }
       ]
    }
    """

  Scenario: Cluster creation with extended storage without feature flag fails
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Invalid disk_size, must be between or equal 10737418240 and 2199023255552"
    }
    """


  Scenario: Cluster creation with extended storage with feature flag works
    And we add default feature flag "MDB_MONGODB_EXTENDEDS"
    And we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with "create" data
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
