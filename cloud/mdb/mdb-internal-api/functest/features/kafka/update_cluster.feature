@kafka
@grpc_api
Feature: Update Compute Apache Kafka Cluster
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
          "fqdn": "myt-3.df.cloud.yandex.net",
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
          "fqdn": "sas-3.df.cloud.yandex.net",
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
        },
        {
          "fqdn": "vla-3.df.cloud.yandex.net",
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

  Scenario: Update cluster description and labels
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["description", "labels"]
      },
      "description": "new description",
      "labels": {
        "key1": "val1",
        "key2": "val2"
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Update Apache Kafka cluster metadata",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateClusterMetadata",
        "cluster_id": "cid1"
      }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "description": "new description",
      "labels": {
        "key1": "val1",
        "key2": "val2"
      }
    }
    """

  Scenario: Update cluster name
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["name"]
      },
      "name": "new-name"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Update Apache Kafka cluster metadata",
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "name": "new-name"
    }
    """

  Scenario: Unable to update name if cluster with same name already exists within folder
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "other-name",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt"]
      }
    }
    """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid2",
      "update_mask": {
        "paths": ["name"]
      },
      "name": "test"
    }
    """
    Then we get gRPC response error with code ALREADY_EXISTS and message "cluster "test" already exists"

  Scenario: Changing version of the cluster is not implemented
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["config_spec.version"]
      },
      "config_spec": {
        "version": "2.7"
      }
    }
    """
    Then we get gRPC response error with code UNIMPLEMENTED and message "changing version of the cluster is not implemented"

  Scenario: Changing list of zones is not implemented
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["config_spec.zone_id"]
      },
      "config_spec": {
        "zoneId": ["myt", "vla"]
      }
    }
    """
    Then we get gRPC response error with code UNIMPLEMENTED and message "removing zones is not implemented"

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
    Then in worker_queue exists "worker_task_id2" id and data contains
    """
    {
        "timeout": "01:00:00"
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
          "name":             "myt-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-3.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-3.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-vla",
          "zone_id":   "vla"
        }
      ]
    }
    """

  Scenario: Decreasing brokers count is not implemented
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["config_spec.brokers_count"]
      },
      "config_spec": {
        "brokersCount": 1
      }
    }
    """
    Then we get gRPC response error with code UNIMPLEMENTED and message "decreasing number of brokers is not implemented"

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
          "name":             "myt-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-vla",
          "zone_id":   "vla"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-vla",
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
          "name":             "myt-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-3.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-3.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-vla",
          "zone_id":   "vla"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-vla",
          "zone_id":   "vla"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-3.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-vla",
          "zone_id":   "vla"
        }
      ]
    }
    """

  Scenario: Change kafka resources
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": [
          "config_spec.kafka.resources.resource_preset_id",
          "config_spec.kafka.resources.disk_size"
        ]
      },
      "config_spec": {
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.2",
            "diskSize": 64424509440
          }
        }
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
    Then in worker_queue exists "worker_task_id2" id and data contains
    """
    {
        "timeout": "02:00:00"
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
          "name":             "myt-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "64424509440",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.2"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "64424509440",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.2"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-vla",
          "zone_id":   "vla"
        }
      ]
    }
    """

  Scenario: Change assign public api flag works both ways
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": [
          "config_spec.assign_public_ip"
        ]
      },
      "config_spec": {
        "assignPublicIp": true
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
    Then in worker_queue exists "worker_task_id2" id and data contains
    """
    {
        "timeout": "01:00:00"
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
          "assign_public_ip": true,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": true,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-vla",
          "zone_id":   "vla"
        }
      ]
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": [
          "config_spec.assign_public_ip"
        ]
      },
      "config_spec": {
        "assignPublicIp": false
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Modify Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id3",
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
    Then in worker_queue exists "worker_task_id3" id and data contains
    """
    {
        "timeout": "01:00:00"
    }
    """
    And "worker_task_id3" acquired and finished by worker
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
          "name":             "myt-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-vla",
          "zone_id":   "vla"
        }
      ]
    }
    """

Scenario: Add host to cluster with public api flag works correctly
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": [
          "config_spec.assign_public_ip"
        ]
      },
      "config_spec": {
        "assignPublicIp": true
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
    Then in worker_queue exists "worker_task_id2" id and data contains
    """
    {
        "timeout": "01:00:00"
    }
    """
    And "worker_task_id2" acquired and finished by worker
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
      "id": "worker_task_id3",
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
    And "worker_task_id3" acquired and finished by worker
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
          "assign_public_ip": true,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": true,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "10737418240",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-vla",
          "zone_id":   "vla"
        },
        {
          "assign_public_ip": true,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-vla",
          "zone_id":   "vla"
        }
      ]
    }
    """


  Scenario: Change zookeeper resources
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": [
          "config_spec.zookeeper.resources.resource_preset_id",
          "config_spec.zookeeper.resources.disk_size"
        ]
      },
      "config_spec": {
        "zookeeper": {
          "resources": {
            "resourcePresetId": "s2.compute.2",
            "diskSize": 21474836480
          }
        }
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
    Then in worker_queue exists "worker_task_id2" id and data contains
    """
    {
        "timeout": "01:30:00"
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
          "name":             "myt-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "myt-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "21474836480",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.2"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-myt",
          "zone_id":   "myt"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "32212254720",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.1"
          },
          "role":      "KAFKA",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "sas-2.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "21474836480",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.2"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-sas",
          "zone_id":   "sas"
        },
        {
          "assign_public_ip": false,
          "cluster_id":       "cid1",
          "health":           "ALIVE",
          "system": null,
          "name":             "vla-1.df.cloud.yandex.net",
          "resources": {
            "disk_size":          "21474836480",
            "disk_type_id":       "network-ssd",
            "resource_preset_id": "s2.compute.2"
          },
          "role":      "ZOOKEEPER",
          "subnet_id": "network1-vla",
          "zone_id":   "vla"
        }
      ]
    }
    """

  Scenario: Change kafka config
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["config_spec.kafka.kafka_config_3_0.compression_type"]
      },
      "config_spec": {
        "kafka": {
          "kafka_config_3_0": {
            "compressionType": "COMPRESSION_TYPE_LZ4"
          }
        }
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
            "disk_type_id": "network-ssd",
            "disk_size": "32212254720"
          },
          "kafka_config_3_0": {
            "compression_type": "COMPRESSION_TYPE_LZ4"
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

  Scenario: Return error when no changes detected within update request
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["description", "config_spec.brokers_count", "config_spec.kafka.resources.resource_preset_id"]
      },
      "description": "test cluster",
      "configSpec": {
        "brokersCount": 1,
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1"
          }
        }
      }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"

  Scenario: Update to 3.0 in production environment works correct
    When we add default feature flag "MDB_KAFKA_ALLOW_UPGRADE"
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRODUCTION",
      "name": "test-prod",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "version": "2.8",
        "brokersCount": 1,
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 32212254720
          }
        }
      }
    }
    """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid2",
      "update_mask": {
        "paths": ["config_spec.version"]
      },
      "configSpec": {
        "version": "3.0"
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Upgrade Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateClusterMetadata",
        "cluster_id": "cid2"
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

  Scenario: Update to version 3.2 doesn't work
    When we add default feature flag "MDB_KAFKA_ALLOW_UPGRADE"
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRODUCTION",
      "name": "test-prod",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "version": "2.8",
        "brokersCount": 1,
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 32212254720
          }
        }
      }
    }
    """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid2",
      "update_mask": {
        "paths": ["config_spec.version"]
      },
      "configSpec": {
        "version": "3.2"
      }
    }
    """
    Then we get gRPC response error with code UNIMPLEMENTED and message "update to version 3.2 is not supported yet"

  @move
  Scenario: Move cluster works
    Given we allow move cluster between clouds
    When we "Move" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "destination_folder_id": "folder2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Move Apache Kafka cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.MoveClusterMetadata",
            "cluster_id": "cid1",
            "destination_folder_id": "folder2",
            "source_folder_id": "folder1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "folder_id": "folder2"
    }
    """
