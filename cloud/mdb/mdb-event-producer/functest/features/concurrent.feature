Feature: Producers should works concurrently

  Scenario: Run 2 event producers
    Given cloud, folder, "50" clusters and tasks with events
    When I run "2" mdb-event-producers
    Then my start events are sent in "30s"
    When I finish my tasks
    Then my done events are sent in "30s"
