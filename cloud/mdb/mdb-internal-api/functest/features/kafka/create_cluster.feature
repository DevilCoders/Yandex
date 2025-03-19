@kafka
@grpc_api
@wip
Feature: Create Compute Apache Kafka Cluster
  Background:
    Given default headers
    And we add default feature flag "MDB_KAFKA_CLUSTER"

  Scenario: Cluster creation and delete works
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
                "data_transfer": false
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
    And we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "hosts": [
        {
          "name": "myt-1.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "myt",
          "role": "KAFKA",
          "subnet_id": "network1-myt",
          "assign_public_ip": false,
          "health": "ALIVE",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "32212254720"
          }
        },
        {
          "name": "myt-2.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "myt",
          "role": "ZOOKEEPER",
          "subnet_id": "network1-myt",
          "assign_public_ip": false,
          "health": "ALIVE",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        },
        {
          "name": "sas-1.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "sas",
          "role": "KAFKA",
          "subnet_id": "network1-sas",
          "assign_public_ip": false,
          "health": "ALIVE",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "32212254720"
          }
        },
        {
          "name": "sas-2.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "sas",
          "role": "ZOOKEEPER",
          "subnet_id": "network1-sas",
          "assign_public_ip": false,
          "health": "ALIVE",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        },
        {
          "name": "vla-1.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "vla",
          "role": "ZOOKEEPER",
          "subnet_id": "network1-vla",
          "assign_public_ip": false,
          "health": "ALIVE",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        }
      ]
    }
    """
    And we run query
    """
    UPDATE dbaas.worker_queue
    SET create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00',
        start_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00',
        end_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    WHERE task_id = 'worker_task_id1'
    """
    When we "ListOperations" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "operations": [
        {
          "id": "worker_task_id1",
          "description": "Create Apache Kafka cluster",
          "created_by": "user",
          "created_at": "2000-01-01T00:00:00Z",
          "modified_at": "2000-01-01T00:00:00Z",
          "done": true,
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
      ]
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.DeleteClusterMetadata",
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

  Scenario: Cluster creation in network with multiple subnets works correctly
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network2",
      "subnetId": ["network2-myt","network2-vla2", "network2-sas"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "vla"]
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
    And we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "hosts": [
        {
          "name": "myt-1.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "myt",
          "role": "KAFKA",
          "subnet_id": "network2-myt",
          "assign_public_ip": false,
          "health": "UNKNOWN",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "32212254720"
          }
        },
        {
          "name": "myt-2.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "myt",
          "role": "ZOOKEEPER",
          "subnet_id": "network2-myt",
          "assign_public_ip": false,
          "health": "UNKNOWN",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        },
        {
          "name": "sas-1.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "sas",
          "role": "ZOOKEEPER",
          "subnet_id": "network2-sas",
          "assign_public_ip": false,
          "health": "UNKNOWN",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        },
        {
          "name": "vla-1.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "vla",
          "role": "KAFKA",
          "subnet_id": "network2-vla2",
          "assign_public_ip": false,
          "health": "UNKNOWN",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "32212254720"
          }
        },
        {
          "name": "vla-2.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "vla",
          "role": "ZOOKEEPER",
          "subnet_id": "network2-vla2",
          "assign_public_ip": false,
          "health": "UNKNOWN",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        }
      ]
    }
    """

  Scenario: Cluster stop and start works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
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
    When we "Stop" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Stop Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.StopClusterMetadata",
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
    And "worker_task_id2" acquired and finished by worker
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
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1",
      "status": "STOPPED"
    }
    """
    When we "Start" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Start Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.StartClusterMetadata",
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
    And "worker_task_id3" acquired and finished by worker
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
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1",
      "status": "RUNNING"
    }
    """

  Scenario: Cluster creation with public_ip works correctly
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network2",
      "subnetId": ["network2-myt","network2-vla2", "network2-sas"],
      "configSpec": {
        "assignPublicIp": true,
        "brokersCount": 1,
        "zoneId": ["myt", "vla"]
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
    And we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "hosts": [
        {
          "name": "myt-1.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "myt",
          "role": "KAFKA",
          "subnet_id": "network2-myt",
          "assign_public_ip": true,
          "health": "UNKNOWN",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "32212254720"
          }
        },
        {
          "name": "myt-2.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "myt",
          "role": "ZOOKEEPER",
          "subnet_id": "network2-myt",
          "assign_public_ip": false,
          "health": "UNKNOWN",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        },
        {
          "name": "sas-1.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "sas",
          "role": "ZOOKEEPER",
          "subnet_id": "network2-sas",
          "assign_public_ip": false,
          "health": "UNKNOWN",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        },
        {
          "name": "vla-1.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "vla",
          "role": "KAFKA",
          "subnet_id": "network2-vla2",
          "assign_public_ip": true,
          "health": "UNKNOWN",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "32212254720"
          }
        },
        {
          "name": "vla-2.df.cloud.yandex.net",
          "cluster_id": "cid1",
          "zone_id": "vla",
          "role": "ZOOKEEPER",
          "subnet_id": "network2-vla2",
          "assign_public_ip": false,
          "health": "UNKNOWN",
          "system": null,
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        }
      ]
    }
    """

  Scenario: Cluster creation with one broker works correctly
    Given health response
    """
    {
      "clusters": [
          {
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
      "networkId": "network1",
      "subnetId": ["network1-vla"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["vla"]
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
                "data_transfer": false
        },
        "assign_public_ip": false,
        "brokers_count": "1",
        "version": "3.0",
        "zone_id": ["vla"],
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
        "zookeeper": null
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

  Scenario: Cluster creation with enabled connect works correctly
    Given health response
    """
    {
      "clusters": [
          {
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
      "networkId": "network1",
      "subnetId": ["network1-vla"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["vla"]
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
                "data_transfer": false
        },
        "assign_public_ip": false,
        "brokers_count": "1",
        "version": "3.0",
        "zone_id": ["vla"],
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
        "zookeeper": null
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
