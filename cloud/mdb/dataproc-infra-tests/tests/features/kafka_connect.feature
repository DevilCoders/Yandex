Feature: Ensure Kafka Conect works

  Background: Wait until go internal api is ready
    Given we are working with standalone Kafka cluster

  @create
  Scenario: Kafka cluster create successfully
    When we try to create kafka cluster "source_cluster"
    Then grpc response should have status OK
    And generated task is finished within "15 minutes" via GRPC
    And kafka cluster "source_cluster" is up and running

  @create
  Scenario: Kafka cluster create successfully
    When we try to create kafka cluster "target_cluster"
    Then grpc response should have status OK
    And generated task is finished within "15 minutes" via GRPC
    And kafka cluster "target_cluster" is up and running

  @connector-create
  Scenario: Create mirrormaker connector
   Given kafka cluster "source_cluster" is up and running
   Given kafka cluster "target_cluster" is up and running
   When we try to create connector
   """
   {
      "connector_spec": {
        "name": "cluster-migration",
        "tasks_max": 3,
        "connector_config_mirrormaker": {
          "topics": "topic1",
          "replication_factor": "1",
          "target_cluster": {
            "alias": "target",
            "this_cluster": {}
          },
          "source_cluster": {
            "alias": "source",
            "external_cluster": {
              "bootstrap_servers": "{{ cluster_connect_strings['source_cluster'] }}",
              "sasl_username": "test",
              "sasl_password": "test-password",
              "sasl_mechanism": "SCRAM-SHA-512",
              "security_protocol": "SASL_SSL"
            }
          }
        },
        "properties": {
          "offset-syncs.topic.replication.factor": "1",
          "refresh.topics.enabled": "true",
          "refresh.topics.interval.seconds": "10"
        }
      }
   }
   """
   Then grpc response should have status OK
   And generated task is finished within "5 minutes" via GRPC
   And kafka cluster "target_cluster" is up and running

  @write-to-topic
  Scenario: Writing to topic works
    Given kafka cluster "source_cluster" is up and running
    And we write message "test_message_1" to topic "topic1" as user "test"

  @update-user
  Scenario: Kafka user update successfully
    Given kafka cluster "target_cluster" is up and running
    When we try to update kafka user
    """
    {
      "user_name": "test",
      "update_mask": "permissions",
      "permissions": [
        {
          "topic_name": "source.topic1",
          "role": 2
        }
      ]
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And kafka cluster "target_cluster" is up and running

  @read-from-topic
  Scenario: Reading from target topic works
    Given kafka cluster "target_cluster" is up and running
    Then we receive message "test_message_1" from topic "source.topic1" as user "test" within "30 seconds"

  @connector-pause
  Scenario: Pause mirrormaker connector
    Given kafka cluster "target_cluster" is up and running
    When we try to pause connector "cluster-migration"
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And kafka cluster "target_cluster" is up and running

  @connector-resume
  Scenario: Resume mirrormaker connector
    Given kafka cluster "target_cluster" is up and running
    When we try to resume connector "cluster-migration"
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And kafka cluster "target_cluster" is up and running

  @delete-source-cluster
  Scenario: Kafka cluster delete successfully
    Given kafka cluster "source_cluster" exists
    When we try to remove kafka cluster
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC

  @create-update-cluster
  Scenario: Kafka cluster create successfully
    When we try to create kafka cluster "update_cluster"
    Then grpc response should have status OK
    And generated task is finished within "15 minutes" via GRPC
    And kafka cluster "update_cluster" is up and running

  @connector-update
  Scenario: Update mirrormaker connector
    Given kafka cluster "update_cluster" is up and running
    Given kafka cluster "target_cluster" is up and running
    When we try to update connector
    """
    {
      "connector_name": "cluster-migration",
      "update_mask": "connectorSpec.tasksMax,connectorSpec.connectorConfigMirrormaker.topics,connectorSpec.connectorConfigMirrormaker.targetCluster.alias,connectorSpec.connectorConfigMirrormaker.sourceCluster.alias,connectorSpec.connectorConfigMirrormaker.sourceCluster.externalCluster.bootstrapServers,connectorSpec.connectorConfigMirrormaker.sourceCluster.externalCluster.saslUsername,connectorSpec.connectorConfigMirrormaker.sourceCluster.externalCluster.saslPassword,connectorSpec.properties.refresh.topics.enabled,connectorSpec.properties.refresh.topics.interval.seconds",
      "connector_spec": {
        "tasks_max": 3,
        "connector_config_mirrormaker": {
          "topics": "topic3",
          "target_cluster": {
            "alias": "alias"
          },
          "source_cluster": {
            "alias": "update",
            "external_cluster": {
              "bootstrap_servers": "{{ cluster_connect_strings['update_cluster'] }}",
              "sasl_username": "test-update-connector-user",
              "sasl_password": "test-update-connector-user-password"
            }
          }
        },
        "properties": {
          "refresh.topics.enabled": "false"
        }
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And kafka cluster "target_cluster" is up and running

  @connector-delete
  Scenario: Delete mirrormaker connector
   Given kafka cluster "target_cluster" is up and running
   When we try to delete connector "cluster-migration"
   Then grpc response should have status OK
   And generated task is finished within "5 minutes" via GRPC
   And kafka cluster "target_cluster" is up and running

  @delete
  Scenario: Kafka cluster delete successfully
    Given kafka cluster "target_cluster" exists
    When we try to remove kafka cluster
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC

  @delete
  Scenario: Kafka cluster delete successfully
    Given kafka cluster "update_cluster" exists
    When we try to remove kafka cluster
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
