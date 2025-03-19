Feature: MongoDB stepdown

  Background: Wait for working infrastructure
    Given a mongodb hosts mongodb01 mongodb02 mongodb03
      And mongodb is working on all mongodb hosts
      And replset initialized on mongodb01
      And auth initialized on mongodb01
      And mongodb01 role is one of PRIMARY
      And oplog size is 990 MB on all mongodb hosts
      And all mongodb hosts are synchronized

  Scenario: Non electable secondaries: stepdown and return primary after replSetStepDown
    Given ReplSet has test mongodb data test1
    When we freeze mongodb02 for 120 seconds
    And we freeze mongodb03 for 120 seconds
    Then we run stepdown on mongodb01 and exit code is 40
    And we freeze mongodb02 for 0 seconds
    And we freeze mongodb03 for 0 seconds
    And mongodb01 role is one of PRIMARY
    And mongodb02 role is one of SECONDARY
    And mongodb03 role is one of SECONDARY

  Scenario: Lagged secondaries: stepdown hangs and fails after secondaryCatchUpPeriodSecs
    Given ReplSet has test mongodb data test2
    When we fsyncLock mongodb02
    And we fsyncLock mongodb03
    And we insert test mongodb data test3 with write concern 1
    Then we run stepdown on mongodb01 and exit code is 20
    And we fsyncUnlock mongodb02
    And we fsyncUnlock mongodb03
    And mongodb01 role is one of PRIMARY
    And mongodb02 role is one of SECONDARY
    And mongodb03 role is one of SECONDARY

  Scenario: Lagged secondaries: stepdown does not run because of FreshSecondariesCheck check fail
    When we fsyncLock mongodb02
    And we fsyncLock mongodb03
    And we insert test mongodb data test4 with write concern 1
    And we sleep for 30 seconds
    And we insert test mongodb data test5 with write concern 1
    Then we run stepdown on mongodb01 and exit code is 10
    And we fsyncUnlock mongodb02
    And we fsyncUnlock mongodb03
    And mongodb01 role is one of PRIMARY
    And mongodb02 role is one of SECONDARY
    And mongodb03 role is one of SECONDARY

  Scenario: Stepdown works
    Then we run stepdown on mongodb01 and exit code is 0
    And mongodb01 role is one of SECONDARY
    And mongodb02 role is one of PRIMARY SECONDARY
    And mongodb03 role is one of PRIMARY SECONDARY
