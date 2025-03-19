@kafka
@grpc_api
Feature: Update Porto Apache Kafka Cluster
  Background:
    Given default headers
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
          "fqdn": "myt-1.db.yandex.net",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "myt-2.db.yandex.net",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "myt-3.db.yandex.net",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "sas-1.db.yandex.net",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "sas-2.db.yandex.net",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "sas-3.db.yandex.net",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "vla-1.db.yandex.net",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "vla-2.db.yandex.net",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "vla-3.db.yandex.net",
          "cid": "cid1",
          "status": "Alive"
        }
      ]
    }
    """
    When we add default feature flag "MDB_KAFKA_CLUSTER"
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.porto.1",
            "diskTypeId": "local-ssd",
            "diskSize": 10737418240
          }
        },
        "zookeeper": {
          "resources": {
            "resourcePresetId": "s2.porto.1",
            "diskTypeId": "local-ssd",
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


  Scenario: Changing brokers count
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["config_spec.brokers_count"]
      },
      "config_spec": {
        "brokersCount": 2
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Modify Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateClusterMetadata",
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
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-1.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-2.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-3.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-1.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-2.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-3.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-1.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "",
          "zone_id":   "vla"
        }
      ]
    }
    """


  Scenario: Adding zone
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["config_spec.zone_id"]
      },
      "config_spec": {
        "zoneId": ["myt", "sas", "vla"]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Modify Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateClusterMetadata",
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
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-1.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-2.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-1.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-2.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-1.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "",
          "zone_id":   "vla"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-2.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "vla"
        }
      ]
    }
    """

  Scenario: Adding zone and increasing brokers count
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["config_spec.zone_id", "config_spec.brokers_count"]
      },
      "config_spec": {
        "zoneId": ["myt", "sas", "vla"],
        "brokersCount": 2
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Modify Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateClusterMetadata",
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
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-1.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-2.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-3.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-1.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-2.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-3.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-1.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "",
          "zone_id":   "vla"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-2.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "vla"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-3.db.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "local-ssd",
            "resource_preset_id": "s2.porto.1"
          },
          "role":      "KAFKA",
          "subnet_id": "",
          "zone_id":   "vla"
        }
      ]
    }
    """
