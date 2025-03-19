Feature: Run Kafka cluster on local ssd disks

  Background: Wait until go internal api is ready
    Given we are working with standart Kafka cluster

  @create @trunk
  Scenario: Create cluster
    When we run YC CLI
    """
    yc kafka cluster create test-local-ssd
    --zone-ids ru-central1-b
    --brokers-count 3
    --network-id {{ conf['test_managed']['networkId'] }}
    --subnet-ids {{ ','.join(conf['test_managed']['subnetIds']) }}
    --version 2.8
    --resource-preset s2.micro
    --disk-size 100GB
    --disk-type local-ssd
    --log-segment-bytes 104857600
    """
    Then YC CLI exit code is 0

  @resize-disk @trunk
  Scenario: Enlarge local-ssd disk of Kafka cluster
    Given kafka cluster "test-local-ssd" is up and running
    When we run YC CLI with "35 minutes" timeout
    """
    yc kafka cluster update test-local-ssd --disk-size 200GB
    """
    Then YC CLI exit code is 0
    And kafka cluster "test-local-ssd" is up and running

  @delete @trunk
  Scenario: Kafka cluster delete successfully
    Given kafka cluster "test-local-ssd" exists
    When we try to remove kafka cluster
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
