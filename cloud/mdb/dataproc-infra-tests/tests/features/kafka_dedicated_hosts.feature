Feature: Kafka cluster on dedicated hosts

  Background: Wait until go internal api is ready
    Given we are working with standart Kafka cluster

  @create
  Scenario: Create Kafka cluster on dedicated host
    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc kafka cluster create kafka_on_dedicated_host
    --version 2.8
    --environment production
    --network-id {{ conf['test_managed']['networkId'] }}
    --subnet-ids {{ subnetWithHostGroup }}
    --brokers-count 2
    --zone-ids {{ zoneWithHostGroup }}
    --resource-preset s2.small
    --disk-size 100GB
    --disk-type network-hdd
    --host-group-ids {{ hostGroupId }}
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    host_group_ids:
    - {{ hostGroupId }}
    """
    And kafka cluster "kafka_on_dedicated_host" is up and running
    And all 5 hosts of current cluster reside on the same dedicated host


  @strengthen
  Scenario: Change resource preset of Kafka brokers
    When we run YC CLI
    """
    yc kafka cluster update kafka_on_dedicated_host
    --resource-preset s2.medium
    """
    Then YC CLI exit code is 0
    And kafka cluster "kafka_on_dedicated_host" is up and running
    And all 5 hosts of current cluster reside on the same dedicated host


  Scenario: Add broker to Kafka cluster
    When we run YC CLI
    """
    yc kafka cluster update kafka_on_dedicated_host
    --brokers-count 3
    """
    Then YC CLI exit code is 0
    And kafka cluster "kafka_on_dedicated_host" is up and running
    And all 6 hosts of current cluster reside on the same dedicated host


  @delete
  Scenario: Delete Kafka cluster
    When we run YC CLI
    """
    yc kafka cluster delete kafka_on_dedicated_host
    """
    Then YC CLI exit code is 0
