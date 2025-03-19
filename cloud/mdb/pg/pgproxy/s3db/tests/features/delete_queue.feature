Feature: Work with dropped objects on shard

  Background: Connect to some shard
      Given empty DB
      Given a bucket with name "delete_queue" of account "1"

  Scenario: We don't delete objects under delay from storage_delete_queue
       When we add object "object_test_delay_key" with following attributes:
        """
        data_size: 10
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 1
        """
       When we drop object with name "object_test_delay_key"
       Then our object has attributes:
        """
        data_size: 10
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we list up to "10" deleted for at least "1 second" object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        """
       When we wait "1.0" seconds
       When we list up to "10" deleted for at least "1 second" object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: object_test_delay_key
          data_size: 10
          part_id:
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 2
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
          storage_class: 1
        """


  Scenario: Only dropped objects fall into storage delete queue
       When we add object "1mds_key_version" with following attributes:
        """
        data_size: 03
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 321
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we add object "mds_key_version2" with following attributes:
        """
        data_size: 30
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
       When we add object "empty_object" with following attributes:
        """
        data_size: 0
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """

       Then bucket "delete_queue" has "3" object(s) of size "33"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object part(s) of size "0"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        """

       When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
       Then we get following objects:
        """
        - name: 1mds_key_version
          data_size: 03
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 321
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: empty_object
          data_size: 0
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 1234
          mds_key_version: 2
          mds_key_uuid: 33333333-3333-3333-3333-333333333333

        - name: mds_key_version2
          data_size: 30
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
        And we have no errors in counters


  Scenario: We don't delete objects, which was dropped less than specified amount of days
       When we add object "mds_key_version2" with following attributes:
        """
        data_size: 30
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
       When we drop object with name "mds_key_version2"
       When we add object "1mds_key_version" with following attributes:
        """
        data_size: 03
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 321
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we drop object with name "1mds_key_version"
       When we add object "empty_object" with following attributes:
        """
        data_size: 0
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
       When we drop object with name "empty_object"

       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "3" object(s) of size "33"
        And bucket "delete_queue" in delete queue has "0" object part(s) of size "0"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: 1mds_key_version
          part_id:
          parts_count:
          data_size: 03
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 321
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: empty_object
          part_id:
          parts_count:
          data_size: 0
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 1234
          mds_key_version: 2
          mds_key_uuid: 33333333-3333-3333-3333-333333333333

        - name: mds_key_version2
          part_id:
          parts_count:
          data_size: 30
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
       When we list up to "10" deleted for at least "1 hour" object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        """
       When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
       Then we get following objects:
        """
        """
        And we have no errors in counters

  Scenario: Dropped objects successfully fall in storage delete queue
       When we add object "mds_key_version2" with following attributes:
        """
        data_size: 30
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
       When we drop object with name "mds_key_version2"
       When we add object "1mds_key_version" with following attributes:
        """
        data_size: 03
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 321
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we drop object with name "1mds_key_version"
       When we add object "empty_object" with following attributes:
        """
        data_size: 0
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
       When we drop object with name "empty_object"

       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "3" object(s) of size "33"
        And bucket "delete_queue" in delete queue has "0" object part(s) of size "0"

       When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
       Then we get following objects:
        """
        """
       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: 1mds_key_version
          data_size: 03
          data_md5: 22222222-2222-2222-2222-222222222222
          part_id:
          parts_count:
          mds_namespace: ns
          mds_couple_id: 321
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: empty_object
          data_size: 0
          data_md5: 22222222-2222-2222-2222-222222222222
          part_id:
          parts_count:
          mds_namespace: ns
          mds_couple_id: 1234
          mds_key_version: 2
          mds_key_uuid: 33333333-3333-3333-3333-333333333333

        - name: mds_key_version2
          data_size: 30
          data_md5: 11111111-1111-1111-1111-111111111111
          part_id:
          parts_count:
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
        And we have no errors in counters

  Scenario: Objects with NULL MDS do not fall into storage delete queue
       When we add object "mds_key_version2" with following attributes:
        """
        data_size: 0
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace:
        mds_couple_id: NULL
        mds_key_version: NULL
        mds_key_uuid: NULL
        """
       When we drop object with name "mds_key_version2"
       When we add object "1mds_key_version" with following attributes:
        """
        data_size: 0
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace:
        mds_couple_id: NULL
        mds_key_version: NULL
        mds_key_uuid: NULL
        """
       When we drop object with name "1mds_key_version"

       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object part(s) of size "0"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        """
       When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
       Then we get following objects:
        """
        """
        And we have no errors in counters


  Scenario: Only dropped object parts fall into storage delete queue
       When we start multipart upload of object "some_object_part"
        And we upload part for object "some_object_part":
        """
        part_id: 1
        data_size: 30
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
        And we upload part for object "some_object_part":
        """
        part_id: 2
        data_size: 03
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 321
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """

       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "2" object part(s) of size "33"
        And bucket "delete_queue" in delete queue has "0" object(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object part(s) of size "0"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        """
       When we list current parts for object "some_object_part"
       Then we get following object parts:
        """
        - name: some_object_part
          part_id: 1
          data_size: 30
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - name: some_object_part
          part_id: 2
          data_size: 03
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 321
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
        And we have no errors in counters


  Scenario: We don't delete object parts, which was dropped less than specified amount of days
       When we start multipart upload of object "some_object_part":
        """
        storage_class: 1
        """
        And we upload part for object "some_object_part":
        """
        part_id: 1
        data_size: 30
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
        And we upload part for object "some_object_part":
        """
        part_id: 2
        data_size: 03
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 321
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """

       When we abort multipart upload for object "some_object_part"
       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object(s) of size "0"
        And bucket "delete_queue" in delete queue has "2" object part(s) of size "33"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: some_object_part
          part_id: 1
          parts_count:
          data_size: 30
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
          storage_class: 1
        - name: some_object_part
          part_id: 2
          parts_count:
          data_size: 03
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 321
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
          storage_class: 1
        """
       When we list up to "10" deleted for at least "1 hour" object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        """
       When we list current parts for object "some_object_part"
       Then we get following object parts:
        """
        """
        And we have no errors in counters


  Scenario: Dropped object parts successfully fall in storage delete queue
       When we start multipart upload of object "some_object_part"
        And we upload part for object "some_object_part":
        """
        part_id: 1
        data_size: 30
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
        And we upload part for object "some_object_part":
        """
        part_id: 2
        data_size: 03
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 321
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """

       When we abort multipart upload for object "some_object_part"
       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object(s) of size "0"
        And bucket "delete_queue" in delete queue has "2" object part(s) of size "33"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: some_object_part
          part_id: 1
          parts_count:
          data_size: 30
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - name: some_object_part
          part_id: 2
          parts_count:
          data_size: 03
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 321
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we list current parts for object "some_object_part"
       Then we get following object parts:
        """
        """
        And we have no errors in counters


  Scenario: Dropped multipart "root" record falls into storage delete queue
       When we start multipart upload of object "multipart_object":
        """
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
       When we abort multipart upload for object "multipart_object"
       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object(s) of size "0"
        And bucket "delete_queue" in delete queue has "1" object part(s) of size "0"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: multipart_object
          part_id: 0
          parts_count:
          data_size: 0
          data_md5:
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
       When we list current parts for object "some_object_part"
       Then we get following object parts:
        """
        """
        And we have no errors in counters


  Scenario: Object parts with data_size = 0 fall into storage delete queue
       When we start multipart upload of object "some_object_part"
        And we upload part for object "some_object_part":
        """
        part_id: 1
        data_size: 0
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
        And we upload part for object "some_object_part":
        """
        part_id: 2
        data_size: 0
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 321
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """

       When we abort multipart upload for object "some_object_part"

       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object(s) of size "0"
        And bucket "delete_queue" in delete queue has "2" object part(s) of size "0"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: some_object_part
          part_id: 1
          parts_count:
          data_size: 0
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - name: some_object_part
          part_id: 2
          parts_count:
          data_size: 0
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 321
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we list current parts for object "some_object_part"
       Then we get following objects:
        """
        """
        And we have no errors in counters


  Scenario: Only dropped multipart objects fall into storage delete queue
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
        And we upload part for object "multipart_object":
        """
        part_id: 2
        data_size: 66660000
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
        And we complete the following multipart upload:
        """
        name: multipart_object
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e

            - part_id: 2
              data_md5: 11111111-1111-1111-1111-111111111111
        """

       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "1" multipart object(s) of size "111100000"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object part(s) of size "0"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        """
       When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
       Then we get following objects:
        """
        - name: multipart_object
          data_size: 111100000
          data_md5: 66666666-6666-6666-6666-666666666666
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:
        """
        And we have no errors in counters


  Scenario: We don't delete dropped multipart objects, which was dropped less than specified amount of days
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
        And we upload part for object "multipart_object":
        """
        part_id: 2
        data_size: 66660000
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
        And we complete the following multipart upload:
        """
        name: multipart_object
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e

            - part_id: 2
              data_md5: 11111111-1111-1111-1111-111111111111
        """

       When we drop object with name "multipart_object"
       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object(s) of size "0"
        And bucket "delete_queue" in delete queue has "2" object part(s) of size "111100000"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: multipart_object
          part_id: 1
          parts_count: 2
          data_size: 44440000
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          mds_couple_id: 222
          mds_key_version: 2
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: multipart_object
          part_id: 2
          parts_count: 2
          data_size: 66660000
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace:
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
       When we list up to "10" deleted for at least "1 hour" object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        """
       When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
       Then we get following objects:
        """
        """
        And we have no errors in counters


  Scenario: Dropped multipart object with "metadata" info falls into storage delete queue
       When we start multipart upload of object "multipart_object":
        """
        mds_namespace: ns-3
        mds_couple_id: 3333
        mds_key_version: 2
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        storage_class: 1
        """
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
        And we upload part for object "multipart_object":
        """
        part_id: 2
        data_size: 66660000
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
        And we complete the following multipart upload:
        """
        name: multipart_object
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e

            - part_id: 2
              data_md5: 11111111-1111-1111-1111-111111111111
        """

       When we drop object with name "multipart_object"
       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "0" object(s) of size "0"
        And bucket "delete_queue" in delete queue has "3" object part(s) of size "111100000"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: multipart_object
          part_id: 0
          parts_count: 2
          data_size: 0
          data_md5: 66666666-6666-6666-6666-666666666666
          mds_namespace: ns-3
          mds_couple_id: 3333
          mds_key_version: 2
          mds_key_uuid: 33333333-3333-3333-3333-333333333333

        - name: multipart_object
          part_id: 1
          parts_count: 2
          data_size: 44440000
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          mds_couple_id: 222
          mds_key_version: 2
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: multipart_object
          part_id: 2
          parts_count: 2
          data_size: 66660000
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace:
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
       When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
       Then we get following objects:
        """
        """
        And we have no errors in counters


  Scenario: Dropped multipart object and simple object fall into storage delete queue
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
        And we upload part for object "multipart_object":
        """
        part_id: 2
        data_size: 66660000
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
        And we complete the following multipart upload:
        """
        name: multipart_object
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e

            - part_id: 2
              data_md5: 11111111-1111-1111-1111-111111111111
        """
       When we drop object with name "multipart_object"
       When we add object "mds_key_version2" with following attributes:
        """
        data_size: 30
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
       When we drop object with name "mds_key_version2"
       When we add object "1mds_key_version" with following attributes:
        """
        data_size: 03
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 321
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we drop object with name "1mds_key_version"

       Then bucket "delete_queue" has "0" object(s) of size "0"
        And bucket "delete_queue" has "0" multipart object(s) of size "0"
        And bucket "delete_queue" has "0" object part(s) of size "0"
        And bucket "delete_queue" in delete queue has "2" object(s) of size "33"
        And bucket "delete_queue" in delete queue has "2" object part(s) of size "111100000"

       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: 1mds_key_version
          data_size: 03
          data_md5: 22222222-2222-2222-2222-222222222222
          part_id:
          parts_count:
          mds_namespace: ns
          mds_couple_id: 321
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: mds_key_version2
          data_size: 30
          data_md5: 11111111-1111-1111-1111-111111111111
          part_id:
          parts_count:
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - name: multipart_object
          part_id: 1
          parts_count: 2
          data_size: 44440000
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          mds_couple_id: 222
          mds_key_version: 2
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: multipart_object
          part_id: 2
          parts_count: 2
          data_size: 66660000
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace:
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
       When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
       Then we get following objects:
        """
        """
        And we have no errors in counters


  Scenario: Test list_deleted_objects on objects
       When we add object "1mds_key_version" with following attributes:
        """
        data_size: 03
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 321
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we drop object with name "1mds_key_version"
       When we add object "mds_key_version2" with following attributes:
        """
        data_size: 30
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
       When we drop object with name "mds_key_version2"

      # Scenario: we successfully list deleted objects
       When we list up to "10" deleted object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: mds_key_version2
          part_id:
          data_size: 30
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
      # Scenario: we list not more than "limit" deleted objects
       When we list up to "0" deleted object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        """
        And we have no errors in counters


  Scenario: Test list_deleted_objects on object parts
       When we start multipart upload of object "some_object_part"
        And we upload part for object "some_object_part":
        """
        part_id: 1
        data_size: 30
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
        And we upload part for object "some_object_part":
        """
        part_id: 2
        data_size: 03
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 321
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """

       When we abort multipart upload for object "some_object_part"
      # Scenario: we successfully list deleted objects
       When we list up to "10" deleted object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: some_object_part
          part_id: 1
          data_size: 30
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
      # Scenario: we list not more than "limit" deleted objects
       When we list up to "0" deleted object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        """
        And we have no errors in counters


  Scenario: Test remove_from_storage_delete_queue on a former object with mds_key_version = 2
       When we add object "object_exist_key_2" with following attributes:
        """
        data_size: 10
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we drop object with name "object_exist_key_2"

      # Scenario: we successfully remove the deleted object
       When we delete "object_exist_key_2" in the storage delete queue with following attributes:
        """
        part_id: NULL
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        """


  Scenario: Test remove_from_storage_delete_queue on former object parts
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
        And we upload part for object "multipart_object":
        """
        part_id: 2
        data_size: 66660000
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we abort multipart upload for object "multipart_object"

      # Scenario: we successfully delete former object part if mds_key_version = 2
       When we delete "multipart_object" in the storage delete queue with following attributes:
        """
        part_id: 1
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: multipart_object
          data_size: 66660000
          data_md5: 22222222-2222-2222-2222-222222222222
          part_id: 2
          parts_count:
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """


  Scenario: Test delay_deletion on a former object
      # Scenario: we successfully fill storage delete queue
       When we add object "object_exist_key_2" with following attributes:
        """
        data_size: 10
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we drop object with name "object_exist_key_2"

      # Scenario: we successfully prevent premature deletion
       When delay deletion of "object_exist_key_2" deleted object with following attributes by "0.5 second":
        """
        part_id: NULL
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we list up to "10" deleted object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        """

      # Scenario: after some time we can successfully delete
       When we wait "0.5" seconds
       When we list up to "10" deleted object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: object_exist_key_2
          data_size: 10
          part_id: NULL
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 2
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we delete "object_exist_key_2" in the storage delete queue with following attributes:
        """
        part_id: NULL
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        """
        And we have no errors in counters


  Scenario: Test delay_deletion on a former object part
      # Scenario: we successfully fill storage delete queue
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
       When we abort multipart upload for object "multipart_object"

      # Scenario: we successfully prevent premature deletion
       When delay deletion of "multipart_object" deleted object with following attributes by "0.5 second":
        """
        part_id: 1
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we list up to "10" deleted object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        """

      # Scenario: after some time we can successfully delete
       When we wait "0.5" seconds
       When we list up to "10" deleted object(s) in delete queue
       Then we get the following deleted object(s) as a result:
        """
        - name: multipart_object
          data_size: 44440000
          part_id: 1
          mds_namespace: ns-2
          mds_couple_id: 222
          mds_key_version: 2
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we delete "multipart_object" in the storage delete queue with following attributes:
        """
        part_id: 1
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we list all in the storage delete queue
       Then we get the following deleted object(s) as a result:
        """
        """
        And we have no errors in counters


    Scenario: Check errors of delay_deletion
       When delay deletion of "object_exist_key_2" deleted object with following attributes by "0.5 second":
        """
        part_id: NULL
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       Then we get an error with code "S3D01"

       When we add object "object_exist_key_2" with following attributes:
        """
        data_size: 10
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       When we drop object with name "object_exist_key_2"

       When delay deletion of "object_exist_key_2" deleted object with following attributes by "-1 second":
        """
        part_id: NULL
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       Then we get an error with code "22004"

       When delay deletion of "object_exist_key_2" deleted object with following attributes by "1 year":
        """
        part_id: NULL
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
       Then we get an error with code "22004"
        And we have no errors in counters

  Scenario: copying from s3.storage_delete_queue to s3.billing_delete_queue works
    When we add object "test1" with following attributes:
      """
      data_size: 30
      data_md5: 11111111-1111-1111-1111-111111111111
      mds_namespace: ns
      mds_couple_id: 123
      mds_key_version: 2
      mds_key_uuid: 22222222-2222-2222-2222-222222222222
      storage_class: 2
      """
    When we drop object with name "test1"
    When we add object "test2" with following attributes:
      """
      data_size: 03
      data_md5: 22222222-2222-2222-2222-222222222222
      mds_namespace: ns
      mds_couple_id: 321
      mds_key_version: 1
      mds_key_uuid: 11111111-1111-1111-1111-111111111111
      storage_class: 2
      """
    When we drop object with name "test2"
    When we add object "test3" with following attributes:
      """
      data_size: 0
      data_md5: 22222222-2222-2222-2222-222222222222
      mds_namespace: ns
      mds_couple_id: 1234
      mds_key_version: 2
      mds_key_uuid: 33333333-3333-3333-3333-333333333333
      storage_class: 1
      """
    When we drop object with name "test3"

    When we list all in the storage delete queue
    When we reset created time on storage_delete_queue "test1"
    Then we get the following deleted object(s) as a result:
      """
      - name: test1
        part_id:
        parts_count:
        data_size: 30
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        storage_class: 2

      - name: test2
        part_id:
        parts_count:
        data_size: 03
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 321
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 2

      - name: test3
        part_id:
        parts_count:
        data_size: 0
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        storage_class: 1
      """

    When we run copy delete queue script on "0" db
    And we list all in the billing delete queue
    Then we get the following deleted object(s) as a result:
    """
     - name: test2
       part_id:
       data_size: 03
       storage_class: 2
       status: 0
     """

    When we add object "test4" with following attributes:
      """
      data_size: 50
      data_md5: 22242222-2222-2222-2222-222222222222
      mds_namespace: ns
      mds_couple_id: 12345
      mds_key_version: 2
      mds_key_uuid: 33373333-3333-3333-3333-333333333333
      storage_class: 2
      """
    And we drop object with name "test4"
    And we run copy delete queue script on "0" db
    And we list all in the billing delete queue
    Then we get the following deleted object(s) as a result:
    """
    - name: test2
      part_id:
      data_size: 03
      storage_class: 2
      status: 0

    - name: test4
      part_id:
      data_size: 50
      storage_class: 2
      status: 0
    """
