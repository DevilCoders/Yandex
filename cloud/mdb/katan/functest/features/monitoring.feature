@monitoring
Feature: monitoring

    Background: Database with 2 imported clusters
        Given empty databases
        And "2" postgresql clusters in metadb
        And mdb-katan-imp import them

    Scenario: Rollout zombies monitoring
        When I run mdb-katan-zombie-rollouts with warn="1h" crit="3h"
        Then monitoring returns "OK"
        When I add rollout for
        """
        {
          "meta": {
            "type": "postgresql_cluster"
          }
        }
        """
        And I start one pending rollout
        And I sleep "1s"
        And I run mdb-katan-zombie-rollouts with warn="1ms" crit="1h"
        Then monitoring returns "WARN"
        And I run mdb-katan-zombie-rollouts with warn="1ms" crit="3ms"
        Then monitoring returns "CRIT"

    Scenario: Broken schedules monitoring
        Given all deploy shipments are "FAILED"
        And all clusters in health are "Alive"
        And "core" katan.schedule with maxSize=1 for
        """
        {
          "meta": {
            "type": "postgresql_cluster"
          }
        }
        """
        When I run mdb-katan-broken-schedules with namespace="not-a-core"
        Then monitoring returns "OK"
        When I run mdb-katan-broken-schedules with namespace="core"
        Then monitoring returns "OK"
        When I execute mdb-katan-scheduler
        And I execute mdb-katan
        And I execute mdb-katan-scheduler with MaxRolloutFails=1
        Then my schedule in "broken" state
        When I run mdb-katan-broken-schedules with namespace="core"
        Then monitoring returns "CRIT"
        # Message formatting we check in unittests,
        # here we just check, that our queries works well
        And monitoring message matches
        """
        ^.*schedule .* is broken. It fails on 1 clusters.*Shipment:.*$
        """
