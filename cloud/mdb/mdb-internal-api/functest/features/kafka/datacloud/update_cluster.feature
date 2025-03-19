@kafka
@grpc_api
Feature: Update DataCloud Compute Apache Kafka Cluster
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
          "fqdn": "akf-ec1a-1.cid1.yadc.io",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "akf-ec1a-3.cid1.yadc.io",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "akf-ec1a-4.cid1.yadc.io",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "akf-ec1b-2.cid1.yadc.io",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "akf-ec1b-4.cid1.yadc.io",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "akf-ec1b-5.cid1.yadc.io",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "akf-ec1c-3.cid1.yadc.io",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "akf-ec1c-6.cid1.yadc.io",
          "cid": "cid1",
          "status": "Alive"
        }
      ]
    }
    """
    When we "Create" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "project_id": "folder1",
      "cloud_type": "aws",
      "region_id": "eu-central1",
      "name": "test",
      "description": "test cluster",
      "version": "3.0",
      "resources": {
          "kafka": {
              "resource_preset_id": "a1.aws.1",
              "disk_size": 34359738368,
              "broker_count": 1,
              "zone_count": 2
          }
      }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "cluster_id": "cid1",
        "operation_id": "worker_task_id1"
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: Update cluster description
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "description": "new description"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body
    """
    {
      "description": "new description"
    }
    """

  Scenario: Update cluster name
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "name": "new-name"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body
    """
    {
      "name": "new-name"
    }
    """

  Scenario: Unable to update name if cluster with same name already exists within folder
    When we "Create" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "project_id": "folder1",
      "cloud_type": "aws",
      "region_id": "eu-central1",
      "name": "other-name",
      "description": "test cluster",
      "version": "3.0",
      "resources": {
          "kafka": {
              "resource_preset_id": "a1.aws.1",
              "disk_size": 34359738368,
              "broker_count": 1,
              "zone_count": 2
          }
      }
    }
    """
    Then we get DataCloud response OK
    And "worker_task_id2" acquired and finished by worker
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid2",
      "name": "test"
    }
    """
    Then we get DataCloud response error with code ALREADY_EXISTS and message "cluster "test" already exists"

  Scenario: Changing list of zones is not implemented
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "resources": {
        "kafka": {
          "zone_count": 1
        }
      }
    }
    """
    Then we get DataCloud response error with code UNIMPLEMENTED and message "removing zones is not implemented"

  Scenario: Changing brokers count
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "resources": {
        "kafka": {
          "broker_count": 2
        }
      }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then in worker_queue exists "worker_task_id2" id and data contains
    """
    {
        "timeout": "01:00:00"
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "ListHosts" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body
    """
    {
      "hosts": [
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1a-1.cid1.yadc.io"
        },
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1a-3.cid1.yadc.io"
        },
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1b-2.cid1.yadc.io"
        },
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1b-4.cid1.yadc.io"
        }
      ]
    }
    """

  Scenario: Decreasing brokers count is not implemented
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "resources": {
        "kafka": {
          "broker_count": 2
        }
      }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "resources": {
        "kafka": {
          "broker_count": 1
        }
      }
    }
    """
    Then we get DataCloud response error with code UNIMPLEMENTED and message "decreasing number of brokers is not implemented"

  Scenario: Adding zone
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "resources": {
        "kafka": {
          "zone_count": 3
        }
      }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "update-firewall" set to "true"
    And in worker_queue exists "worker_task_id2" id with args "hosts_create" set to "[akf-ec1c-3.cid1.yadc.io]"
    And in worker_queue exists "worker_task_id2" id with args "restart" set to "false"
    And "worker_task_id2" acquired and finished by worker
    When we "ListHosts" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body
    """
    {
      "hosts": [
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1a-1.cid1.yadc.io"
        },
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1b-2.cid1.yadc.io"
        },
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1c-3.cid1.yadc.io"
        }
      ]
    }
    """

  Scenario: Adding zone and increasing brokers count
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "resources": {
        "kafka": {
          "zone_count": 3,
          "broker_count": 2
        }
      }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "ListHosts" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body
    """
    {
      "hosts": [
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1a-1.cid1.yadc.io"
        },
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1a-4.cid1.yadc.io"
        },
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1b-2.cid1.yadc.io"
        },
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1b-5.cid1.yadc.io"
        },
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1c-3.cid1.yadc.io"
        },
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1c-6.cid1.yadc.io"
        }
      ]
    }
    """

  Scenario: Change kafka resources
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "resources": {
        "kafka": {
          "resource_preset_id": "a1.aws.2",
          "disk_size": 68719476736
        }
      }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then in worker_queue exists "worker_task_id2" id and data contains
    """
    {
        "timeout": "02:04:00"
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "ListHosts" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body
    """
    {
      "hosts": [
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1a-1.cid1.yadc.io"
        },
        {
          "cluster_id":       "cid1",
          "status":           "HOST_STATUS_ALIVE",
          "name":             "akf-ec1b-2.cid1.yadc.io"
        }
      ]
    }
    """

  Scenario: Return error when no changes detected within update request
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "description": "test cluster",
      "resources": {
          "kafka": {
              "resource_preset_id": "a1.aws.1",
              "broker_count": 1
          }
      }
    }
    """
    Then we get DataCloud response error with code FAILED_PRECONDITION and message "no changes detected"

  Scenario: Modify cluster access works
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "access": {
          "ipv4_cidr_blocks": { "values":[{"value":"0.0.0.0/0","description":"test access"}]},
          "data_services": {"values":["DATA_SERVICE_TRANSFER"]}
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "sensitive": true
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid1",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": "random_string1"
        },
        "private_connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": "random_string1"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_ALIVE",
        "version": "3.0",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "broker_count": "1",
                "zone_count": "2"
            }
        },
        "access": {
          "ipv4_cidr_blocks": { "values":[ {"value":"0.0.0.0/0", "description":"test access" } ] },
          "ipv6_cidr_blocks": null,
          "data_services": {"values":["DATA_SERVICE_TRANSFER"]}
        }
    }
    """
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "access": {
          "ipv4_cidr_blocks": { "values":[]},
          "data_services": {"values":["DATA_SERVICE_TRANSFER"]}
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id3"
    }
    """
    When "worker_task_id3" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "sensitive": true
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid1",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": "random_string1"
        },
        "private_connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": "random_string1"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_ALIVE",
        "version": "3.0",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "broker_count": "1",
                "zone_count": "2"
            }
        },
        "access": {
          "ipv4_cidr_blocks": null,
          "ipv6_cidr_blocks": null,
          "data_services": { "values":["DATA_SERVICE_TRANSFER"] }
        }
    }
    """
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "access": {
          "data_services": {"values":[]}
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id4"
    }
    """
    When "worker_task_id4" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "sensitive": true
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid1",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": "random_string1"
        },
        "private_connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": "random_string1"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_ALIVE",
        "version": "3.0",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "broker_count": "1",
                "zone_count": "2"
            }
        },
        "access": null
    }
    """
