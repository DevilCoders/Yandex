Feature: Create Compute Dataproc cluster

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with standard Hadoop cluster


  @create
  Scenario: Dataproc cluster created successfully
    When we try to create cluster "test_cluster"
    Then response should have status 200
    And generated task is finished within "10 minutes"


  @delete
  Scenario: Dataproc cluster deleted successfully
    Given cluster "test_cluster" exists
    When we try to remove cluster
    Then response should have status 200
    And generated task is finished within "3 minutes"
