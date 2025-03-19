@disk_types
Feature: network-ssd-nonreplicated disk_type

  Background: Use PostgreSQL cluster as example
    Given default headers
    And "create" data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "resources": {
               "resourcePresetId": "s1.compute.1",
               "diskTypeId": "network-ssd-nonreplicated",
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
           "zoneId": "myt"
       }],
       "networkId": "network1"
    }
    """

  Scenario: Creating cluster without feature flag
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "resourcePreset 's1.compute.1' and diskTypeId 'network-ssd-nonreplicated' are not available"
    }
    """

  Scenario: Resize cluster without feature flag
    Given feature flags
    """
    ["MDB_ALLOW_NETWORK_SSD_NONREPLICATED"]
    """
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 21474836480
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Requested feature is not available"
    }
    """

  Scenario: Resize with feature flag
    Given feature flags
    """
    ["MDB_ALLOW_NETWORK_SSD_NONREPLICATED", "MDB_ALLOW_NETWORK_SSD_NONREPLICATED_RESIZE"]
    """
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 21474836480
            }
        }
    }
    """
    Then we get response with status 200
