Feature: Common errors expose logic

  Background: Use PostgreSQL cluster as example
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "resources": {
               "resourcePresetId": "s1.porto.1",
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
       }]
    }
    """

  Scenario: Exposable errors are shown
    When we run query
    """
    SELECT code.acquire_task('dummy', 'worker_task_id1')
    """
    And we run query
    """
    SELECT code.finish_task('dummy', 'worker_task_id1', false, '{}'::jsonb, '', '[{"exposable": true, "code": 1, "type": "TestError", "message": "Test error"}]')
    """
    And we "GET" via REST at "/mdb/1.0/operations/worker_task_id1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create PostgreSQL cluster",
        "done": true,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        },
        "error": {
            "code": 1,
            "details": [
                {
                    "code": 1,
                    "message": "Test error",
                    "type": "TestError"
                }
            ],
            "message": "Test error"
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.v1.OperationService" with data
    """
    {
        "operation_id" : "worker_task_id1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create PostgreSQL cluster",
        "done": true,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.postgresql.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        },
        "error": {
            "code": 1,
            "details": [
                {
                    "@type":   "type.googleapis.com/google.rpc.Status",
                    "code": 1,
                    "message": "Test error",
                    "details": []
                }
            ],
            "message": "Test error"
        }
    }
    """

  Scenario: Non-Exposable errors are hidden for non-support users
    Given default headers with "ro-token" token
    When we run query
    """
    SELECT code.acquire_task('dummy', 'worker_task_id1')
    """
    And we run query
    """
    SELECT code.finish_task('dummy', 'worker_task_id1', false, '{}'::jsonb, '', '[{"exposable": false, "code": 1, "type": "TestError", "message": "Test error"}]')
    """
    And we "GET" via REST at "/mdb/1.0/operations/worker_task_id1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create PostgreSQL cluster",
        "done": true,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        },
        "error": {
            "code": 2,
            "details": [],
            "message": "Unknown error"
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.v1.OperationService" with data
    """
    {
        "operation_id" : "worker_task_id1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create PostgreSQL cluster",
        "done": true,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.postgresql.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        },
        "error": {
            "code": 2,
            "details": [],
            "message": "Unknown error"
        }
    }
    """

  Scenario: Non-Exposable errors are shown for support users
    When we run query
    """
    SELECT code.acquire_task('dummy', 'worker_task_id1')
    """
    And we run query
    """
    SELECT code.finish_task('dummy', 'worker_task_id1', false, '{}'::jsonb, '', '[{"exposable": false, "code": 1, "type": "TestError", "message": "Test error"}]')
    """
    And we GET "/mdb/1.0/operations/worker_task_id1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create PostgreSQL cluster",
        "done": true,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        },
        "error": {
            "code": 1,
            "details": [
                {
                    "code": 1,
                    "message": "Test error",
                    "type": "TestError"
                }
            ],
            "message": "Test error"
        }
    }
    """

  Scenario: Non-Exposable errors are shown in support handle
    When we run query
    """
    SELECT code.acquire_task('dummy', 'worker_task_id1')
    """
    And we run query
    """
    SELECT code.finish_task('dummy', 'worker_task_id1', false, '{}'::jsonb, '', '[{"exposable": false, "code": 1, "type": "TestError", "message": "Test error"}]')
    """
    And we GET "/mdb/1.0/support/operations/worker_task_id1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create PostgreSQL cluster",
        "done": true,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        },
        "error": {
            "code": 1,
            "details": [
                {
                    "code": 1,
                    "message": "Test error",
                    "type": "TestError"
                }
            ],
            "message": "Test error"
        }
    }
    """
