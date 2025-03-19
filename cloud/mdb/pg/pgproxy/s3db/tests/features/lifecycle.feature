Feature: Testing metadata update
  Background: Set initial data

  Scenario: Abort multiple uploads
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "MultipartBucket"
    When we start multipart upload of object "test_multipart_object"
    When we upload part for object "test_multipart_object":
      """
      part_id: 1
      data_size: 1000000
      data_md5: 11111111-1111-1111-1111-111111111111
      mds_namespace: ns-1
      mds_couple_id: 11111
      mds_key_version: 2
      mds_key_uuid: 11111111-1111-1111-1111-111111111111
      """
    When we start multipart upload of object "test_multipart_object_2"
    When we start multipart upload of object "keep_multipart_object"
    When we upload part for object "keep_multipart_object":
      """
      part_id: 1
      data_size: 2000000
      data_md5: 22222222-2222-2222-2222-222222222222
      mds_namespace: ns-1
      mds_couple_id: 2222
      mds_key_version: 2
      mds_key_uuid: 22222222-2222-2222-2222-222222222222
      """
    When we abort following multipart uploads
      """
      [test_multipart_object, test_multipart_object_2]
      """
    Then we get following lifecycle results
      """
      - name: test_multipart_object
      - name: test_multipart_object_2
      """
      And bucket "MultipartBucket" has "1" object part(s) of size "2000000"
      And bucket "MultipartBucket" in delete queue has "1" object part(s) of size "1000000"
      And we have no errors in counters

  Scenario: Abort multiple uploads with duplicated keys
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "MultipartBucket"
    When we start multipart upload of object "test_multipart_object"
    When we upload part for object "test_multipart_object":
      """
      part_id: 1
      data_size: 1000000
      data_md5: 11111111-1111-1111-1111-111111111111
      mds_namespace: ns-1
      mds_couple_id: 11111
      mds_key_version: 2
      mds_key_uuid: 11111111-1111-1111-1111-111111111111
      """
    When we abort following multipart uploads
      """
      [test_multipart_object, test_multipart_object]
      """
    Then we get following lifecycle results
      """
      - name: test_multipart_object
      """
      And bucket "MultipartBucket" in delete queue has "1" object part(s) of size "1000000"
      And we have no errors in counters

  Scenario: Expire multiple objects
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "ExpireBucket"
      And we add object "expire_object_1" with following attributes
        """
        data_size: 10
        """
      And we add object "expire_object_2" with following attributes
        """
        data_size: 20
        """
      And we add object "keep_object" with following attributes
        """
        data_size: 30
        """
    When we expire following objects
      """
      [expire_object_1, expire_object_2]
      """
    Then we get following lifecycle results
      """
      - name: expire_object_1
      - name: expire_object_2
      """
      And bucket "ExpireBucket" has "1" object(s) of size "30"
      And bucket "ExpireBucket" in delete queue has "2" object(s) of size "30"
      And we have no errors in counters

  Scenario: Transfer multiple objects
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "TransferBucket"
      And we add object "transfer_object_1" with following attributes
        """
        data_size: 10
        storage_class: 0
        """
      And we add object "transfer_object_2" with following attributes
        """
        data_size: 20
        storage_class: 1
        """
      And we add object "transferred_object" with following attributes
        """
        data_size: 30
        storage_class: 3
        """
      And we add object "keep_object" with following attributes
        """
        data_size: 40
        storage_class: 0
        """
    Then bucket "TransferBucket" has "4" object(s) in chunks counters queue
    When we transfer following objects to storage class "3"
      """
      [transfer_object_1, transfer_object_2, transferred_object]
      """
    Then we get following lifecycle results
      """
      - name: transfer_object_1
      - name: transfer_object_2
      """
    When we list all objects in a bucket
    Then we get following objects:
      """
        - name: keep_object
          storage_class: 0
        - name: transfer_object_1
          storage_class: 3
        - name: transfer_object_2
          storage_class: 3
        - name: transferred_object
          storage_class: 3
      """
      And bucket "TransferBucket" has "8" object(s) in chunks counters queue
      And we have no errors in counters

  Scenario: Expire multiple objects in versioned bucket
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "ExpireBucket"
      And we enable bucket versioning
      And we add object "expire_object_1" with following attributes
        """
        data_size: 10
        """
      And we add object "expire_object_2" with following attributes
        """
        data_size: 20
        """
      And we add object "keep_object" with following attributes
        """
        data_size: 100
        """
    When we expire following objects
      """
      [expire_object_1, expire_object_2]
      """
    Then we get following lifecycle results
      """
      - name: expire_object_1
      - name: expire_object_2
      """
     And bucket "ExpireBucket" has "5" object(s) of size "160"
     And we have no errors in counters

  Scenario: Expire multiple noncurrent objects in versioned bucket
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "ExpireBucket"
      And we enable bucket versioning
      And we add object "expire_object_1" with following attributes
        """
        data_size: 10
        """
      And we add object "expire_object_1" with following attributes
        """
        data_size: 100
        """
      And we add object "expire_object_2" with following attributes
        """
        data_size: 20
        """
      And we add object "expire_object_2" with following attributes
        """
        data_size: 200
        """
      And we add object "keep_object" with following attributes
        """
        data_size: 40
        """
      And we add object "keep_object" with following attributes
        """
        data_size: 400
        """
    When we expire following noncurrent versions
      """
      [expire_object_1, expire_object_2]
      """
    Then we get following lifecycle results
      """
      - name: expire_object_1
      - name: expire_object_2
      """
      And bucket "ExpireBucket" has "4" object(s) of size "740"
      And bucket "ExpireBucket" in delete queue has "2" object(s) of size "30"
      And we have no errors in counters

  Scenario: Transfer multiple noncurrent objects in versioned bucket
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "TransferBucket"
      And we enable bucket versioning
      And we add object "transfer_object_1" with following attributes
        """
        data_size: 10
        storage_class: 0
        """
      And we add object "transfer_object_1" with following attributes
        """
        data_size: 100
        storage_class: 4
        """
      And we add object "transfer_object_2" with following attributes
        """
        data_size: 20
        storage_class: 1
        """
      And we add object "transfer_object_2" with following attributes
        """
        data_size: 200
        storage_class: 4
        """
      And we add object "transferred_object" with following attributes
        """
        data_size: 30
        storage_class: 3
        """
      And we add object "transferred_object" with following attributes
        """
        data_size: 300
        storage_class: 4
        """
      And we add object "keep_object" with following attributes
        """
        data_size: 40
        storage_class: 0
        """
      And we add object "keep_object" with following attributes
        """
        data_size: 400
        storage_class: 4
        """
    Then bucket "TransferBucket" has "8" object(s) in chunks counters queue
    When we transfer following noncurrent versions to storage class "3"
      """
      [transfer_object_1, transfer_object_2, transferred_object]
      """
    Then we get following lifecycle results
      """
      - name: transfer_object_1
      - name: transfer_object_2
      """
    When we list all noncurrent versions in a bucket
    Then we get following objects:
      """
        - name: keep_object
          storage_class: 0
        - name: transfer_object_1
          storage_class: 3
        - name: transfer_object_2
          storage_class: 3
        - name: transferred_object
          storage_class: 3
      """
      And bucket "TransferBucket" has "12" object(s) in chunks counters queue
      And we have no errors in counters

  Scenario: Cleanup delete markers in versioned bucket
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "CleanMarks"
      And we enable bucket versioning
      And we add delete marker with name "drop_1"
      And we add delete marker with name "drop_2"
      And we add delete marker with name "keep"
    When we cleanup following delete markers
      """
      [drop_1, drop_2]
      """
    Then we get following lifecycle results
      """
      - name: drop_1
      - name: drop_2
      """
    When we list all versions in a bucket
    Then we get following objects:
      """
        - name: keep
      """
    And bucket "CleanMarks" has "5" object(s) in chunks counters queue
    And we have no errors in counters
