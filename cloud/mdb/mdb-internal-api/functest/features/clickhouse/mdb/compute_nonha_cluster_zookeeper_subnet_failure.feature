Feature: Add ZooKeeper to Compute Non-HA ClickHouse Cluster fails

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
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "clickhouse",
                       "status": "Alive"
                   }
               ]
           }
       ]
    }
    """
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
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
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network2"
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: Add ZooKeeper with with host with no subnets
    When we POST "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "resources": {
            "resourcePresetId": "s1.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
        },

        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "ZooKeeper network cannot be configured as there is no subnet in zone 'man'."
    }
    """

  Scenario: Add ZooKeeper with host with not subnet specified while there are multiple subnets in zone
    When we POST "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "resources": {
            "resourcePresetId": "s1.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
        },

        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "ZooKeeper network cannot be configured as there are multiple subnets in zone 'iva'."
    }
    """
