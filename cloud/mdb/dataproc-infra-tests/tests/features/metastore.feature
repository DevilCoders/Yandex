Feature: Create Compute Metastore cluster

  Background: Wait until go internal api is ready
    Given we are working with standard Metastore cluster


  @create
  Scenario: Metastore cluster create successfully
    When we try to create metastore cluster "test_cluster"
    Then grpc response should have status OK
    And generated task is finished within "20 minutes" via GRPC
    And metastore cluster "test_cluster" is up and running


  @test
  Scenario: Metastore is being tested
    When we test metastore
    Then metastore cluster "test_cluster" is up and running


  @delete
  Scenario: Metastore cluster delete successfully
    Given metastore cluster "test_cluster" exists
    When we try to remove metastore cluster
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
