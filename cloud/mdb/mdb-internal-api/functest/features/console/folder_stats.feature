Feature: Folder stats
  Background: Create Postgres
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "testPostgre",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "10",
           "postgresqlConfig_10": {},
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
       }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "id": "worker_task_id1"
    }
    """

  Scenario: Clusters count
    When we "GetFolderStats" via gRPC at "yandex.cloud.priv.mdb.v1.console.ClusterService" with data
    """
    {
      "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "cluster_stats": [{
        "cluster_type": "POSTGRESQL",
        "clusters_count": "1"
      }]
    }
    """
