Feature: Operations with account roles
  Background: Empty accounts, roles tables
    Given an empty granted roles DB
      And an empty accounts DB


  Scenario: Granting role to a user
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     | 100      |             |
     When the account "1" grants role "admin" to the user "123"
     Then the grant request succeeds
      And we have the following granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |


  Scenario: Granting role to a user within suspended account
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | suspended  | 100      |             |
     When the account "1" grants role "admin" to the user "123"
     Then the grant request fails with error "S3A03"


  Scenario: Granting role to a user within unknown account
     When the account "1" grants role "admin" to the user "123"
     Then the grant request fails with error "S3A02"


  Scenario: Granting already presented role to a user is idempotent
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     | 100      |             |
      And we have the following granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
     When the account "1" grants role "admin" to the user "123"
     Then the grant request succeeds
      And we have the following granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |


  Scenario: Removing granted role
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     |          |             |
      And we have the following granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
     When the account "1" removes role "admin" from the user "123"
     Then the grant request succeeds
      And we have the following granted roles:
          | service_id |  role  |  grantee_uid  |


  Scenario: Removing granted role from suspended account
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     | 100      |             |
      And we have the following granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
     When we suspend the account "1"
      And the account "1" removes role "admin" from the user "123"
     Then the grant request succeeds
      And we have the following granted roles:
          | service_id |  role  |  grantee_uid  |


  Scenario: Removing absent role
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     |          |             |
      And an empty granted roles DB
     When the account "1" removes role "admin" from the user "123"
     Then the grant request succeeds


  Scenario: Removing absent role from unknown account
     When the account "1" removes role "admin" from the user "123"
     Then the grant request fails with error "S3A02"


  Scenario: Granting role from multiple accounts to the same user
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     | 100      |             |
          | 22         | active     |          |             |
     When the account "1" grants role "admin" to the user "123"
      And the account "22" grants role "admin" to the user "123"
     Then we have the following granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
          | 22         | admin  | 123           |


  Scenario: Granting role to multiple users
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     |          |             |
     When the account "1" grants role "admin" to the user "123"
      And the account "1" grants role "admin" to the user "456"
     Then we have the following granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
          | 1          | admin  | 456           |


  Scenario: Granting some roles to one user
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     | 100      |             |
     When the account "1" grants role "admin" to the user "123"
      And the account "1" grants role "owner" to the user "123"
      And the account "1" grants role "reader" to the user "123"
      And the account "1" grants role "uploader" to the user "123"
     Then we have the following granted roles:
          | service_id |  role    |  grantee_uid  |
          | 1          | admin    | 123           |
          | 1          | owner    | 123           |
          | 1          | reader   | 123           |
          | 1          | uploader | 123           |


  Scenario: Removing one of some granted roles
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     |          |             |
      And we have the following granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
          | 1          | owner  | 123           |
     When the account "1" removes role "owner" from the user "123"
     Then the grant request succeeds
      And we have the following granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
