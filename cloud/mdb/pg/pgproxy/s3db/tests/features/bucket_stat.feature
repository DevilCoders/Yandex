Feature: Testing bucket stat
  Background: Set initial data
    Given empty DB

    Given buckets owner account "1"
      And a bucket with name "Versioned"
      And we enable bucket versioning
      And we add object "present" with following attributes
          """
          data_size: 1
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
          """
      And we add object "present" with following attributes
          """
          data_size: 100
          data_md5: 11111111-1111-1111-1111-111111111112
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111112
          """
      And we add delete marker with name "deleted___"
     When we refresh all statistic

    Given buckets owner account "1"
      And a bucket with name "Columba"
      And we add object "Phact" with following attributes
          """
          data_size: 1111
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
          """
     When we refresh all statistic

  Scenario: Request stat for existing bucket
    Then bucket stat for bucket "Columba":
          """
          - multipart_objects_count: 0
            multipart_objects_size: 0
            name: Columba
            objects_parts_count: 0
            objects_parts_size: 0
            service_id: 1
            simple_objects_count: 1
            simple_objects_size: 1111
          """
    Then we have "0" deleted objects of "0" size for bucket "Columba"
     And we have "0" active multipart upload(s) for bucket "Columba"

  Scenario: Request stat for versioned bucket
    Then bucket stat for bucket "Versioned":
          """
          - multipart_objects_count: 0
            multipart_objects_size: 0
            name: Versioned
            objects_parts_count: 0
            objects_parts_size: 0
            service_id: 1
            simple_objects_count: 3
            simple_objects_size: 111
          """
    Then we have "0" deleted objects of "0" size for bucket "Columba"
     And we have "0" active multipart upload(s) for bucket "Columba"

  Scenario: Request deleted stats
    When we drop object with name "Phact"
    Then bucket "Columba" in delete queue has "1" object(s) of size "1111"
     And bucket "Columba" in delete queue has "0" object part(s) of size "0"
    When we refresh all statistic
    Then we have "1" deleted objects of "1111" size for bucket "Columba"

  Scenario: Request active multipart stats
    When we start multipart upload of object "multipart_object"
     And we refresh all statistic
    Then we have "1" active multipart upload(s) for bucket "Columba"
    When we abort multipart upload for object "multipart_object"
    And we refresh all statistic
    Then we have "0" active multipart upload(s) for bucket "Columba"
    When we start multipart upload of object "multipart_object"
     And we upload part for object "multipart_object":
     """
     part_id: 1
     data_size: 44440000
     data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
     mds_namespace: ns-2
     mds_couple_id: 222
     mds_key_version: 2
     mds_key_uuid: 11111111-1111-1111-1111-111111111111
     """
     And we complete the following multipart upload:
     """
     name: multipart_object
     data_md5: 66666666-6666-6666-6666-666666666666
     parts_data:
         - part_id: 1
           data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
     """
     And we refresh all statistic
    Then we have "0" active multipart upload(s) for bucket "Columba"
     And we have no errors in counters

  Scenario: Request buckets stats with negative counters
    When we break bucket stat for bucket "Columba"
    Given refreshed buckets statistics on "0" meta db
     And refreshed buckets statistics on "1" meta db
    Then bucket stat for bucket "Columba":
          """
          - multipart_objects_count: 0
            multipart_objects_size: 0
            name: Columba
            objects_parts_count: 0
            objects_parts_size: 0
            service_id: 1
            simple_objects_count: 0
            simple_objects_size: 0
          """
