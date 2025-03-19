Feature: Operations with account TVM2 roles
  Background: Empty accounts, roles tables
    Given an empty granted roles DB
      And an empty accounts DB


  Scenario: Granting role to a TVM2
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     |          |             |
     When the account "1" grants role "admin" to the TVM2 "123"
     Then the grant request succeeds
      And we have the following TVM2 granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
      And we have the following granted roles:
          | service_id |  role  |  grantee_uid  |


  Scenario: Granting role to a TVM2 within suspended account
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | suspended  |          |             |
     When the account "1" grants role "admin" to the TVM2 "123"
     Then the grant request fails with error "S3A03"


  Scenario: Granting role to a TVM2 within unknown account
     When the account "1" grants role "admin" to the TVM2 "123"
     Then the grant request fails with error "S3A02"


  Scenario: Granting already presented role to a TVM2 is idempotent
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     |          |             |
      And we have the following TVM2 granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
     When the account "1" grants role "admin" to the TVM2 "123"
     Then the grant request succeeds
      And we have the following TVM2 granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |


  Scenario: Removing TVM2 granted role
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     |          |             |
      And we have the following TVM2 granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
     When the account "1" removes role "admin" from the TVM2 "123"
     Then the grant request succeeds
      And we have the following TVM2 granted roles:
          | service_id |  role  |  grantee_uid  |


  Scenario: Removing granted role from suspended account
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     |          |             |
      And we have the following TVM2 granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
     When we suspend the account "1"
      And the account "1" removes role "admin" from the TVM2 "123"
     Then the grant request succeeds
      And we have the following TVM2 granted roles:
          | service_id |  role  |  grantee_uid  |


  Scenario: Removing absent role
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     |          |             |
      And an empty granted roles DB
     When the account "1" removes role "admin" from the TVM2 "123"
     Then the grant request succeeds


  Scenario: Removing absent role from unknown account
     When the account "1" removes role "admin" from the TVM2 "123"
     Then the grant request fails with error "S3A02"


  Scenario: Granting role from multiple accounts to the same TVM2
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     | 100      |             |
          | 22         | active     |          |             |
     When the account "1" grants role "admin" to the TVM2 "123"
      And the account "22" grants role "admin" to the TVM2 "123"
     Then we have the following TVM2 granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
          | 22         | admin  | 123           |


  Scenario: Granting role to multiple TVM2
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     |          |             |
     When the account "1" grants role "admin" to the TVM2 "123"
      And the account "1" grants role "admin" to the TVM2 "456"
     Then we have the following TVM2 granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
          | 1          | admin  | 456           |


  Scenario: Granting some roles to one TVM2
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     | 150      |             |
     When the account "1" grants role "admin" to the TVM2 "123"
      And the account "1" grants role "owner" to the TVM2 "123"
      And the account "1" grants role "reader" to the TVM2 "123"
      And the account "1" grants role "uploader" to the TVM2 "123"
     Then we have the following TVM2 granted roles:
          | service_id |  role    |  grantee_uid  |
          | 1          | admin    | 123           |
          | 1          | owner    | 123           |
          | 1          | reader   | 123           |
          | 1          | uploader | 123           |


  Scenario: Removing one of some granted roles
    Given we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     |          |             |
      And we have the following TVM2 granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
          | 1          | owner  | 123           |
     When the account "1" removes role "owner" from the TVM2 "123"
     Then the grant request succeeds
      And we have the following TVM2 granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 123           |
