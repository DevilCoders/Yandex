@events
Feature: Common event fields

  Background: Use PostgreSQL cluster as example
    Given "create" data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "resources": {
               "resourcePresetId": "s1.compute.1",
               "diskTypeId": "network-ssd",
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
           "zoneId": "myt",
           "subnentId": "network1-myt",
            "assignPublicIp": true
       }],
       "networkId": "network1"
    }
    """

  Scenario: Authentication and authorization messages
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id1" exists "yandex.cloud.events.mdb.postgresql.CreateCluster" event with
    """
    {
        "authentication": {
            "authenticated": true,
            "subject_id": "user",
            "subject_type": "YANDEX_PASSPORT_USER_ACCOUNT"
        },
        "authorization": {
            "authorized": true,
            "permissions": [
                {
                    "permission": "mdb.all.create",
                    "resource_type": "resource-manager.folder",
                    "resource_id": "folder1",
                    "authorized": true
                },
                {
                    "permission": "vpc.subnets.use",
                    "resource_type": "resource-manager.folder",
                    "resource_id": "folder1",
                    "authorized": true
                },
                {
                    "permission": "vpc.addresses.createExternal",
                    "resource_type": "resource-manager.folder",
                    "resource_id": "folder1",
                    "authorized": true
                }
            ]
        }
    }
    """

  Scenario: Request with X-Request-Id and Idempotence-Key
    Given headers
    """
    {
       "X-YaCloud-SubjectToken": "rw-token",
       "Accept": "application/json",
       "Content-Type": "application/json",
       "X-Request-Id": "My-Test-Request-Id",
       "Idempotency-Key": "00000000-0000-0000-0000-000000000001"
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    And for "worker_task_id1" exists "yandex.cloud.events.mdb.postgresql.CreateCluster" event with
    """
    {
        "request_metadata": {
            "request_id": "My-Test-Request-Id",
            "user_agent": "",
            "remote_address": "",
            "idempotency_id": "00000000-0000-0000-0000-000000000001"
        }
    }
    """

  Scenario: Request without X-Request-Id and Idempotence-Key
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    And for "worker_task_id1" exists "yandex.cloud.events.mdb.postgresql.CreateCluster" event with
    """
    {
        "request_metadata": {
            "request_id": "",
            "user_agent": "",
            "remote_address": ""
        }
    }
    """

  @MDB-11775
  Scenario: Request with X-Forwarded-* headers
    Given headers
    """
    {
       "X-YaCloud-SubjectToken": "rw-token",
       "Accept": "application/json",
       "Content-Type": "application/json",
       "X-Forwarded-For": "2001:0DB8::/32",
       "X-Forwarded-Agent": "Functional tests"
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    And for "worker_task_id1" exists "yandex.cloud.events.mdb.postgresql.CreateCluster" event with
    """
    {
        "request_metadata": {
            "request_id": "",
            "user_agent": "Functional tests",
            "remote_address": "2001:0DB8::/32"
        }
    }
    """

  Scenario: Event matches schema
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    And for "worker_task_id1" exists event
    And that event matches "schemas/event.json" schema

  @delete
  Scenario: Events created only for 'base' delete tasks
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    When "worker_task_id1" acquired and finished by worker
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.DeleteCluster" event
    And in worker_queue exists "postgresql_cluster_delete_metadata" with "worker_task_id3" id
    But for "worker_task_id3" there are no events
    And in worker_queue exists "postgresql_cluster_purge" with "worker_task_id4" id
    But for "worker_task_id4" there are no events

  Scenario: Common move cluster logic
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    When "worker_task_id1" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1:move" with data
    """
    {
        "destinationFolderId": "folder2"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "description": "Move PostgreSQL cluster",
        "id": "worker_task_id2"
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.MoveCluster" event with
    """
    {
        "authorization": {
            "authorized": true,
            "permissions": [
                {
                    "permission": "mdb.all.modify",
                    "resource_type": "resource-manager.folder",
                    "resource_id": "folder1",
                    "authorized": true
                },
                {
                    "permission": "mdb.all.create",
                    "resource_type": "resource-manager.folder",
                    "resource_id": "folder2",
                    "authorized": true
                }
            ]
        }
    }
    """
    When we GET "/mdb/1.0/operations/worker_task_id3"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Move PostgreSQL cluster",
        "id": "worker_task_id3"
    }
    """
    But for "worker_task_id3" there are no events
