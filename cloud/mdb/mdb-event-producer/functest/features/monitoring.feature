Feature: Monitoring should works

  Scenario: Check monitoring
    When I run mdb-event-queue-unsent with warn="1ms" crit="1s"
    Then monitoring returns "OK"
    When I create cloud, folder, "2" clusters and tasks with events
    And I sleep "1s"
    When I run mdb-event-queue-unsent with warn="1ms" crit="60s"
    Then monitoring returns "WARN"
    When I run mdb-event-queue-unsent with warn="1ms" crit="2ms"
    Then monitoring returns "CRIT"
    When I run mdb-event-queue-unsent with warn="24h" crit="48h"
    Then monitoring returns "OK"
