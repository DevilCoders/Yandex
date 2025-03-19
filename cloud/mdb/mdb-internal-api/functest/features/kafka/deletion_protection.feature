Feature: Deletion Protection
    Background:
        Given default headers
        And we add default feature flag "MDB_KAFKA_CLUSTER"

        And we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
        """
        {
          "folderId": "folder1",
          "environment": "PRESTABLE",
          "name": "testName",
          "description": "test cluster description",
          "networkId": "network1",
          "configSpec": {
            "brokersCount": 1,
            "zoneId": ["myt", "sas"],
            "kafka": {
              "resources": {
                "resourcePresetId": "s2.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 32212254720
              }
            },
            "zookeeper": {
              "resources": {
                "resourcePresetId": "s2.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 10737418240
              }
            }
          },
          "topic_specs": [
            {
              "name": "topic1",
              "partitions": 12,
              "replication_factor": 1
            }],
          "user_specs": [
            {
              "name": "producer",
              "password": "ProducerPassword"
            }]
        }
        """
        Then we get gRPC response with body
        """
        {
          "done": false,
          "id": "worker_task_id1",
          "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
          }
        }
        """
        When "worker_task_id1" acquired and finished by worker
        When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body ignoring empty
        """
        {
          "config": {
            "access": {},
            "brokers_count": "1",
            "version": "3.0",
            "zone_id": ["myt", "sas"],
            "kafka": {
              "resources": {
                "resource_preset_id": "s2.compute.1",
                "disk_size": "32212254720",
                "disk_type_id": "network-ssd"
              }
            },
            "zookeeper": {
              "resources": {
                "resource_preset_id": "s2.compute.1",
                "disk_type_id": "network-ssd",
                "disk_size": "10737418240"
              }
            }
          }
        }
        """

    Scenario: deletion_protection works
        #
        # ENABLE deletion_protection
        #
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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

        When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
        When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
        Then we get gRPC response error with code FAILED_PRECONDITION and message "The operation was rejected because cluster has 'deletion_protection' = ON"
        #
        # DISABLE deletion_protection
        #
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
        When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
        #
        # TEST deletion_protection
        #
        When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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

        When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
        When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
        Then we get gRPC response OK
