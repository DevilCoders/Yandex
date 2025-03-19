Feature: Producers should sent events

  Scenario: Run event producer
    Given cloud, folder, "2" clusters and tasks with events
    When I run one mdb-event-producer
    Then my start events are sent in "30s"
    When I fail my tasks
    Then my done events are not sent in "30s"
