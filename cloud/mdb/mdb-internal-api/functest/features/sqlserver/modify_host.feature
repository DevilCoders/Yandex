@sqlserver
@grpc_api
Feature: Modify Compute Microsoft SQLServer Cluster host

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
                        "name": "sqlserver",
                        "role": "Master",
                        "status": "Alive",
                        "timestamp": 1600860350
                    }
                ]
           }
       ]
    }
    """
    When we add default feature flag "MDB_SQLSERVER_CLUSTER"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateClusterMetadata",
        "cluster_id": "cid1"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id1" acquired and finished by worker


Scenario: Modify assign_public_ip works
        When we "UpdateHosts" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            "update_host_specs": [{
              "host_name": "myt-1.db.yandex.net",
              "assign_public_ip": true
            }]
        }
        """
        Then we get gRPC response with body
        """
        {
            "description": "Update SQL Server cluster host",
            "done": false,
            "id": "worker_task_id2",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateClusterHostsMetadata",
                "cluster_id": "cid1",
                "host_names": [
                    "myt-1.db.yandex.net"
                ]
            }
        }
        """
        And "worker_task_id2" acquired and finished by worker
        And we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        And gRPC response body at path "$.hosts[0]" contains
        """
        {
          "name": "myt-1.db.yandex.net",
          "assign_public_ip": true
        }
        """
    
Scenario: Modify assign_public_ip with wrong FQDN fails
        When we "UpdateHosts" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            "update_host_specs": [{
              "host_name": "xyz-1.db.yandex.net",
              "assign_public_ip": true
            }]
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "there is no host with such FQDN: xyz-1.db.yandex.net"

Scenario: Modify host without any changes fails
        When we "UpdateHosts" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            "update_host_specs": [{
              "host_name": "myt-1.db.yandex.net",
              "assign_public_ip": false
            }]
        }
        """
        Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"
