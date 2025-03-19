Feature: Deletion Protection
    Background:
        Given default headers

        When we add default feature flag "MDB_SQLSERVER_CLUSTER"
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
          "folderId": "folder1",
          "environment": "PRESTABLE",
          "name": "test",
          "description": "test cluster",
          "labels": {
            "foo": "bar"
          },
          "networkId": "network1",
          "sqlcollation": "Cyrillic_General_CI_AI",
          "config_spec": {
            "version": "2016sp2ent",
            "sqlserver_config_2016sp2ent": {
              "maxDegreeOfParallelism": 30,
              "auditLevel": 3
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
        When "worker_task_id1" acquired and finished by worker
        When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "id": "cid1",
            "deletion_protection": false
        }
        """

    Scenario: delete_protection works with go-api
        #
        # ENABLE deletion_protection
        #
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1",
          "update_mask": {
            "paths": [
                "deletion_protection"
            ]
          },
          "deletion_protection": true
        }
        """
        Then we get gRPC response OK

        When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "id": "cid1",
            "deletion_protection": true
        }
        """
        #
        # TEST deletion_protection
        #
        When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
        Then we get gRPC response error with code FAILED_PRECONDITION and message "The operation was rejected because cluster has 'deletion_protection' = ON"
        #
        # DISABLE deletion_protection
        #
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1",
          "update_mask": {
            "paths": [
                "deletion_protection"
            ]
          },
          "deletion_protection": false
        }
        """
        Then we get gRPC response OK
        """
        {
            "deletion_protection": false
        }
        """
        #
        # TEST deletion_protection
        #
        When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
        Then we get gRPC response OK


    Scenario: resource_reaper can overcome delete_protection in go-api
        #
        # ENABLE deletion_protection
        #
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1",
          "update_mask": {
            "paths": [
                "deletion_protection"
            ]
          },
          "deletion_protection": true
        }
        """
        Then we get gRPC response OK

        When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "id": "cid1",
            "deletion_protection": true
        }
        """
        #
        # TEST deletion_protection
        #
        When default headers with "resource-reaper-service-token" token
        When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
        Then we get gRPC response OK
