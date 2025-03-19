Feature: Billing counters updating
  Background: Set buckets owner account
   Given empty DB
     And buckets owner account "1"
     And a bucket with name "Billing"

  Scenario: Add and delete object in same hour
    When we insert into chunk counters queue row with size "100" at "2020-01-22 15:10:00.00000+00"
     And we insert into chunk counters queue row with size "-100" at "2020-01-22 15:50:00.00000+00"
     And we update chunk counters on "0" db
     And we update chunk counters on "1" db
    Then buckets usage contains rows:
      """
        - storage_class: 0
          size_change: 0
          byte_secs: 240000
          start_ts: '2020-01-22 15:00:00'
      """
     And we have no errors in counters

  Scenario: Moving chunk with object
    When we add object "first" with following attributes:
        """
        data_size: 100
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
     And we add object "second" with following attributes:
        """
        data_size: 200
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "Billing" has "2" object(s) of size "300"
     And bucket "Billing" has "0" multipart object(s) of size "0"
     And bucket "Billing" has "0" object part(s) of size "0"
    When we change timestamps in chunks counters queue to "2020-01-22 16:10:00.00000+00"
     And we refresh all statistic
    Then buckets usage contains rows:
      """
        - storage_class: 0
          size_change: 300
          byte_secs: 900000
          start_ts: '2020-01-22 16:00:00'
      """
     And we have no errors in counters

  Scenario: Moving chunk with object in versioned bucket
    When we enable bucket versioning
     And we add object "first" with following attributes:
        """
        data_size: 100
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
     And we add object "second" with following attributes:
        """
        data_size: 200
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
     And we add object "second" with following attributes:
        """
        data_size: 300
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 2
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
     And we add delete marker with name "zz_last_zz"
    Then bucket "Billing" has "4" object(s) of size "610"
     And bucket "Billing" has "0" multipart object(s) of size "0"
     And bucket "Billing" has "0" object part(s) of size "0"
    When we change timestamps in chunks counters queue to "2020-01-22 16:10:00.00000+00"
     And we refresh all statistic
    Then buckets usage contains rows:
      """
        - storage_class: 0
          size_change: 610
          byte_secs: 1830000
          start_ts: '2020-01-22 16:00:00'
      """
    When we split chunks bigger than "1" object(s) on "0" db
     And we split chunks bigger than "1" object(s) on "1" db
     And we refresh all statistic
    When we run chunk_mover on "0" meta db
     And we change timestamps in chunks counters queue to "2020-01-22 16:40:00.00000+00"
     And we refresh all statistic
    Then bucket usage at "2020-01-22 16:00:00+00" by shards:
      """
        - size_change: 100
          byte_secs: 1218000
        - size_change: 510
          byte_secs: 612000
      """
      # 1218000 = (610*30 + 100*20)*60
      # 612000 = 510*20*60
      # 1218000 + 612000 = 1830000
     And we have no errors in counters

  Scenario: Billing simple objects with different storage classes
    When we enable bucket versioning
     And we add object "first" with following attributes:
        """
        data_size: 50
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 0
        """
     And we add object "first" with following attributes:
        """
        data_size: 50
        data_md5: 11111111-1111-1111-1111-111111111112
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111112
        storage_class: 0
        """
     And we add object "second" with following attributes:
        """
        data_size: 100
        data_md5: 22222222-2222-2222-2222-222222222221
        mds_namespace: ns-2
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222221
        storage_class: 1
        """
     And we add object "second" with following attributes:
        """
        data_size: 100
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        storage_class: 1
        """
    Then bucket "Billing" has "4" object(s) of size "300"
     And bucket "Billing" has "0" multipart object(s) of size "0"
     And bucket "Billing" has "0" object part(s) of size "0"
    When we change timestamps in chunks counters queue to "2020-01-22 16:40:00.00000+00"
     And we refresh all statistic
    Then buckets usage contains rows:
      """
        - storage_class: 0
          size_change: 100
          byte_secs: 120000
        - storage_class: 1
          size_change: 200
          byte_secs: 240000
      """
     And we have no errors in counters

  Scenario: Billing multipart objects with different storage classes
    Given a bucket with name "Billing"
     When we start multipart upload of object "first_multipart":
         """
         storage_class: 0
         """
      And we upload part for object "first_multipart":
         """
         part_id: 1
         data_size: 500
         data_md5: 55555555-5555-5555-5555-555555555555
         mds_namespace: ns-5
         mds_couple_id: 555
         mds_key_version: 1
         mds_key_uuid: 55555555-5555-5555-5555-555555555555
         """
      And we complete the following multipart upload:
         """
         name: first_multipart
         data_md5: 55555555-5555-5555-5555-555555555555
         parts_data:
            - part_id: 1
              data_md5: 55555555-5555-5555-5555-555555555555
         """
    Given a bucket with name "Billing"
     When we start multipart upload of object "second_multipart":
         """
         storage_class: 1
         """
     When we upload part for object "second_multipart":
         """
         part_id: 1
         data_size: 400
         data_md5: 55555555-5555-5555-5555-555555555555
         mds_namespace: ns-5
         mds_couple_id: 555
         mds_key_version: 1
         mds_key_uuid: 55555555-5555-5555-5555-555555555555
         """
    Then bucket "Billing" has "0" object(s) of size "0"
     And bucket "Billing" has "1" multipart object(s) of size "500"
     And bucket "Billing" has "1" object part(s) of size "400"
    When we change timestamps in chunks counters queue to "2020-01-22 18:50:00.00000+00"
     And we refresh all statistic
    Then buckets usage contains rows:
      """
        - storage_class: 0
          size_change: 500
          byte_secs: 300000
          start_ts: '2020-01-22 18:00:00'
        - storage_class: 1
          size_change: 400
          byte_secs: 240000
          start_ts: '2020-01-22 18:00:00'
      """
    When we abort multipart upload for object "second_multipart"
     And we change timestamps in chunks counters queue to "2020-01-22 18:55:00.00000+00"
     And we refresh all statistic
    Then buckets usage contains rows:
      """
        - storage_class: 0
          size_change: 500
          byte_secs: 300000
          start_ts: '2020-01-22 18:00:00'
        - storage_class: 1
          size_change: 0
          byte_secs: 120000
          start_ts: '2020-01-22 18:00:00'
      """
     And we have no errors in counters

  Scenario: Replace simple object with other storage_class
    When we add object "obj" with following attributes:
        """
        data_size: 100
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 1
        """
     And we change timestamps in chunks counters queue to "2020-01-22 16:10:00.00000+00"
     And we refresh all statistic
     And we add object "obj" with following attributes:
        """
        data_size: 100
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 2
        """
    Then our object has attributes:
        """
        name: obj
        data_size: 100
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 2
        null_version: True
        delete_marker: False
        parts_count:
        parts:
        """
    When we change timestamps in chunks counters queue to "2020-01-22 16:20:00.00000+00"
     And we refresh all statistic
    Then buckets usage contains rows:
      """
        - storage_class: 1
          size_change: 0
          byte_secs: 60000
          start_ts: '2020-01-22 16:00:00'
        - storage_class: 2
          size_change: 100
          byte_secs: 240000
          start_ts: '2020-01-22 16:00:00'
      """
     And we have no errors in counters

  Scenario: Replace simple object with other storage_class and size
    When we add object "obj" with following attributes:
        """
        data_size: 100
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 1
        """
     And we change timestamps in chunks counters queue to "2020-01-22 16:10:00.00000+00"
     And we refresh all statistic
     And we add object "obj" with following attributes:
        """
        data_size: 150
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 2
        """
    Then our object has attributes:
        """
        name: obj
        data_size: 150
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 2
        null_version: True
        delete_marker: False
        parts_count:
        parts:
        """
    When we change timestamps in chunks counters queue to "2020-01-22 16:20:00.00000+00"
     And we refresh all statistic
    Then buckets usage contains rows:
      """
        - storage_class: 1
          size_change: 0
          byte_secs: 60000
          start_ts: '2020-01-22 16:00:00'
        - storage_class: 2
          size_change: 150
          byte_secs: 360000
          start_ts: '2020-01-22 16:00:00'
      """
     And we have no errors in counters

  Scenario: Replace simple object without storage_class with other storage_class
    When we add object "obj" with following attributes:
        """
        data_size: 100
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
     And we change timestamps in chunks counters queue to "2020-01-22 16:10:00.00000+00"
     And we refresh all statistic
     And we add object "obj" with following attributes:
        """
        data_size: 100
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 2
        """
    Then our object has attributes:
        """
        name: obj
        data_size: 100
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 2
        null_version: True
        delete_marker: False
        parts_count:
        parts:
        """
    When we change timestamps in chunks counters queue to "2020-01-22 16:20:00.00000+00"
     And we refresh all statistic
    Then buckets usage contains rows:
      """
        - storage_class: 0
          size_change: 0
          byte_secs: 60000
          start_ts: '2020-01-22 16:00:00'
        - storage_class: 2
          size_change: 100
          byte_secs: 240000
          start_ts: '2020-01-22 16:00:00'
      """
     And we have no errors in counters

  Scenario: Filling and updating bucket stats at meta
    When we add object "obj_1" with following attributes:
        """
        data_size: 100
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 1
        """
     And we add object "obj_2" with following attributes:
        """
        data_size: 200
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 2
        """
     And we change timestamps in chunks counters queue to "2020-01-22 15:40:00.00000+00"
     And create partition for "2020-01-22 00:00:00+00"
     And we refresh all statistic

     # Case: buckets_size is empty, buckets_usage is not empty
     And we update buckets size at "2020-01-22 16:00:00.00000+00"
    Then buckets size contains rows:
        """
        - storage_class: 1
          size: 100
          target_ts: "2020-01-22 16:00:00"
        - storage_class: 2
          size: 200
          target_ts: "2020-01-22 16:00:00"
        """
    When we change timestamps in chunks_counters to "2020-01-22 16:30:00.00000+00"
     And we fill buckets size at "2020-01-22 17:00:00.00000+00"
    Then buckets sizes for "2020-01-22 16:00:00.00000+00" and "2020-01-22 17:00:00.00000+00" are the same
    When we add object "obj_3" with following attributes:
        """
        data_size: 300
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 1
        """
     And we change timestamps in chunks counters queue to "2020-01-22 17:40:00.00000+00"
     And we refresh all statistic
     # Case: buckets_size isn't empty, buckets_usage not empty for storage_class 1 and empty for storage_class 2
     And we update buckets size at "2020-01-22 18:00:00.00000+00"
    Then buckets size contains rows:
        """
        - storage_class: 1
          size: 400
          target_ts: "2020-01-22 18:00:00"
        - storage_class: 2
          size: 200
          target_ts: "2020-01-22 18:00:00"
        """
    When we delete buckets usage at "2020-01-22 15:00:00.00000+00"
    # Case: sum of size changes != first row in buckets_size (buckets created before first filling buckets_size)
     And we update buckets size at "2020-01-22 19:00:00.00000+00"
    Then buckets size contains rows:
        """
        - storage_class: 1
          size: 400
          target_ts: "2020-01-22 19:00:00"
        - storage_class: 2
          size: 200
          target_ts: "2020-01-22 19:00:00"
        """

    When we drop object with name "obj_1"
     And we drop object with name "obj_2"
     And we drop object with name "obj_3"
     And we change timestamps in chunks counters queue to "2020-01-22 19:40:00.00000+00"
     And we drop a bucket with name "Billing"
     And we refresh all statistic
    Then we have no errors in counters

  Scenario: Reverse for MDB-10835 (missing recent s3.buckets_usage)
    When we add object "obj_1" with following attributes:
        """
        data_size: 100
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
     And we change timestamps in chunks counters queue to "2020-01-22 17:40:00.00000+00"
     And create partition for "2020-01-22 00:00:00+00"
     And we refresh all statistic
     And we change timestamps in chunks_counters to "2020-01-22 17:40:00.00000+00"
     And we fill buckets size at "2020-01-22 18:00:00.00000+00"
     And we add object "obj_2" with following attributes:
        """
        data_size: 200
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    And we change timestamps in chunks counters queue to "2020-01-22 18:40:00.00000+00"
    And we update chunk counters on "0" db
    And we update chunk counters on "1" db
    And we change timestamps in chunks_counters to "2020-01-22 18:40:00.00000+00"
    And we fill buckets size at "2020-01-22 19:00:00.00000+00"
    And we update chunks counters on "0" meta db
    And we update chunks counters on "1" meta db
   Then we have no errors in counters
