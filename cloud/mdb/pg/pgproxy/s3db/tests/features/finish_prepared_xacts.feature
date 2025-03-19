Feature: Moving chunk in other shard

  Background: Set buckets owner account

  Scenario: Checking rollback prepared xacts for splitter
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "foo"
     When we add 100 randomly generated objects
      And we refresh all statistic
    When we fail on step "0" while split chunks bigger than "5" object(s) on "0" db without attempts
     And we fail on step "0" while split chunks bigger than "5" object(s) on "1" db without attempts
     And we run finish_prepared_xacts on "0" meta db
     And we run finish_prepared_xacts on "1" meta db
    Then we have no prepared transactions
    When we refresh all statistic
    Then chunks counters for bucket "foo" on meta and db are the same
     And chunks bounds for bucket "foo" on meta and db are the same

  Scenario: Checking commit prepared xacts for splitter
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "foo"
     When we add 100 randomly generated objects
      And we refresh all statistic
    When we fail on step "1" while split chunks bigger than "5" object(s) on "0" db without attempts
     And we fail on step "1" while split chunks bigger than "5" object(s) on "1" db without attempts
     And we run finish_prepared_xacts on "0" meta db
     And we run finish_prepared_xacts on "1" meta db
    Then we have no prepared transactions
    When we refresh all statistic
    Then chunks counters for bucket "foo" on meta and db are the same
     And chunks bounds for bucket "foo" on meta and db are the same
     And we have no errors in counters

  Scenario: Checking rollback prepared xacts for mover
   Given buckets owner account "1"
     And a bucket with name "foo"
    When we fail after "-1" transaction commits while move chunk on "1" meta db
     And we run finish_prepared_xacts on "0" meta db
     And we run finish_prepared_xacts on "1" meta db
     And we refresh all statistic
    Then we get objects count by shard on "1" meta db:
      """
        - c: 100
        - c: 0
      """
     And we have no prepared transactions
     And there are "100" object(s) on "0" db
     And there are "0" object(s) on "1" db
    When we get first object name on "0" db
    Then we can get info about this object
     And we have no errors in counters

  Scenario: Checking commit prepared xacts for mover
   Given buckets owner account "1"
     And a bucket with name "foo"
    When we fail after "1" transaction commits while move chunk on "1" meta db
     And we run finish_prepared_xacts on "0" meta db
     And we run finish_prepared_xacts on "1" meta db
     And we refresh all statistic
    Then we get objects count by shard on "1" meta db:
      """
        - c: 50
        - c: 50
      """
     And we have no prepared transactions
     And there are "50" object(s) on "0" db
     And there are "50" object(s) on "1" db
    When we get first object name on "1" db
    Then we can get info about this object
     And we have no errors in counters

  Scenario: Checking prepared xacts for creator
   Given empty DB
    When we add a bucket with name "foo" of account "1"
    Then chunks for object "bar" on meta and db are the same
    When we get chunk info for object "bar"
     And we delete this chunk
    Then we get no error
    When we run chunk_creator on "0" meta db with fail on "0" step
     And we run chunk_creator on "1" meta db with fail on "0" step
     And we run finish_prepared_xacts on "0" meta db
     And we run finish_prepared_xacts on "1" meta db
    Then we have no prepared transactions
    When we refresh all statistic
    Then chunks counters for bucket "foo" on meta and db are the same
     And we have no errors in counters
