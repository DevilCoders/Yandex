Feature: Create Compute Redis cluster

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with standard Redis cluster


  @create
  Scenario: Redis cluster created successfully
    When we try to create cluster "test_cluster"
    Then response should have status 200
    And generated task is finished within "20 minutes"


  @delete
  Scenario: Redis cluster deleted successfully
    Given cluster "test_cluster" exists
    When we try to remove cluster
    Then response should have status 200
    And generated task is finished within "5 minutes"
