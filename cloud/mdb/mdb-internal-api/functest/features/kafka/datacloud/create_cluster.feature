@kafka
@grpc_api
Feature: Create/Modify DataCloud Kafka Cluster
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
          "fqdn": "akf-ec1b-2.cid1.yadc.io",
          "cid": "cid1",
          "status": "Alive"
        },
        {
          "fqdn": "akf-ec1c-3.cid1.yadc.io",
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
        "name": "test_cluster",
        "description": "test cluster",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "broker_count": 1,
                "zone_count": 3
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
    When "worker_task_id1" acquired and finished by worker

  Scenario: Get ListHosts
    And we "ListHosts" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
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
          "name": "akf-ec1a-1.cid1.yadc.io",
          "cluster_id": "cid1",
          "status": "HOST_STATUS_ALIVE"
        },
        {
          "name": "akf-ec1b-2.cid1.yadc.io",
          "cluster_id": "cid1",
          "status": "HOST_STATUS_ALIVE"
        },
        {
          "name": "akf-ec1c-3.cid1.yadc.io",
          "cluster_id": "cid1",
          "status": "HOST_STATUS_ALIVE"
        }
      ]
    }
    """

  Scenario: Create and Get Kafka cluster
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
            "connection_string": "akf-ec1a-1.cid1.yadc.io:9091,akf-ec1b-2.cid1.yadc.io:9091",
            "user": "admin",
            "password": "random_string1"
        },
        "private_connection_info": {
            "connection_string": "akf-ec1a-1.cid1.private.yadc.io:19091,akf-ec1b-2.cid1.private.yadc.io:19091",
            "user": "admin",
            "password": "random_string1"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_ALIVE",
        "version": "3.0",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "broker_count": "1",
                "zone_count": "3"
            }
        },
        "access": null
    }
    """

  Scenario: Get sensitive data without permission acts like sensitive is false
    Given default headers with "ro-token" token
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
            "connection_string": "akf-ec1a-1.cid1.yadc.io:9091,akf-ec1b-2.cid1.yadc.io:9091",
            "user": "admin",
            "password": ""
        },
        "private_connection_info": {
            "connection_string": "akf-ec1a-1.cid1.private.yadc.io:19091,akf-ec1b-2.cid1.private.yadc.io:19091",
            "user": "admin",
            "password": ""
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_ALIVE",
        "version": "3.0",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "broker_count": "1",
                "zone_count": "3"
            }
        },
        "access": null
    }
    """

  Scenario: Create and List Kafka cluster
    When we "List" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "project_id": "folder1",
      "view": 2
    }
    """
    Then we get DataCloud response with body
    """
    {
      "clusters": [
        {
          "id": "cid1",
          "create_time": "**IGNORE**",
          "project_id": "folder1",
          "cloud_type": "aws",
          "connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": ""
          },
          "private_connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": ""
          },
          "region_id": "eu-central1",
          "network_id": "network1",
          "name": "test_cluster",
          "description": "test cluster",
          "status": "CLUSTER_STATUS_ALIVE",
          "version": "3.0",
          "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "broker_count": "1",
                "zone_count": "3"
            }
          },
          "access": null,
          "encryption": null
        }
      ]
    }
    """

  Scenario: Create and Reset credentials Kafka cluster
    When we "ResetCredentials" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
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
            "password": "random_string5"
        },
        "private_connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": "random_string5"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UPDATING",
        "version": "3.0",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "broker_count": "1",
                "zone_count": "3"
            }
        },
        "access": null
    }
    """

  Scenario: Create and Delete Kafka cluster
    When we "Delete" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body
    """
    {
      "operation_id": "worker_task_id2"
    }
    """
    When we "List" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "project_id": "folder1",
      "view": 2
    }
    """
    Then we get DataCloud response with body
    """
    {
      "clusters": []
    }
    """

  Scenario: Create/Get of Kafka cluster with Access params works
    When we "Create" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster_2",
        "description": "test cluster",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "broker_count": 1,
                "zone_count": 1
            }
        },
         "access": {
            "ipv4_cidr_blocks": {
                "values":[{"value":"0.0.0.0/0"}]
                },
            "ipv6_cidr_blocks": {
                "values":[{"value":"::/0"}]
                },
            "data_services": {"values":[2]}
        }
    }
    """

    Then we get DataCloud response OK
    When "worker_task_id2" acquired and finished by worker

    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2",
        "sensitive": false
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid2",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": ""
        },
        "private_connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": ""
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster_2",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN",
        "version": "3.0",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "broker_count": "1",
                "zone_count": "1"
            }
        },
        "access": {
            "ipv4_cidr_blocks": {
                "values":[{"value":"0.0.0.0/0","description":""}]
                },
            "ipv6_cidr_blocks": {
                "values":[{"value":"::/0","description":""}]
                },
            "data_services": {"values":["DATA_SERVICE_TRANSFER"]}
        }
    }
    """

  Scenario: Create/Get of Kafka cluster with Encryption params works
    When we "Create" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster_2",
        "description": "test cluster",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "broker_count": 1,
                "zone_count": 1
            }
         },
         "encryption": {
             "enabled": true
         }
    }
    """

    Then we get DataCloud response OK
    When "worker_task_id2" acquired and finished by worker

    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2",
        "sensitive": false
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid2",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": ""
        },
        "private_connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": ""
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster_2",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN",
        "version": "3.0",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "broker_count": "1",
                "zone_count": "1"
            }
        },
        "encryption": {
            "enabled": true
        }
    }
    """
    When we GET via PillarConfig "/v1/config/akf-ec1a-1.cid2.yadc.io"
    Then we get response with status 200
    And body at path "$.data.encryption" contains
    """
    {
      "enabled": true
    }
    """
    When we update pillar for cluster "cid2" on path "{data,encryption,key}" with value '{"id": "id1", "type": "aws"}'
    Then cluster "cid2" pillar has value '{"key": {"id": "id1", "type": "aws"}, "enabled": true}' on path "data:encryption"
    When we "Create" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid2",
      "topic_spec": {
        "name": "topic",
        "partitions": 12,
        "replication_factor": 1
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
    Then cluster "cid2" pillar has value '{"key": {"id": "id1", "type": "aws"}, "enabled": true}' on path "data:encryption"

  Scenario: Stop/Start cluster works
    When we "Stop" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    { "cluster_id": "cid1"}
    """
    Then we get DataCloud response with body ignoring empty
    """
    { "status": "CLUSTER_STATUS_STOPPING" }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    { "cluster_id": "cid1"}
    """
    Then we get DataCloud response with body ignoring empty
    """
    { "status": "CLUSTER_STATUS_STOPPED" }
    """
    When we "Start" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    { "cluster_id": "cid1"}
    """
    Then we get DataCloud response with body ignoring empty
    """
    { "status": "CLUSTER_STATUS_STARTING" }
    """
    When "worker_task_id3" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    { "cluster_id": "cid1"}
    """
    Then we get DataCloud response with body ignoring empty
    """
    { "status": "CLUSTER_STATUS_ALIVE" }
    """

  Scenario: Create/Get of Kafka cluster with specified network
    When we "Create" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster_2",
        "description": "test cluster",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "broker_count": 1,
                "zone_count": 1
            }
         },
         "network_id": "some-network-id"
    }
    """

    Then we get DataCloud response OK
    When "worker_task_id2" acquired and finished by worker

    When we "Get" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2",
        "sensitive": false
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid2",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": ""
        },
        "private_connection_info": {
            "connection_string": "**IGNORE**",
            "user": "admin",
            "password": ""
        },
        "region_id": "eu-central1",
        "name": "test_cluster_2",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN",
        "version": "3.0",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "broker_count": "1",
                "zone_count": "1"
            }
        },
        "network_id": "some-network-id"
    }
    """
