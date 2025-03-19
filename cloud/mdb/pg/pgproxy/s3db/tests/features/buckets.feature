Feature: Operations with buckets

  Scenario: Adding a bucket
    Given empty DB
     When we add a bucket with name "foo" of account "1"
     Then we get bucket with following attributes:
         """
         name: foo
         versioning: disabled
         banned: False
         service_id: 1
         anonymous_read: False
         anonymous_list: False
         """

  Scenario: List buckets after adding
    Given empty DB
      And a bucket with name "bucket-1" of account "1"
      And a bucket with name "bucket-2" of account "2"
     When we list all public buckets
     Then we get the following buckets:
         """
         """
     When we list all buckets of account "1"
     Then we get the following buckets:
         """
         - name: bucket-1
           versioning: disabled
           banned: False
           service_id: 1
           anonymous_read: False
           anonymous_list: False
         """
     When we list all buckets of account "2"
     Then we get the following buckets:
         """
         - name: bucket-2
           versioning: disabled
           banned: False
           service_id: 2
           anonymous_read: False
           anonymous_list: False
         """

  Scenario: List chunks on db after adding a bucket
    Given empty DB
     When we add a bucket with name "bucket-1" of account "1"
     Then chunks for object "bar" on meta and db are the same
     When we get chunk info for object "bar"
      And we delete this chunk
     Then we get no error
     When we run chunk_creator on "0" meta db
      And we run chunk_creator on "1" meta db
     Then chunks for object "bar" on meta and db are the same
     When we add a bucket with name "bucket-2" of account "1"
     Then chunks for object "bar" on meta and db are the same
     When we get chunk info for object "bar"
      And we run chunk_creator on "0" meta db
      And we run chunk_creator on "1" meta db
     Then chunks for object "bar" on meta and db are the same
     And we have no errors in counters


  Scenario: Adding duplicate of bucket
    Given empty DB
      And a bucket with name "foo" of account "1"
     When we add a bucket with name "foo" of account "1"
     Then we get an error with code "S3B05"
     When we add a bucket with name "foo" of account "2"
     Then we get an error with code "S3B02"
     When we add public bucket with name "foo"
     Then we get an error with code "S3A01"


  Scenario: Adding a bucket with short name
   Given buckets owner account "1"
    When we add a bucket with name "fo"
    Then we get an error with code "23514"


  Scenario: Adding a bucket with long name
   Given buckets owner account "1"
    When we add a bucket with name "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
    Then we get an error with code "23514"


  Scenario: Bucket info works as expected
    Given empty DB
    Given buckets owner account "1"
    Given a bucket with name "foo"
    When we get bucket info
    Then we get bucket with following attributes:
        """
        name: foo
        versioning: disabled
        banned: False
        service_id: 1
        anonymous_read: False
        anonymous_list: False
        """


  Scenario: Getting info about non-existent bucket
    Given empty DB
    When we get info about bucket with name "bar"
    Then we get an error with code "S3B01"


  Scenario: Check default max size for new bucket
    Given empty DB
    Given a bucket with name "restricted" of account "1"
    Then we get bucket with following attributes:
        """
        name: restricted
        versioning: disabled
        banned: False
        service_id: 1
        max_size:
        anonymous_read: False
        anonymous_list: False
        """


  Scenario: Check explicit max size for new bucket
    Given empty DB
    Given a bucket with name "restricted" of account "1" and max size "100500"
    Then we get bucket with following attributes:
        """
        name: restricted
        versioning: disabled
        banned: False
        service_id: 1
        max_size: 100500
        anonymous_read: False
        anonymous_list: False
        """


  Scenario: Successfully change max size for bucket
    Given empty DB
    Given buckets owner account "1"
    Given a bucket with name "restricted"
    When we change bucket attributes to:
        """
        max_size: 500100
        """
    When we get bucket info
    Then we get bucket with following attributes:
        """
        name: restricted
        versioning: disabled
        banned: False
        service_id: 1
        max_size: 500100
        anonymous_read: False
        anonymous_list: False
        """
    When we change bucket attributes to:
        """
        max_size: 666666
        """
    When we get bucket info
    Then we get bucket with following attributes:
        """
        name: restricted
        versioning: disabled
        banned: False
        service_id: 1
        max_size: 666666
        anonymous_read: False
        anonymous_list: False
        """

  Scenario: Successfully change anonymous attributes for bucket
    Given empty DB
    Given buckets owner account "1"
    Given a bucket with name "bucket"
    When we get bucket info
    Then we get bucket with following attributes:
        """
        name: bucket
        versioning: disabled
        banned: False
        service_id: 1
        max_size:
        anonymous_read: False
        anonymous_list: False
        """
    When we change bucket attributes to:
        """
        anonymous_read: True
        """
    When we get bucket info
    Then we get bucket with following attributes:
        """
        name: bucket
        versioning: disabled
        banned: False
        service_id: 1
        max_size:
        anonymous_read: True
        anonymous_list: False
        """
    When we change bucket attributes to:
        """
        anonymous_list: True
        """
    When we get bucket info
    Then we get bucket with following attributes:
        """
        name: bucket
        versioning: disabled
        banned: False
        service_id: 1
        max_size:
        anonymous_read: True
        anonymous_list: True
        """
    When we change bucket attributes to:
        """
        anonymous_read: False
        anonymous_list: False
        """
    When we get bucket info
    Then we get bucket with following attributes:
        """
        name: bucket
        versioning: disabled
        banned: False
        service_id: 1
        max_size:
        anonymous_read: False
        anonymous_list: False
        """

  Scenario: Check return value of modifying bucket command
    Given empty DB
    Given buckets owner account "1"
    Given a bucket with name "restricted"
    When we change bucket attributes to:
        """
        max_size: 500100
        """
    Then we get bucket with following attributes:
        """
        name: restricted
        versioning: disabled
        banned: False
        service_id: 1
        max_size: 500100
        anonymous_read: False
        anonymous_list: False
        """

  Scenario: Successfully change service_id of the bucket
    Given empty DB
    Given buckets owner account "1"
    Given a bucket with name "moved"
    When we get bucket info
    Then we get bucket with following attributes:
        """
        name: moved
        versioning: disabled
        banned: False
        service_id: 1
        max_size:
        anonymous_read: False
        anonymous_list: False
        """
    When we change bucket attributes to:
        """
        service_id: 2
        """
    Then we get bucket with following attributes:
        """
        name: moved
        versioning: disabled
        banned: False
        service_id: 2
        max_size:
        anonymous_read: False
        anonymous_list: False
        """
    When we get bucket info
    Then we get bucket with following attributes:
        """
        name: moved
        versioning: disabled
        banned: False
        service_id: 2
        max_size:
        anonymous_read: False
        anonymous_list: False
        """

  Scenario: Fail to change service_id of non-existent bucket
    Given empty DB
    Given buckets owner account "1"
    When we change bucket "foo" attributes to:
        """
        service_id: 2
        """
    Then we get an error with code "S3B01"


  Scenario: Getting object chunk works
    Given empty DB
    Given buckets owner account "1"
    Given a bucket with name "foo"
    When we get chunk info for object "bar"
    Then we get a chunk with following attributes:
        """
        read_only: False
        start_key:
        end_key:
        """

  Scenario: Modifying non-existent bucket
    Given empty DB
    When we change bucket "bar" attributes to:
        """
        versioning: enabled
        """
    Then we get an error with code "S3B01"


  Scenario: Enabling versioning for bucket
    Given empty DB
    Given buckets owner account "1"
    Given a bucket with name "foo"
    When we change bucket attributes to:
        """
        versioning: enabled
        """
    Then we get bucket with following attributes:
        """
        name: foo
        versioning: enabled
        banned: False
        service_id: 1
        """


  Scenario: Invalid disabling versioning for bucket
    Given empty DB
    Given buckets owner account "1"
    Given a bucket with name "foo"
    When we change bucket attributes to:
        """
        versioning: enabled
        """
      And we change bucket attributes to:
        """
        versioning: disabled
        """
    Then we get an error with code "22004"


  Scenario: Dropping non-existent bucket
    Given empty DB
    When we drop a bucket with name "bar"
    Then we get an error with code "S3B01"


  Scenario: Dropping existent bucket
    Given empty DB
    Given buckets owner account "1"
      And a bucket with name "foo"
    When we drop a bucket with name "foo"
     And we list all chunks in the bucket "foo" on meta db
    Then we get following chunks:
        """
        """
    When we list all buckets
    Then we get the following buckets:
        """
        """
    When we list all chunks in delete queue
    Then we get following chunks:
        """
        - start_key: NULL
          end_key: NULL
          shard_id: 0
        """
    When we run chunk_purger on "0" meta db
     And we run chunk_purger on "1" meta db
    When we list all chunks on db
    Then we get following chunks:
        """
        """
    When we list all chunks in delete queue
    Then we get following chunks:
        """
        """
    When we refresh all statistic
    Then we have zero counters for bucket "foo"
     And we have no errors in counters


  Scenario: Dropping existent non-empty bucket with objects
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "foo"
    When we add object "bar" with following attributes:
        """
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    When we drop a bucket with name "foo"
    Then we get an error with code "S3B06"


  Scenario: Dropping existent non-empty bucket with object parts
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "foo"
      And a multipart upload in bucket "foo" for object "bar"
    When we upload part for object "bar":
        """
        part_id: 1
        data_size: 10485760
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we drop a bucket with name "foo"
    Then we get an error with code "S3B07"
