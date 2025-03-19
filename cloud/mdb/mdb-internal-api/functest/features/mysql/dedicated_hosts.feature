Feature: MySQL Cluster on dedicated hosts

  Background:
    Given default headers
    And we add default feature flag "MDB_DEDICATED_HOSTS"

  Scenario: Create MySQL cluster on dedicated hosts
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
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
        "hostGroupIds": ["hg_id1", "hg_id2"],
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
            "zoneId": "vla"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create MySQL cluster",
        "done": false,
        "id": "worker_task_id1"
    }
    """
    Then "worker_task_id1" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "hostGroupIds": ["hg_id1", "hg_id2"]
    }
    """


  Scenario: Host Group cpu_limit/ram validation works for MySQL cluster
    When we POST "/mdb/v2/quota" with data
    """
    {
      "cloud_id": "cloud1",
      "metrics":[
        {
          "name": "mdb.cpu.count",
          "limit": 300
        },
        {
          "name": "mdb.memory.size",
          "limit": 412316860416
        }
      ]
    }
    """
    Then we get response with status 200
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.5",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            }
        },
        "hostGroupIds": ["hg_id1", "hg_id2"],
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
            "zoneId": "vla"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Cannot use host group hg_id1 with this resource preset. There are up to 4294967296 bytes of memory in host group hg_id1. Required memory size is 68719476736"
    }
    """

  Scenario: Host Group clouds validation works for MySQL cluster
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
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
        "hostGroupIds": ["hg_id1", "hg_id3"],
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
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Host group hg_id3 belongs to the cloud distinct from cluster's cloud"
    }
    """