Feature: Operations with accounts

  Scenario: Registering new service account
    Given an empty accounts DB
     When we register the account "1"
     Then the account "1" has "active" status


  Scenario: Registering account with implicit folder
    Given an empty accounts DB
     When we register the account "1"
     Then the account "1" has "active" status
      And the account "1" has "1" folder_id


  Scenario: Registering service account is idempotent
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     | 100      |             |
     When we register the account "1"
     Then the account "1" has "active" status


  Scenario: Suspending service account
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     | 100      |             |
     When we suspend the account "1"
     Then the account "1" has "suspended" status


  Scenario: Suspending service account is idempotent
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | suspended  | 100      |             |
     When we suspend the account "1"
     Then the account "1" has "suspended" status


  Scenario: Re-registering suspended service account
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | suspended  | 100      |             |
     When we register the account "1"
     Then the account "1" has "active" status

  Scenario: Listing of all accounts
    Given an empty accounts DB
     When we register the account "1"
      And we register the account "22" with max_size "100" and max_buckets "10"
      And we register the account "333" with max_size "150"
      And we suspend the account "22"
     Then we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     | None     | None        |
          | 22         | suspended  | 100      | 10          |
          | 333        | active     | 150      | None        |

  Scenario: Request info about unknown account
    Given an empty accounts DB
     When we request info about the account "1"
     Then the account request fails with error "S3A02"
