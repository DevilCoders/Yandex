Feature: Testing metadata update
  Background: Set initial data
    Given empty DB

    Given buckets owner account "1"
      And a bucket with name "Columba"
      And we add multipart object "Phact" with following attributes
          """
          data_md5: 99999999-9999-9999-9999-999999999999
          mds_namespace: ns-9
          mds_couple_id: 999
          mds_key_version: 9
          mds_key_uuid: 99999999-9999-9999-9999-999999999999
          storage_class: 0

          parts:
             - part_id: 1
               mds_namespace: ns-1
               mds_couple_id: 1
               mds_key_version: 1
               mds_key_uuid: 11111111-1111-1111-1111-111111111111
               data_size: 1000000000
               data_md5: 11111111-1111-1111-1111-111111111111

             - part_id: 2
               mds_namespace: ns-2
               mds_couple_id: 2
               mds_key_version: 2
               mds_key_uuid: 22222222-2222-2222-2222-222222222222
               data_size: 2000000000
               data_md5: 22222222-2222-2222-2222-222222222222
          """
      And we add multipart object "AlKurud" with following attributes
          """
          data_md5: 88888888-8888-8888-8888-888888888888
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:
          storage_class: 1

          parts:
             - part_id: 1
               mds_namespace: ns-1
               mds_couple_id: 1
               mds_key_version: 1
               mds_key_uuid: 11111111-1111-1111-1111-111111111111
               data_size: 1000000000
               data_md5: 11111111-1111-1111-1111-111111111111

             - part_id: 2
               mds_namespace: ns-2
               mds_couple_id: 2
               mds_key_version: 2
               mds_key_uuid: 22222222-2222-2222-2222-222222222222
               data_size: 2000000000
               data_md5: 22222222-2222-2222-2222-222222222222
          """
      And we add object "Wazn" with following attributes
          """
          data_size: 1111
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
          storage_class: 2
          """

  Scenario: Update metadata of existent multipart object with nonempty metadata
    When we update object "Phact" metadata with following info
       """
       mds_namespace: ns-7
       mds_couple_id: 7
       mds_key_version: 7
       mds_key_uuid: 77777777-7777-7777-7777-777777777777
       """
    Then we get following object
       """
       data_md5: 99999999-9999-9999-9999-999999999999
       mds_namespace: ns-7
       mds_couple_id: 7
       mds_key_version: 7
       mds_key_uuid: 77777777-7777-7777-7777-777777777777

       parts:
          - part_id: 1
            mds_couple_id: 1
            mds_key_version: 1
            mds_key_uuid: 11111111-1111-1111-1111-111111111111
            data_size: 1000000000
            data_md5: 11111111-1111-1111-1111-111111111111

          - part_id: 2
            mds_couple_id: 2
            mds_key_version: 2
            mds_key_uuid: 22222222-2222-2222-2222-222222222222
            data_size: 2000000000
            data_md5: 22222222-2222-2222-2222-222222222222
       """
    When we list all in the storage delete queue
    Then we get the following deleted object(s) as a result
       """
       - name: Phact
         data_size: 0
         data_md5: 99999999-9999-9999-9999-999999999999
         mds_namespace: ns-9
         mds_couple_id: 999
         mds_key_version: 9
         mds_key_uuid: 99999999-9999-9999-9999-999999999999
       """

  Scenario: Update metadata of existent multipart object with empty metadata
    When we update object "AlKurud" metadata with following info
       """
       mds_namespace: ns-7
       mds_couple_id: 7
       mds_key_version: 7
       mds_key_uuid: 77777777-7777-7777-7777-777777777777
       """
    Then we get following object
       """
       data_md5: 88888888-8888-8888-8888-888888888888
       mds_namespace: ns-7
       mds_couple_id: 7
       mds_key_version: 7
       mds_key_uuid: 77777777-7777-7777-7777-777777777777

       parts:
          - part_id: 1
            mds_couple_id: 1
            mds_key_version: 1
            mds_key_uuid: 11111111-1111-1111-1111-111111111111
            data_size: 1000000000
            data_md5: 11111111-1111-1111-1111-111111111111

          - part_id: 2
            mds_couple_id: 2
            mds_key_version: 2
            mds_key_uuid: 22222222-2222-2222-2222-222222222222
            data_size: 2000000000
            data_md5: 22222222-2222-2222-2222-222222222222
       """
    When we list all in the storage delete queue
    Then we get the following deleted object(s) as a result
       """
       """

  Scenario: Remove metadata of existent multipart object with nonempty metadata
    When we update object "Phact" metadata with following info
       """
       """
    Then we get following object
       """
       data_md5: 99999999-9999-9999-9999-999999999999
       mds_namespace:
       mds_couple_id:
       mds_key_version:
       mds_key_uuid:

       parts:
          - part_id: 1
            mds_couple_id: 1
            mds_key_version: 1
            mds_key_uuid: 11111111-1111-1111-1111-111111111111
            data_size: 1000000000
            data_md5: 11111111-1111-1111-1111-111111111111

          - part_id: 2
            mds_couple_id: 2
            mds_key_version: 2
            mds_key_uuid: 22222222-2222-2222-2222-222222222222
            data_size: 2000000000
            data_md5: 22222222-2222-2222-2222-222222222222
       """
    When we list all in the storage delete queue
    Then we get the following deleted object(s) as a result
       """
       - name: Phact
         data_size: 0
         data_md5: 99999999-9999-9999-9999-999999999999
         mds_namespace: ns-9
         mds_couple_id: 999
         mds_key_version: 9
         mds_key_uuid: 99999999-9999-9999-9999-999999999999
       """

  Scenario: Update simple object metadata
    When we update object "Wazn" metadata with following info
       """
       mds_namespace: ns-7
       mds_couple_id: 7
       mds_key_version: 7
       mds_key_uuid: 77777777-7777-7777-7777-777777777777
       """
    Then we get an error with code "0A000"

  Scenario: Update nonexistent object metadata
    When we update object "non_existent_object" metadata with following info
       """
       mds_namespace: ns-7
       mds_couple_id: 7
       mds_key_version: 7
       mds_key_uuid: 77777777-7777-7777-7777-777777777777
       """
    Then we get an error with code "S3K01"

  Scenario: Change storage_class
    When we change timestamps in chunks counters queue to "2020-01-22 19:00:00.00000+00"
     And we refresh all statistic
    When we update object "Phact" metadata with following info
       """
       storage_class: 4
       """
    Then we get following object
       """
       data_md5: 99999999-9999-9999-9999-999999999999
       mds_namespace: ns-9
       mds_couple_id: 999
       mds_key_version: 9
       mds_key_uuid: 99999999-9999-9999-9999-999999999999
       storage_class: 4

       parts:
          - part_id: 1
            mds_couple_id: 1
            mds_key_version: 1
            mds_key_uuid: 11111111-1111-1111-1111-111111111111
            data_size: 1000000000
            data_md5: 11111111-1111-1111-1111-111111111111

          - part_id: 2
            mds_couple_id: 2
            mds_key_version: 2
            mds_key_uuid: 22222222-2222-2222-2222-222222222222
            data_size: 2000000000
            data_md5: 22222222-2222-2222-2222-222222222222
       """
   When we change timestamps in chunks counters queue to "2020-01-22 20:30:00.00000+00"
    And we refresh all statistic
   Then buckets usage contains rows:
      """
        - storage_class: 0
          size_change: -3000000000
          byte_secs: -5400000000000
          start_ts: '2020-01-22 20:00:00'
        - storage_class: 4
          size_change: 3000000000
          byte_secs: 5400000000000
          start_ts: '2020-01-22 20:00:00'
      """
