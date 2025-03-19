Feature: Create Compute Kafka cluster

  Background: Wait until go internal api is ready
    Given we are working with standart Kafka cluster


  @create
  Scenario: Kafka cluster create successfully
    When we try to create kafka cluster "test_cluster"
    Then grpc response should have status OK
    And generated task is finished within "15 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running


  @create-topic
  Scenario: Kafka topic create successfully
    Given kafka cluster "test_cluster" is up and running
    When we try to create kafka topic
    """
    {
      "topic_spec": {
        "name": "topic3",
        "partitions": 4,
        "replication_factor": 2,
        "topic_config_3_1": {
            "compression_type": 4,
            "flush_ms": 9223372036854775807
        }
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running


  @create-user
  Scenario: Kafka user create successfully
    Given kafka cluster "test_cluster" is up and running
    When we try to create kafka user
    """
    {
      "user_spec": {
        "name": "test",
        "password": "test-password",
        "permissions": [
          {
            "topic_name": "topic1",
            "role": "ACCESS_ROLE_PRODUCER"
          },
          {
            "topic_name": "topic2",
            "role": "ACCESS_ROLE_CONSUMER"
          },
          {
            "topic_name": "topic2",
            "role": "ACCESS_ROLE_PRODUCER"
          }
        ]
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running


  @read-write
  Scenario: Writing to topic and reading from topic works
    Given kafka cluster "test_cluster" is up and running
    And we write message "test_message_1" to topic "topic2" as user "test"
    Then we read message from topic "topic2" as user "test" and its text is "test_message_1"

  @authorization
  Scenario: Writing to topic and reading from topic without proper authorization does not work
    Given kafka cluster "test_cluster" is up and running
    When we try to write message "test_message_2" to topic "topic3" as user "test"
    Then producer fails with error TOPIC_AUTHORIZATION_FAILED
    When we try to read message from topic "topic3" as user "test"
    Then consumer fails with error TOPIC_AUTHORIZATION_FAILED
    When we try to write message "test_message_3" to topic "topic3" as user "unknown"
    Then producer fails with error _AUTHENTICATION

  @topic-sync @topic-sync-create
  Scenario: Topic created via Kafka Admin API is also visible within Cloud API
    Given kafka cluster "test_cluster" is up and running
    When user "admin" creates topic "best_topic" via Kafka Admin API with 6 partitions and 2 replicas and following config
    """
    segment.bytes: 104857600
    preallocate: True
    """
    Then wait no more than "30 seconds" until topic "best_topic" appears within Cloud API
    And topic "best_topic" has following params
    """
    partitions: 6
    replication_factor: 2
    config:
      segment_bytes: 104857600
      preallocate: True
    """

  @topic-sync @topic-sync-update
  Scenario: Changes to topics made via Kafka Admin API are synced to control-plane
    Given kafka cluster "test_cluster" is up and running
    When user "admin" updates topic "best_topic" to have 12 partitions and sets its config to
    """
    segment.bytes: 209715200
    preallocate: True
    max.message.bytes: 1048576
    """
    Then topic "best_topic" has following params within "30 seconds"
    """
    partitions: 12
    replication_factor: 2
    config:
      segment_bytes: 209715200
      preallocate: True
      max_message_bytes: 1048576
    """

  @topic-sync @topic-sync-delete
  Scenario: Topic deleted via Kafka Admin API is also removed from Cloud API
    Given kafka cluster "test_cluster" is up and running
    When user "admin" deletes topic "best_topic" via Kafka Admin API
    Then topic "best_topic" disappears from Cloud API within "30 seconds"


  Scenario: Kafka user update successfully
    Given kafka cluster "test_cluster" is up and running
    When we try to update kafka user
    """
    {
      "user_name": "test",
      "update_mask": "password,permissions",
      "password": "newPassword"
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running


  Scenario: Kafka topic update successfully
    Given kafka cluster "test_cluster" is up and running
    When we try to update kafka topic
    """
    {
      "topic_name": "topic1",
      "update_mask": "topicSpec.partitions",
      "topic_spec": {
        "partitions": 6
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running


  @modify @strengthen @trunk
  Scenario: Kafka cluster update successfully
    Given kafka cluster "test_cluster" is up and running
    When we try to update kafka cluster
    """
    {
      "config_spec": {
        "kafka": {
          "resources": {
            "resource_preset_id": "s2.small"
          }
        }
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "10 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running


  @modify @extend @trunk
  Scenario: Kafka cluster update successfully
    Given kafka cluster "test_cluster" is up and running
    When we try to update kafka cluster
    """
    {
      "config_spec": {
        "kafka": {
          "resources": {
            "disk_size": 53687091200
          }
        }
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "15 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running


  @modify @update-config
  Scenario: Update kafka config
    Given kafka cluster "test_cluster" is up and running
    When we try to update kafka cluster
    """
    {
      "config_spec": {
        "kafka": {
          "kafka_config_3_1": {
            "log_flush_interval_messages": 10000
          }
        }
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "10 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running


  Scenario: Kafka topic delete successfully
    Given kafka cluster "test_cluster" is up and running
    When we try to delete kafka topic "topic3"
    Then grpc response should have status OK
    And generated task is finished within "3 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running


  Scenario: Kafka user delete successfully
    Given kafka cluster "test_cluster" is up and running
    When we try to delete kafka user "test"
    Then grpc response should have status OK
    And generated task is finished within "3 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running

  @revert @securitygroups
  Scenario: Kafka security group delete works
    Given kafka cluster "test_cluster" is up and running
    When we try to update kafka cluster
    """
    {
      "security_group_ids": []
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running

  @modify @securitygroups
  Scenario: Kafka security group modify works
    Given kafka cluster "test_cluster" is up and running
    When we try to update kafka cluster
    """
    {
      "security_group_ids": ["{{ kafkaSecurityGroupId }}"]
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running

  @delete
  Scenario: Kafka cluster delete successfully
    Given kafka cluster "test_cluster" exists
    When we try to remove kafka cluster
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
