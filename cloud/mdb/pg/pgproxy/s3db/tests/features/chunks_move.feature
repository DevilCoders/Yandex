Feature: Moving chunk in other shard

  Background: Set buckets owner account

  Scenario: Getting chunk to move
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "foo"
     When we add 100 randomly generated objects
      And we refresh all statistic
     When we split chunks bigger than "5" object(s) on "0" db
      And we split chunks bigger than "5" object(s) on "1" db
      And we refresh all statistic
    When we split chunks bigger than "5" object(s) on "0" db
      And we split chunks bigger than "5" object(s) on "1" db
      And we refresh all statistic
    Then we get objects count by shard on "1" meta db:
      """
        - c: 100
        - c: 0
      """
     And there are "100" object(s) on "0" db
     And there are "0" object(s) on "1" db

  Scenario: Move default chunk
   Given buckets owner account "1"
     And a bucket with name "foo"
    When we run chunk_mover on "1" meta db
     And we refresh all statistic
    Then we get objects count by shard on "1" meta db:
      """
        - c: 75
        - c: 25
      """
     And we have no prepared transactions
     And we have no transactions with application_name like s3_script_%
     And there are "75" object(s) on "0" db
     And there are "25" object(s) on "1" db
    When we get first object name on "1" db
    Then we can get info about this object
     And we have no errors in counters

  Scenario: Move default chunk with zero copy timeout fails
   Given buckets owner account "1"
     And a bucket with name "foo"
    When we run chunk_mover with zero copy timeout on "1" meta db
    Then we exit with code "3"
    When we refresh all statistic
    Then we get objects count by shard on "1" meta db:
      """
        - c: 75
        - c: 25
      """
     And we have no prepared transactions
     And we have no transactions with application_name like s3_script_%
     And there are "75" object(s) on "0" db
     And there are "25" object(s) on "1" db
    When we get first object name on "1" db
    Then we can get info about this object
     And we have no errors in counters

  Scenario: Move custom chunk by queue
    When add to chunks moving queue one record on "1" meta db
    Then there are "1" object(s) in chunks moving queues
    When we move all chunks from queue on "1" meta db
     And we refresh all statistic
    Then we get objects count by shard on "1" meta db:
      """
        - c: 50
        - c: 50
      """
     And we have no prepared transactions
     And we have no transactions with application_name like s3_script_%
     And there are "0" object(s) in chunks moving queues
     And there are "50" object(s) on "0" db
     And there are "50" object(s) on "1" db
     And we have no errors in counters

  Scenario: Nothing to move
    When add to chunks moving queue one record on "1" meta db
    Then there are "0" object(s) in chunks moving queues

  Scenario: Check object availability after moving
    Given buckets owner account "1"
      And a bucket with name "ChunkMoving"
    When we add object "test_object" with following attributes:
      """
      data_size: 1111
      data_md5: 11111111-1111-1111-1111-111111111111
      mds_namespace: ns-1
      mds_couple_id: 111
      mds_key_version: 1
      mds_key_uuid: 11111111-1111-1111-1111-111111111111
      """
    Given a multipart upload in bucket "ChunkMoving" for object "test_multipart_object"
     When we upload part for object "test_multipart_object":
      """
      part_id: 1
      data_size: 5242880
      data_md5: 55555555-5555-5555-5555-555555555555
      mds_namespace:
      mds_couple_id: 555
      mds_key_version: 1
      mds_key_uuid: 55555555-5555-5555-5555-555555555555
      """
     When we upload part for object "test_multipart_object":
      """
      part_id: 2
      data_size: 6666
      data_md5: 66666666-6666-6666-6666-666666666666
      mds_namespace:
      mds_couple_id: 666
      mds_key_version: 1
      mds_key_uuid: 76666666-6666-6666-6666-666666666666
      """
    When we complete the following multipart upload:
      """
      name: test_multipart_object
      data_md5: 55555555-5555-5555-5555-555555555555
      parts_data:
          - part_id: 1
            data_md5: 55555555-5555-5555-5555-555555555555
          - part_id: 2
            data_md5: 66666666-6666-6666-6666-666666666666
      """
    Then we get no error
      and bucket "ChunkMoving" has "1" multipart object(s) of size "5249546"
    When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
    Then we get following objects:
      """
      - name: test_multipart_object
        data_size: 5249546
        data_md5: 55555555-5555-5555-5555-555555555555
        mds_namespace:
        parts_count: 2

      - name: test_object
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        parts_count:
      """
    When we push to move queue this chunk from "0" to "1" shard on "0" meta db
     And we move all chunks from queue on "0" meta db
     And we get chunk info for object "test_object"
    Then we get a chunk with following attributes:
      """
      shard_id: 1
      """
     And we have no prepared transactions
     And we have no transactions with application_name like s3_script_%
    When we get info about object with name "test_object"
    Then our object has attributes:
      """
      name: test_object
      data_size: 1111
      data_md5: 11111111-1111-1111-1111-111111111111
      mds_namespace: ns-1
      mds_couple_id: 111
      mds_key_version: 1
      mds_key_uuid: 11111111-1111-1111-1111-111111111111
      null_version: True
      delete_marker: False
      parts_count:
      parts:
      """
    When we get info about object with name "test_multipart_object"
    Then our object has attributes:
      """
      name: test_multipart_object
      data_size: 5249546
      data_md5: 55555555-5555-5555-5555-555555555555
      mds_namespace:
      parts_count: 2
      """
     And there are "0" object(s) in chunks moving queues
     And we have no errors in counters
    When we list object parts for object "test_multipart_object"
    Then we get following object parts:
        """
        - name: test_multipart_object
          part_id: 1
          data_size: 5242880
          data_md5: 55555555-5555-5555-5555-555555555555
          mds_namespace:
          mds_couple_id: 555
          mds_key_version: 1
          mds_key_uuid: 55555555-5555-5555-5555-555555555555

        - name: test_multipart_object
          part_id: 2
          data_size: 6666
          data_md5: 66666666-6666-6666-6666-666666666666
          mds_namespace:
          mds_couple_id: 666
          mds_key_version: 1
          mds_key_uuid: 76666666-6666-6666-6666-666666666666
        """

  Scenario: Check multipart completing after chunk moving
    Given buckets owner account "1"
      And a bucket with name "ChunkMoving"
      And a multipart upload in bucket "ChunkMoving" for object "test_multipart_object_2"
      When we upload part for object "test_multipart_object_2":
      """
      part_id: 1
      data_size: 222
      data_md5: 22222222-2222-2222-2222-222222222222
      mds_namespace:
      mds_couple_id: 222
      mds_key_version: 1
      mds_key_uuid: 22222222-2222-2222-2222-222222222222
      """
      Then bucket "ChunkMoving" has "1" object part(s) of size "222"
      When we get chunk info for object "test_multipart_object_2"
      Then we get a chunk with following attributes:
      """
      shard_id: 1
      """
    When we push to move queue this chunk from "1" to "0" shard on "0" meta db
     And we move all chunks from queue on "0" meta db
    And we complete the following multipart upload:
      """
      name: test_multipart_object_2
      data_md5: 22222222-2222-2222-2222-222222222222
      parts_data:
          - part_id: 1
            data_md5: 22222222-2222-2222-2222-222222222222
      """
     And we get chunk info for object "test_multipart_object_2"
    Then we get a chunk with following attributes:
      """
      shard_id: 0
      """
     And we have no errors in counters

  Scenario: Parallel mover
    When we push to move queue all chunks on "1" meta db
     And we move all chunks from queue on "1" meta db
     And we refresh all statistic
    Then we get objects count by shard on "1" meta db:
      """
        - c: 50
        - c: 50
      """
     And we have no errors in counters
