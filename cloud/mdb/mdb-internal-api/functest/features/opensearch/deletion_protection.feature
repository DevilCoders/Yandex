@opensearch
Feature: Deletion Protection
    Background:
        Given default headers
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
        """
        {
            "folder_id": "folder1",
            "name": "test",
            "environment": "PRESTABLE",
            "config_spec": {
                "version": "7.14",
                "admin_password": "admin_password",
                "opensearch_spec": {
                    "data_node": {
                        "resources": {
                            "resource_preset_id": "s1.porto.1",
                            "disk_type_id": "local-ssd",
                            "disk_size": 10737418240
                        }
                    },
                    "master_node": {
                        "resources": {
                            "resource_preset_id": "s1.porto.1",
                            "disk_type_id": "local-ssd",
                            "disk_size": 10737418240
                        }
                    }
                }
            },
            "host_specs": [{
                "zone_id": "myt",
                "type": "DATA_NODE"
            }, {
                "zone_id": "sas",
                "type": "DATA_NODE"
            }, {
                "zone_id": "man",
                "type": "DATA_NODE"
            }, {
                "zone_id": "myt",
                "type": "MASTER_NODE"
            }, {
                "zone_id": "sas",
                "type": "MASTER_NODE"
            }, {
                "zone_id": "man",
                "type": "MASTER_NODE"
            }],
            "description": "test cluster",
            "labels": {
                "foo": "bar"
            },
            "networkId": "IN-PORTO-NO-NETWORK-API"
        }
        """
        Then we get gRPC response with body
        """
        {
          "done": false,
          "id": "worker_task_id1",
          "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
          }
        }
        """
        When "worker_task_id1" acquired and finished by worker
        When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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

        When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
        When we "Delete" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
        Then we get gRPC response error with code FAILED_PRECONDITION and message "The operation was rejected because cluster has 'deletion_protection' = ON"
        #
        # DISABLE deletion_protection
        #
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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

        #
        # TEST deletion_protection
        #
        When we "Delete" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1",
          "update_mask": {
            "paths": [
                "deletion_protection"
            ]
          },
          "deletionProtection": true
        }
        """
        Then we get gRPC response OK

        When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
        When we "Delete" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
        Then we get gRPC response OK

