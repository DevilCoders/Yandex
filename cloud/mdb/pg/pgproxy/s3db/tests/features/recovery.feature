Feature: Recover objects from delete queue

  Background: Set buckets owner account

  Scenario: Recover simple object
    Given empty DB
     And buckets owner account "1"
     And a bucket with name "Recovery"
    When we add object "test_simple_object" with following attributes:
        """
        data_size: 333
        data_md5: 22222222-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
     And we get info about object with name "test_simple_object"
    Then our object has attributes:
        """
        name: test_simple_object
        data_size: 333
        data_md5: 22222222-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        null_version: True
        delete_marker: False
        parts_count:
        parts:
        """
    Then bucket "Recovery" has "1" object(s) of size "333"
     And bucket "Recovery" has "0" multipart object(s) of size "0"
     And bucket "Recovery" has "0" object part(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object part(s) of size "0"

    When we drop object with name "test_simple_object"
    Then bucket "Recovery" has "0" object(s) of size "0"
     And bucket "Recovery" has "0" multipart object(s) of size "0"
     And bucket "Recovery" has "0" object part(s) of size "0"
     And bucket "Recovery" in delete queue has "1" object(s) of size "333"
     And bucket "Recovery" in delete queue has "0" object part(s) of size "0"

    When we recover object with name "test_simple_object"
     And we get info about object with name "test_simple_object"
    Then our object has attributes:
        """
        name: test_simple_object
        data_size: 333
        data_md5: 22222222-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        null_version: True
        delete_marker: False
        parts_count:
        parts:
        """
    Then bucket "Recovery" has "1" object(s) of size "333"
     And bucket "Recovery" has "0" multipart object(s) of size "0"
     And bucket "Recovery" has "0" object part(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters


  Scenario: Recover multipart object
    Given empty DB
     And buckets owner account "1"
     And a bucket with name "Recovery"

    Given a multipart upload in bucket "Recovery" for object "test_multipart_object"
    When we abort multipart upload for object "test_multipart_object"
    When we start multipart upload of object "test_multipart_object":
        """
        metadata: '{"encryption": {}}'
        """
    When we upload part for object "test_multipart_object":
        """
        part_id: 1
        data_size: 2147483648
        data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9
        mds_namespace: ns-6
        mds_couple_id: 6666
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        encryption: qwerty
        """
    When we upload part for object "test_multipart_object":
        """
        part_id: 2
        data_size: 4294967296
        data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
        mds_namespace: ns-6
        mds_couple_id: 66666
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        encryption: zxcvbn
        """
    # data_md5 = md5(string_agg(uuid_send(data_md5), ''::bytea order by part_id))::uuid
    When we complete the following multipart upload:
        """
        name: test_multipart_object
        data_md5: 945d1dca-c73f-8a13-102f-655f8902daaf
        parts_data:
            - part_id: 1
              data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9

            - part_id: 2
              data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
        """
     And we get info about object with name "test_multipart_object"
    Then our object has attributes:
        """
        name: test_multipart_object
        data_size: 6442450944
        data_md5: 945d1dca-c73f-8a13-102f-655f8902daaf
        mds_namespace:
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        null_version: True
        delete_marker: False
        parts_count: 2
        metadata:
          encryption:
            parts_meta:
              - qwerty
              - zxcvbn
        """
    When we list object parts for object "test_multipart_object"
    Then we get following object parts:
        """
        - part_id: 1
          data_size: 2147483648
          data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9
          mds_namespace:
          mds_couple_id: 6666
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - part_id: 2
          data_size: 4294967296
          data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
          mds_namespace:
          mds_couple_id: 66666
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "Recovery" has "0" object(s) of size "0"
     And bucket "Recovery" has "1" multipart object(s) of size "6442450944"
     And bucket "Recovery" has "0" object part(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object part(s) of size "0"

    When we drop object with name "test_multipart_object"
    Then bucket "Recovery" has "0" object(s) of size "0"
     And bucket "Recovery" has "0" multipart object(s) of size "0"
     And bucket "Recovery" has "0" object part(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object(s) of size "0"
     And bucket "Recovery" in delete queue has "2" object part(s) of size "6442450944"

    When we recover object with name "test_multipart_object"
     And we get info about object with name "test_multipart_object"
    Then our object has attributes:
        """
        name: test_multipart_object
        data_size: 6442450944
        data_md5: 945d1dca-c73f-8a13-102f-655f8902daaf
        mds_namespace:
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        null_version: True
        delete_marker: False
        parts_count: 2
        metadata:
          encryption:
            parts_meta:
              - qwerty
              - zxcvbn
        """
    When we list object parts for object "test_multipart_object"
    Then we get following object parts:
        """
        - part_id: 1
          data_size: 2147483648
          data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9
          mds_namespace:
          mds_couple_id: 6666
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - part_id: 2
          data_size: 4294967296
          data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
          mds_namespace:
          mds_couple_id: 66666
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "Recovery" has "0" object(s) of size "0"
     And bucket "Recovery" has "1" multipart object(s) of size "6442450944"
     And bucket "Recovery" has "0" object part(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters


  Scenario: Recover multipart object with metadata
    Given empty DB
     And buckets owner account "1"
     And a bucket with name "Recovery"
    When we start multipart upload of object "test_multipart_object_with_metadata":
        """
        mds_namespace: ns-7
        mds_couple_id: 77777
        mds_key_version: 2
        mds_key_uuid: 77777777-7777-7777-7777-777777777777
        """
     And we upload part for object "test_multipart_object_with_metadata":
        """
        part_id: 1
        data_size: 1000000000
        data_md5: 88888888-8888-8888-8888-888888888888
        mds_namespace: ns-8
        mds_couple_id: 88888
        mds_key_version: 2
        mds_key_uuid: 88888888-8888-8888-8888-888888888888
        """
     And we complete the following multipart upload:
        """
        name: test_multipart_object_with_metadata
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: 88888888-8888-8888-8888-888888888888
        """
    Then our object has attributes:
        """
        name: test_multipart_object_with_metadata
        data_size: 1000000000
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_count: 1
        mds_namespace: ns-7
        mds_couple_id: 77777
        mds_key_version: 2
        mds_key_uuid: 77777777-7777-7777-7777-777777777777
        """
    Then bucket "Recovery" has "0" object(s) of size "0"
     And bucket "Recovery" has "1" multipart object(s) of size "1000000000"
     And bucket "Recovery" has "0" object part(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object part(s) of size "0"

    When we drop object with name "test_multipart_object_with_metadata"
    Then bucket "Recovery" has "0" object(s) of size "0"
     And bucket "Recovery" has "0" multipart object(s) of size "0"
     And bucket "Recovery" has "0" object part(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object(s) of size "0"
     And bucket "Recovery" in delete queue has "2" object part(s) of size "1000000000"

    When we recover object with name "test_multipart_object_with_metadata"
     And we get info about object with name "test_multipart_object_with_metadata"
    Then our object has attributes:
        """
        name: test_multipart_object_with_metadata
        data_size: 1000000000
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_count: 1
        mds_namespace: ns-7
        mds_couple_id: 77777
        mds_key_version: 2
        mds_key_uuid: 77777777-7777-7777-7777-777777777777
        """
    When we list object parts for object "test_multipart_object_with_metadata"
    Then we get following object parts:
        """
        - part_id: 1
          data_size: 1000000000
          data_md5: 88888888-8888-8888-8888-888888888888
          mds_namespace:
          mds_couple_id: 88888
          mds_key_version: 2
          mds_key_uuid: 88888888-8888-8888-8888-888888888888
        """
    Then bucket "Recovery" has "0" object(s) of size "0"
     And bucket "Recovery" has "1" multipart object(s) of size "1000000000"
     And bucket "Recovery" has "0" object part(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters


  Scenario: Recover simple object if have some objects with same name
    Given empty DB
     And buckets owner account "1"
     And a bucket with name "Recovery"
    When we add object "test_simple_object" with following attributes:
        """
        data_size: 111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
     And we drop object with name "test_simple_object"
     And we add object "test_simple_object" with following attributes:
        """
        data_size: 222
        data_md5: 22222222-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-1111-1111-1111-111111111111
        """
     And we drop object with name "test_simple_object"
     And we recover object with name "test_simple_object"
     And we get info about object with name "test_simple_object"
    Then our object has attributes:
        """
        name: test_simple_object
        data_size: 222
        data_md5: 22222222-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-1111-1111-1111-111111111111
        null_version: True
        delete_marker: False
        parts_count:
        parts:
        """
    Then bucket "Recovery" has "1" object(s) of size "222"
     And bucket "Recovery" has "0" multipart object(s) of size "0"
     And bucket "Recovery" has "0" object part(s) of size "0"
     And bucket "Recovery" in delete queue has "1" object(s) of size "111"
     And bucket "Recovery" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters


  Scenario: Recover multipart object if have some objects with same name
    Given empty DB
     And buckets owner account "1"
     And a bucket with name "Recovery"

    Given a multipart upload in bucket "Recovery" for object "test_multipart_object"
    When we upload part for object "test_multipart_object":
        """
        part_id: 1
        data_size: 1147483641
        data_md5: 300c0872-0610-4865-b1ef-7653ed80af01
        mds_namespace: ns-6
        mds_couple_id: 6666
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we upload part for object "test_multipart_object":
        """
        part_id: 2
        data_size: 1294967291
        data_md5: 2dd1ee96-f646-44e1-ab46-bbcc69db449f
        mds_namespace: ns-6
        mds_couple_id: 66666
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    # data_md5 = md5(string_agg(uuid_send(data_md5), ''::bytea order by part_id))::uuid
    When we complete the following multipart upload:
        """
        name: test_multipart_object
        data_md5: 7fd30cc0-26c7-f64e-b6cf-104b5192ff50
        parts_data:
            - part_id: 1
              data_md5: 300c0872-0610-4865-b1ef-7653ed80af01

            - part_id: 2
              data_md5: 2dd1ee96-f646-44e1-ab46-bbcc69db449f
        """
     And we get info about object with name "test_multipart_object"
    Then our object has attributes:
        """
        name: test_multipart_object
        data_size: 2442450932
        data_md5: 7fd30cc0-26c7-f64e-b6cf-104b5192ff50
        mds_namespace:
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        null_version: True
        delete_marker: False
        parts_count: 2
        """
    When we drop object with name "test_multipart_object"

    Given a multipart upload in bucket "Recovery" for object "test_multipart_object"
    When we upload part for object "test_multipart_object":
        """
        part_id: 1
        data_size: 2147483648
        data_md5: 2100517a-27e7-4327-8ee9-7c7aca358de4
        mds_namespace: ns-6
        mds_couple_id: 6666
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-3333-111111111111
        """
    When we upload part for object "test_multipart_object":
        """
        part_id: 2
        data_size: 2294967296
        data_md5: 053e8f61-3bda-4560-9f95-3bc9c4fb6549
        mds_namespace: ns-6
        mds_couple_id: 66666
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-4444-222222222222
        """
    # data_md5 = md5(string_agg(uuid_send(data_md5), ''::bytea order by part_id))::uuid
    When we complete the following multipart upload:
        """
        name: test_multipart_object
        data_md5: 76a0b2ab-6df6-76f9-8101-50e28a0fbd40
        parts_data:
            - part_id: 1
              data_md5: 2100517a-27e7-4327-8ee9-7c7aca358de4

            - part_id: 2
              data_md5: 053e8f61-3bda-4560-9f95-3bc9c4fb6549
        """
     And we get info about object with name "test_multipart_object"
    Then our object has attributes:
        """
        name: test_multipart_object
        data_size: 4442450944
        data_md5: 76a0b2ab-6df6-76f9-8101-50e28a0fbd40
        mds_namespace:
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        null_version: True
        delete_marker: False
        parts_count: 2
        """
    When we drop object with name "test_multipart_object"
     And we recover object with name "test_multipart_object"
     And we get info about object with name "test_multipart_object"
    Then our object has attributes:
        """
        name: test_multipart_object
        data_size: 4442450944
        data_md5: 9d00df6a-7d32-f127-e0b8-39f30fabe7c7
        mds_namespace:
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        null_version: True
        delete_marker: False
        parts_count: 2
        """
    When we list object parts for object "test_multipart_object"
    Then we get following object parts:
        """
        - part_id: 1
          data_size: 2147483648
          data_md5: 2100517a-27e7-4327-8ee9-7c7aca358de4
          mds_namespace:
          mds_couple_id: 6666
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-3333-111111111111

        - part_id: 2
          data_size: 2294967296
          data_md5: 053e8f61-3bda-4560-9f95-3bc9c4fb6549
          mds_namespace:
          mds_couple_id: 66666
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-4444-222222222222
        """
    Then bucket "Recovery" has "0" object(s) of size "0"
     And bucket "Recovery" has "1" multipart object(s) of size "4442450944"
     And bucket "Recovery" has "0" object part(s) of size "0"
     And bucket "Recovery" in delete queue has "0" object(s) of size "0"
     And bucket "Recovery" in delete queue has "2" object part(s) of size "2442450932"
     And we have no errors in counters

  Scenario: Recover broken multipart object fails
    Given empty DB
     And buckets owner account "1"
     And a bucket with name "Recovery"

    Given a multipart upload in bucket "Recovery" for object "test_multipart_object"
    When we upload part for object "test_multipart_object":
        """
        part_id: 1
        data_size: 11
        data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9
        mds_namespace: ns-6
        mds_couple_id: 6666
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we upload part for object "test_multipart_object":
        """
        part_id: 3
        data_size: 33
        data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
        mds_namespace: ns-6
        mds_couple_id: 66666
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    When we abort multipart upload for object "test_multipart_object"
    When we recover object with name "test_multipart_object"
    Then we get an error with code "S3M03"
