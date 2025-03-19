Feature: Create Compute Kafka cluster

  Background: Wait until go internal api is ready
    Given we are working with kafka28 Kafka cluster


  @create
  Scenario: Kafka cluster create successfully
    When we try to create kafka cluster "test_cluster"
    Then grpc response should have status OK
    And generated task is finished within "15 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running
    And config of Kafka broker 1 has following entries
    """
    inter.broker.protocol.version: "2.8-IV0"
    """


  @upgrade30
  Scenario: Kafka28 cluster is upgraded to Kafka30 successfully
    Given kafka cluster "test_cluster" is up and running
    When we try to update kafka cluster
    """
    {
      "config_spec": {
        "version": "3.0"
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "10 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running
    And config of Kafka broker 1 has following entries
    """
    inter.broker.protocol.version: "3.0"
    """


  @upgrade31
  Scenario: Kafka30 cluster is upgraded to Kafka31 successfully
    Given kafka cluster "test_cluster" is up and running
    When we try to update kafka cluster
    """
    {
      "config_spec": {
        "version": "3.1"
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "10 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running
    And config of Kafka broker 1 has following entries
    """
    inter.broker.protocol.version: "3.1"
    """


  @upgrade32
  Scenario: Kafka31 cluster is upgraded to Kafka32 successfully
    Given kafka cluster "test_cluster" is up and running
    When we try to update kafka cluster
    """
    {
      "config_spec": {
        "version": "3.2"
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "10 minutes" via GRPC
    And kafka cluster "test_cluster" is up and running
    And config of Kafka broker 1 has following entries
    """
    inter.broker.protocol.version: "3.2"
    """

  @delete
  Scenario: Kafka cluster delete successfully
    Given kafka cluster "test_cluster" exists
    When we try to remove kafka cluster
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
