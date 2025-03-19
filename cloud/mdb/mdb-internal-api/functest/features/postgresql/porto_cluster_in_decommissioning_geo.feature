@decommission @geo
Feature: Create Porto PostgreSQL Cluster with host in decommissioning geo

  Background:
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "resources": {
               "resourcePresetId": "s1.porto.2",
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
           "zoneId": "myt"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And "iva-1.db.yandex.net" change fqdn to "ugr-1.db.yandex.net" and geo to "ugr"

  Scenario: Scale up resourcePreset is forbidden
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
           "resources": {
               "resourcePresetId": "s1.porto.3"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "No new resources could be created in zone 'ugr'"
    }
    """

  Scenario: Scale up resourcePreset is allowed if feature flag present
    Given feature flags
    """
    ["MDB_ALLOW_DECOMMISSIONED_ZONE_USE"]
    """
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
           "resources": {
               "resourcePresetId": "s1.porto.3"
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """

  Scenario: Scale down resourcePreset is ok
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
           "resources": {
               "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """

  Scenario: Scale up diskSize is ok
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
           "resources": {
               "diskSize": 21474836480
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """

  Scenario: Deleting host works for host from dismmissing geo
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["ugr-1.db.yandex.net"]
    }
    """
    Then we get response with status 200

  Scenario: Deleting host works for host from avaliable geo
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    Then we get response with status 200

  Scenario: Creating host in available zone
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
