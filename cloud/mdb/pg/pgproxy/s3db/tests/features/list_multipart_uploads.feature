Feature: List mulipart object parts

  Background: Set buckets owner account

  Scenario: List multipart object parts
    Given empty DB
      And buckets owner account "1"
      And a bucket with name "MultipartBucket"
    When we start multipart upload of object "test_multipart_object_list"
    Then we get the following multipart info:
        """
        - name: test_multipart_object_list
          part_id: 0
          data_size: 0
          data_md5:
        """
    When we upload part for object "test_multipart_object_list":
        """
        part_id: 1
        data_size: 1000000
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 11111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    Then we get the following multipart info:
        """
        - name: test_multipart_object_list
          part_id: 1
          data_size: 1000000
          data_md5: 11111111-1111-1111-1111-111111111111
        """
    When we upload part for object "test_multipart_object_list":
        """
        part_id: 2
        data_size: 1000000
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-1
        mds_couple_id: 22222
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then we get the following multipart info:
        """
        - name: test_multipart_object_list
          part_id: 2
          data_size: 1000000
          data_md5: 22222222-2222-2222-2222-222222222222
        """
