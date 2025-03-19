@basic
Feature: Basic

  Scenario: Producer should send documents
    Given empty search_queue
    And "2" documents in search_queue
    Then in search_queue there are "2" unsent documents
    When I run mdb-search-producer
    Then in search_queue there are no unsent documents
