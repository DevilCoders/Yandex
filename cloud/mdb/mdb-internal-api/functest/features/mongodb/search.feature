@search
Feature: MongoDB search API

  Scenario: Verify search renders
    Given default headers
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "testMongoDBCluster",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_5_0": {
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
            "name": "testdbMongoDB"
        }],
        "userSpecs": [{
            "name": "testMongoUser",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
    Then we get response with status 200
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "timestamp": "<TIMESTAMP>",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testMongoDBCluster",
        "service": "managed-mongodb",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testMongoDBCluster",
            "description": "",
            "labels": {},
            "users": ["testMongoUser"],
            "databases": ["testdbMongoDB"],
            "hosts": ["myt-1.db.yandex.net"]
        }
    }
    """
