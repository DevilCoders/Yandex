@search
Feature: PostgreSQL search API

  Scenario: Verify search renders and search_queue docs
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "testName",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test_user"
       }],
       "userSpecs": [{
           "name": "test_user",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "sas"
       }],
       "description": "test cluster description",
       "labels": {
           "color": "blue"
       }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    Then in search_queue there is one document
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-postgresql",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {"color": "blue"},
            "databases": ["testdb"],
            "users": ["test_user"],
            "hosts": ["sas-1.db.yandex.net"]
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "name": "New_cluster_name"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """
    Then in search_queue there are "2" documents
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "New_cluster_name",
        "service": "managed-postgresql",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "New_cluster_name",
            "description": "test cluster description",
            "labels": {"color": "blue"},
            "databases": ["testdb"],
            "users": ["test_user"],
            "hosts": ["sas-1.db.yandex.net"]
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/postgresql/1.0/clusters/cid1:move" with data
    """
    {
        "destinationFolderId": "folder2"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id3"
    }
    """
    Then in search_queue there are "4" documents
    And "3" document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "New_cluster_name",
        "service": "managed-postgresql",
        "deleted": "<TIMESTAMP>",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1"
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder2"
        }],
        "name": "New_cluster_name",
        "service": "managed-postgresql",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud2",
        "folder_id": "folder2",
        "attributes": {
            "name": "New_cluster_name",
            "description": "test cluster description",
            "labels": {"color": "blue"},
            "databases": ["testdb"],
            "users": ["test_user"],
            "hosts": ["sas-1.db.yandex.net"]
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And "worker_task_id4" acquired and finished by worker
    And we DELETE "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id5"
    }
    """
    Then in search_queue there are "5" documents
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder2"
        }],
        "name": "New_cluster_name",
        "service": "managed-postgresql",
        "timestamp": "<TIMESTAMP>",
        "deleted": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud2",
        "folder_id": "folder2"
    }
    """
