Feature: Monitoring

  Scenario: Check mdb-search-queue-documents-age monitoring
    Given empty search_queue
    When I run search-queue-documents-age with warn="1ms" crit="1s"
    Then monitoring returns "OK"
    When I add "2" documents to the search_queue
    And I sleep "1s"
    And I run search-queue-documents-age with warn="1ms" crit="1h"
    Then monitoring returns "WARN"
    When I run search-queue-documents-age with warn="1ms" crit="2ms"
    Then monitoring returns "CRIT"
