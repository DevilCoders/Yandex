Feature: Operations with access keys

  Scenario: Granting role to a user
    Given an empty accounts DB
      And we have the following accounts:
          | service_id |   status   | max_size | max_buckets |
          | 1          | active     | 10       |             |
          | 2          | active     | 100      |             |
      And we have the following granted roles:
          | service_id |  role  |  grantee_uid  |
          | 1          | admin  | 111           |
          | 2          | admin  | 222           |

     Then we have the following access keys:
        | service_id |   user_id  |        key_id        |   secret_token  | key_version |

    # Attempts to add access key to invalid service role fail with
    # S3A02 (NotSignedUp) error
     When we add access key:
        """
        service_id: 3
        user_id: 111
        role: admin
        key_id: AAAAAAAAAAAAAAAAAAAA
        secret_token: SECRET
        key_version: 1
        """
     Then the access key request fails with "S3A02" error
     When we add access key:
        """
        service_id: 1
        user_id: 222
        role: admin
        key_id: AAAAAAAAAAAAAAAAAAAA
        secret_token: SECRET
        key_version: 1
        """
     Then the access key request fails with "S3A02" error

    # Attempts to add malformed access key fail with check violation
    # error (23514)
     When we add access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: access_key_id
        secret_token: SECRET
        key_version: 1
        """
     Then the access key request fails with "23514" error

    # Add maximum number (5) of access keys for a service role
     When we successfully added access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 10000000000000000001
        secret_token: SECRET1
        key_version: 1
        """
      And we successfully added access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 10000000000000000002
        secret_token: SECRET2
        key_version: 1
        """
      And we successfully added access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 10000000000000000003
        secret_token: SECRET3
        key_version: 1
        """
      And we successfully added access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 10000000000000000004
        secret_token: SECRET4
        key_version: 1
        """
      And we successfully added access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 10000000000000000005
        secret_token: SECRET5
        key_version: 1
        """

    # Next attempt to add access key to the same service role fails with
    # S3A05 (TooManyAccessKeys) error
     When we add access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 10000000000000000006
        secret_token: SECRET6
        key_version: 1
        """
     Then the access key request fails with "S3A05" error

    # Add access key for another service role and check all access keys
     When we successfully added access key:
        """
        service_id: 2
        user_id: 222
        role: admin
        key_id: 20000000000000000001
        secret_token: SECRET1
        key_version: 1
        """
     Then we have the following access keys:
        | service_id |   user_id  |        key_id        |   secret_token  | key_version |
        | 1          | 111        | 10000000000000000001 | SECRET1         | 1           |
        | 1          | 111        | 10000000000000000002 | SECRET2         | 1           |
        | 1          | 111        | 10000000000000000003 | SECRET3         | 1           |
        | 1          | 111        | 10000000000000000004 | SECRET4         | 1           |
        | 1          | 111        | 10000000000000000005 | SECRET5         | 1           |
        | 2          | 222        | 20000000000000000001 | SECRET1         | 1           |

    # Attempts to delete nonexistent, invalid or another's access key fail
    # with S3A04 (InvalidAccessKeyId) error
     When we delete access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: AAAAAAAAAAAAAAAAAAAA
        secret_token: SECRET
        key_version: 1
        """
     Then the access key request fails with "S3A04" error
     When we delete access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: AAAAAAAAAAAAAAAAAAAA
        secret_token: SECRET
        key_version: 1
        """
     Then the access key request fails with "S3A04" error
     When we delete access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 20000000000000000001
        """
     Then the access key request fails with "S3A04" error

    # Delete some access keys and check listing
     When we successfully deleted access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 10000000000000000002
        """
      And we successfully deleted access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 10000000000000000003
        """
      And we successfully deleted access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 10000000000000000004
        """
      And we successfully deleted access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 10000000000000000005
        """
     Then we have the following access keys:
        | service_id |   user_id  |        key_id        |   secret_token  | key_version |
        | 1          | 111        | 10000000000000000001 | SECRET1         | 1           |
        | 2          | 222        | 20000000000000000001 | SECRET1         | 1           |

    # Delete remaining access keys and check listing
     When we successfully deleted access key:
        """
        service_id: 1
        user_id: 111
        role: admin
        key_id: 10000000000000000001
        """
      And we successfully deleted access key:
        """
        service_id: 2
        user_id: 222
        role: admin
        key_id: 20000000000000000001
        """
     Then we have the following access keys:
        | service_id |   user_id  |        key_id        |   secret_token  | key_version |
