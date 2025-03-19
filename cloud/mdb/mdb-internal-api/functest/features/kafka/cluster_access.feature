@kafka
@grpc_api
@wip
Feature: Kafka Cluster with access settings
  Background:
    Given default headers
    And we add default feature flag "MDB_KAFKA_CLUSTER"

  Scenario: Cluster creation with access settings
    Given health response
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
               "fqdn": "myt-1.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "fqdn": "myt-2.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "fqdn": "sas-1.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "fqdn": "sas-2.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "fqdn": "vla-1.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "fqdn": "vla-2.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           }
       ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
      "configSpec": {
        "access": {
                "web_sql": false,
                "serverless": false,
                "data_transfer": true
        },
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
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "description": "test cluster",
      "labels": {
        "foo": "bar"
      },
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1",
      "status": "RUNNING",
      "health": "ALIVE",
      "config": {
        "access": {
                "web_sql": false,
                "serverless": false,
                "data_transfer": true
            },
        "assign_public_ip": false,
        "brokers_count": "1",
        "version": "3.0",
        "zone_id": ["myt", "sas"],
        "unmanaged_topics": false,
        "sync_topics": false,
        "schema_registry": false,
        "kafka": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "32212254720"
          }
        },
        "zookeeper": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        }
      },
      "monitoring": [
        {
          "name": "YASM",
          "description": "YaSM (Golovan) charts",
          "link": "https://yasm/cid=cid1"
        },
        {
          "name": "Solomon",
          "description": "Solomon charts",
          "link": "https://solomon/cid=cid1&fExtID=folder1"
        },
        {
          "name": "Console",
          "description": "Console charts",
          "link": "https://console/cid=cid1&fExtID=folder1"
        }
      ]
    }
    """

  Scenario: Edit cluster access settings
    Given health response
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
               "fqdn": "myt-1.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "fqdn": "myt-2.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "fqdn": "sas-1.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "fqdn": "sas-2.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "fqdn": "vla-1.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "fqdn": "vla-2.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive"
           }
       ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
      "configSpec": {
        "access": {},
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
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
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
    Then we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1",
          "config_spec": {
            "access": {
              "data_transfer": true
            }
          },
          "update_mask": {
            "paths": [
                "config_spec.access"
            ]
          }
        }
        """
    Then we get gRPC response OK

    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
        """
        {
          "cluster_id": "cid1"
        }
        """
    Then we get gRPC response OK
    And gRPC response body at path "$.config.access" contains
        """
        {
          "data_transfer": true
        }
        """
