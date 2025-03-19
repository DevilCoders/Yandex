Feature: MongoDB resetup: 1P + 1S + 1 Stale

  Background: Wait for working infrastructure
    Given a mongodb hosts mongodb01 mongodb02 mongodb03
      And mongodb is working on all mongodb hosts
      And replset initialized on mongodb01
      And auth initialized on mongodb01
      And mongodb01 role is one of PRIMARY
      And oplog size is 990 MB on all mongodb hosts


  Scenario: Resetup does not handle alive replset
    Given ReplSet has test mongodb data test1
    When all mongodb pids are saved
    And we run resetup on mongodb01
    And we run resetup on mongodb02
    And we run resetup on mongodb03
    Then mongodb01 mongodb02 mongodb03 were running
    And we got same user data on all mongodb hosts

  Scenario: Resetup does nothing on alive replset with --continue
    Given ReplSet has test mongodb data test1
    When all mongodb pids are saved
    And we run resetup on mongodb01 with args "--continue"
    And we run resetup on mongodb02 with args "--continue"
    And we run resetup on mongodb03 with args "--continue"
    Then mongodb01 mongodb02 mongodb03 were running
    And we got same user data on all mongodb hosts


  Scenario: Resetup will handle alive replset with --force
    Given ReplSet has test mongodb data test1
    When all mongodb pids are saved
    And we run resetup on mongodb02 with args "-f"
    And we run resetup on mongodb03 with args "-f"
    And we run resetup on mongodb01 with args "-f"
    Then mongodb01 mongodb02 mongodb03 were restarted
    And we got same user data on all mongodb hosts
    And mongodb01 role is one of PRIMARY SECONDARY


  Scenario: Resetup does not handle stalled secondary with dry run mode
    Given ReplSet has test mongodb data test2
    When we make mongodb03 stale
    And all mongodb pids are saved
    And we run resetup on mongodb01 with args "-n"
    And we run resetup on mongodb02 with args "-n"
    And we run resetup on mongodb03 with args "-n"
    Then mongodb01 mongodb02 mongodb03 were running
    And mongodb03 role is one of RECOVERING
    And mongodb01 role is one of PRIMARY
    And mongodb02 role is one of SECONDARY


  Scenario: Resetup does not handle stalled secondary without quorum
    Given ReplSet has test mongodb data test3
    When we make mongodb03 stale
    And all mongodb pids are saved
    And we stop mongodb on mongodb02
    And we run resetup on mongodb01
    And we run resetup on mongodb03 and exit code is 11
    And we start mongodb on mongodb02
    Then mongodb02 was restarted
    And mongodb01 mongodb03 were running
    And mongodb03 role is one of RECOVERING
    And mongodb01 role is one of PRIMARY
    And mongodb02 role is one of SECONDARY


  Scenario: Resetup does not handle stalled secondary without quorum with --force
    Given ReplSet has test mongodb data test3
    When we make mongodb03 stale
    And all mongodb pids are saved
    And we stop mongodb on mongodb02
    And we run resetup on mongodb03 with args "-f" and exit code is 11
    And we start mongodb on mongodb02
    Then mongodb02 was restarted
    And mongodb01 mongodb03 were running
    And mongodb03 role is one of RECOVERING
    And mongodb01 role is one of PRIMARY
    And mongodb02 role is one of SECONDARY


  Scenario: Resetup does not handle stalled secondary without primary
    When we make mongodb03 stale
    And all mongodb pids are saved
    And we stop mongodb on mongodb01
    And we stop mongodb on mongodb02
    And we run resetup on mongodb03 and exit code is 1
    And we start mongodb on mongodb01
    And we start mongodb on mongodb02
    Then mongodb01 mongodb02 was restarted
    And mongodb03 was running
    And mongodb03 role is one of RECOVERING
    And mongodb01 role is one of PRIMARY
    And mongodb02 role is one of SECONDARY


  Scenario: Resetup resyncs stalled secondary
    Given ReplSet has test mongodb data test4
    When we make mongodb03 stale
    And all mongodb pids are saved
    And we run resetup on mongodb01
    And we run resetup on mongodb02
    And we run resetup on mongodb03
    Then mongodb03 was restarted
    And mongodb01 mongodb02 were running
    And all mongodb hosts are synchronized
    And we got same user data on all mongodb hosts
