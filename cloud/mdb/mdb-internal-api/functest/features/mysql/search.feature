@search
Feature: MySQL search API

  Scenario: Verify search renders
    Given default headers
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "testMysql",
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
            "name": "testMysqlDB"
        }],
        "userSpecs": [{
            "name": "testMysqlUser",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt"
        }],
        "description": "test mysql cluster"
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
        "name": "testMysql",
        "service": "managed-mysql",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testMysql",
            "description": "test mysql cluster",
            "labels": {},
            "users": ["testMysqlUser"],
            "databases": ["testMysqlDB"],
            "hosts": ["myt-1.db.yandex.net"]
        }
    }
    """
