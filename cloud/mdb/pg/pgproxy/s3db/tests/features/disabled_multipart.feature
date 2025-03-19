Feature: Operations with multipart objects in disabled versioning bucket

  Background: Set buckets owner account
    Given buckets owner account "1"

  Scenario: Starting multipart upload
    Given a bucket with name "TestMultipart"
    When we start multipart upload of object "Arven"
    Then we get the following object part:
        """
        name: Arven
        part_id: 0
        data_size: 0
        data_md5:
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "0" multipart object(s) of size "0"
     And bucket "TestMultipart" has "0" object part(s) of size "0"
    When we start multipart upload of object "MultipartMetainfo":
        """
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then we get the following object part:
        """
        name: MultipartMetainfo
        part_id: 0
        data_size: 0
        data_md5:
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "0" multipart object(s) of size "0"
     And bucket "TestMultipart" has "0" object part(s) of size "0"
     And we have no errors in counters

  Scenario: Uploading part into multipart object
    Given a multipart upload in bucket "TestMultipart" for object "Bilbo"
    When we upload part for object "Bilbo":
        """
        part_id: 1
        data_size: 10485760
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    Then we get the following object part:
        """
        name: Bilbo
        part_id: 1
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "0" multipart object(s) of size "0"
     And bucket "TestMultipart" has "1" object part(s) of size "10485760"

  Scenario: Uploading part into non-existent multipart object
    Given a bucket with name "TestMultipart"
    When we upload part for object "Gollum":
        """
        part_id: 1
        data_size: 10485760
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    Then we get an error with code "S3M01"
     And bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "0" multipart object(s) of size "0"
     And bucket "TestMultipart" has "1" object part(s) of size "10485760"


  Scenario: Uploading huge part into multipart object
    Given a multipart upload in bucket "TestMultipart" for object "Bilbo"
    When we upload part for object "Bilbo":
        """
        part_id: 2
        data_size: 5368709121
        data_md5: e55ce0c7-48e7-41ed-ba03-88c28cf89895
        mds_namespace: ns-9
        mds_couple_id: 987
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then we get an error with code "S3M05"
     And bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "0" multipart object(s) of size "0"
     And bucket "TestMultipart" has "1" object part(s) of size "10485760"


  Scenario: Listing object parts for incomplete multipart upload
    Given a multipart upload in bucket "TestMultipart" for object "Bilbo"
    When we list current parts for object "Bilbo"
    Then we get the following object part:
        """
        name: Bilbo
        part_id: 1
        data_size: 10485760
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """

  Scenario: Listing object parts for non-existent incomplete multipart upload
    Given a multipart upload in bucket "TestMultipart" for object "Galadriel"
    When we list current parts for object "Galadriel"
    Then we get empty result


  Scenario: Aborting non-existent multipart upload
    Given a bucket with name "TestMultipart"
    When we abort multipart upload for object "Gollum"
    Then we get an error with code "S3M01"


  Scenario: Aborting multipart upload
    Given a multipart upload in bucket "TestMultipart" for object "Gendalf"
    When we upload part for object "Gendalf":
        """
        part_id: 1
        data_size: 134217728
        data_md5: 20f4dc25-eb54-4118-8199-91094eee2cb3
        mds_namespace: ns-9
        mds_couple_id: 987
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
     And we abort multipart upload for object "Gendalf"
    Then we get the following object part:
        """
        name: Gendalf
        part_id: 0
        data_size: 134217728
        data_md5:
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "0" multipart object(s) of size "0"
     And bucket "TestMultipart" has "1" object part(s) of size "10485760"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "1" object part(s) of size "134217728"
     And we have no errors in counters

  Scenario: Listing object parts for aborted multipart upload
    Given a multipart upload in bucket "TestMultipart" for object "Isildur"
    When we upload part for object "Isildur":
        """
        part_id: 1
        data_size: 268435456
        data_md5: c617a597-67f4-4b54-913d-9ec30c444e8c
        mds_namespace: ns-7
        mds_couple_id: 777
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
     And we abort multipart upload for object "Isildur"
     And we list current parts for object "Isildur"
    Then we get empty result
     And bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "0" multipart object(s) of size "0"
     And bucket "TestMultipart" has "1" object part(s) of size "10485760"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "2" object part(s) of size "402653184"


  Scenario: Rewriting already uploaded object part
    Given a multipart upload in bucket "TestMultipart" for object "Bilbo"
    When we upload part for object "Bilbo":
        """
        part_id: 1
        data_size: 536870912
        data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        mds_namespace: ns-6
        mds_couple_id: 654321
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
     And we list current parts for object "Bilbo"
    Then we get the following object part:
        """
        name: Bilbo
        part_id: 1
        data_size: 536870912
        data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "0" multipart object(s) of size "0"
     And bucket "TestMultipart" has "1" object part(s) of size "536870912"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "3" object part(s) of size "413138944"
    # Now upload the same object part again (emulate retry)
    When we upload part for object "Bilbo":
        """
        part_id: 1
        data_size: 536870912
        data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        mds_namespace: ns-6
        mds_couple_id: 654321
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
     And we list current parts for object "Bilbo"
    Then we get the following object part:
        """
        name: Bilbo
        part_id: 1
        data_size: 536870912
        data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "0" multipart object(s) of size "0"
     And bucket "TestMultipart" has "1" object part(s) of size "536870912"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "3" object part(s) of size "413138944"
     And we have no errors in counters

  Scenario: Completing multipart upload
    Given a multipart upload in bucket "TestMultipart" for object "Bilbo"
    When we complete the following multipart upload:
        """
        name: Bilbo
        data_md5: 0ae6d8fe-ed81-0320-ef10-04b5e6c62455
        parts_data:
            - part_id: 1
              data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        """
    Then our object has attributes:
        """
        name: Bilbo
        data_size: 536870912
        data_md5: 0ae6d8fe-ed81-0320-ef10-04b5e6c62455
        parts_count: 1
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "1" multipart object(s) of size "536870912"
     And bucket "TestMultipart" has "0" object part(s) of size "0"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "3" object part(s) of size "413138944"
     And we have no errors in counters

  Scenario: Completing non-existent multipart upload
    Given a bucket with name "TestMultipart"
    When we complete the following multipart upload:
        """
        name: Bilbo
        data_md5: 0ae6d8fe-ed81-0320-ef10-04b5e6c62455
        parts_data:
            - part_id: 1
              data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        """
    Then we get an error with code "S3M01"
     And bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "1" multipart object(s) of size "536870912"
     And bucket "TestMultipart" has "0" object part(s) of size "0"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "3" object part(s) of size "413138944"


  Scenario: Completing invalid multipart upload
    Given a multipart upload in bucket "TestMultipart" for object "Sauron"
    When we upload part for object "Sauron":
        """
        part_id: 1
        data_size: 1073741824
        data_md5: 686194c1-afa8-4407-bb29-779189f65212
        mds_namespace: ns-6
        mds_couple_id: 666
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
     And we complete the following multipart upload:
        """
        name: Sauron
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: 686194c1-afa8-4407-bb29-779189f65212

            - part_id: 2
              data_md5: 66666666-6666-6666-6666-666666666666
        """
    Then we get an error with code "S3M02"
     And bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "1" multipart object(s) of size "536870912"
     And bucket "TestMultipart" has "1" object part(s) of size "1073741824"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "3" object part(s) of size "413138944"


  Scenario: Completing invalid multipart upload
    Given a multipart upload in bucket "TestMultipart" for object "Sauron"
    When we upload part for object "Sauron":
        """
        part_id: 1
        data_size: 1073741824
        data_md5: 686194c1-afa8-4407-bb29-779189f65212
        mds_namespace: ns-6
        mds_couple_id: 666
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
     And we complete the following multipart upload:
        """
        name: Sauron
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 2
              data_md5: 66666666-6666-6666-6666-666666666666

            - part_id: 1
              data_md5: 686194c1-afa8-4407-bb29-779189f65212
        """
    Then we get an error with code "S3M03"
     And bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "1" multipart object(s) of size "536870912"
     And bucket "TestMultipart" has "1" object part(s) of size "1073741824"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"


  Scenario: Completing multipart upload with several parts
    Given a multipart upload in bucket "TestMultipart" for object "Saruman"
    When we upload part for object "Saruman":
        """
        part_id: 1
        data_size: 2147483648
        data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9
        mds_namespace: ns-6
        mds_couple_id: 6666
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "1" multipart object(s) of size "536870912"
     And bucket "TestMultipart" has "2" object part(s) of size "3221225472"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"
    When we upload part for object "Saruman":
        """
        part_id: 2
        data_size: 4294967296
        data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
        mds_namespace: ns-6
        mds_couple_id: 66666
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "1" multipart object(s) of size "536870912"
     And bucket "TestMultipart" has "3" object part(s) of size "7516192768"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"
    When we complete the following multipart upload:
        """
        name: Saruman
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9

            - part_id: 2
              data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
        """
    Then our object has attributes:
        """
        name: Saruman
        data_size: 6442450944
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_count: 2
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "2" multipart object(s) of size "6979321856"
     And bucket "TestMultipart" has "1" object part(s) of size "1073741824"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"
     And we have no errors in counters

  Scenario: Completing multipart upload with metadata info
    Given a bucket with name "TestMultipart"
    When we start multipart upload of object "Metadata":
        """
        mds_namespace: ns-7
        mds_couple_id: 77777
        mds_key_version: 2
        mds_key_uuid: 77777777-7777-7777-7777-777777777777
        """
    Then we get the following object part:
        """
        name: Metadata
        part_id: 0
        data_size: 0
        data_md5:
        mds_namespace: ns-7
        mds_couple_id: 77777
        mds_key_version: 2
        mds_key_uuid: 77777777-7777-7777-7777-777777777777
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "2" multipart object(s) of size "6979321856"
     And bucket "TestMultipart" has "1" object part(s) of size "1073741824"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"
    When we upload part for object "Metadata":
        """
        part_id: 1
        data_size: 1000000000
        data_md5: 88888888-8888-8888-8888-888888888888
        mds_namespace: ns-8
        mds_couple_id: 88888
        mds_key_version: 2
        mds_key_uuid: 88888888-8888-8888-8888-888888888888
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "2" multipart object(s) of size "6979321856"
     And bucket "TestMultipart" has "2" object part(s) of size "2073741824"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"
    When we complete the following multipart upload:
        """
        name: Metadata
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: 88888888-8888-8888-8888-888888888888
        """
    Then our object has attributes:
        """
        name: Metadata
        data_size: 1000000000
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_count: 1
        mds_namespace: ns-7
        mds_couple_id: 77777
        mds_key_version: 2
        mds_key_uuid: 77777777-7777-7777-7777-777777777777
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "3" multipart object(s) of size "7979321856"
     And bucket "TestMultipart" has "1" object part(s) of size "1073741824"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"
     And we have no errors in counters

  Scenario: Completing multipart upload with small part
    Given a multipart upload in bucket "TestMultipart" for object "Fingon"
    When we upload part for object "Fingon":
        """
        part_id: 1
        data_size: 5000000
        data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9
        mds_namespace: ns-6
        mds_couple_id: 6666
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "3" multipart object(s) of size "7979321856"
     And bucket "TestMultipart" has "2" object part(s) of size "1078741824"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"
    When we upload part for object "Fingon":
        """
        part_id: 2
        data_size: 4294967296
        data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
        mds_namespace: ns-6
        mds_couple_id: 66666
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "3" multipart object(s) of size "7979321856"
     And bucket "TestMultipart" has "3" object part(s) of size "5373709120"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"
    When we complete the following multipart upload:
        """
        name: Fingon
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9

            - part_id: 2
              data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
        """
    Then we get an error with code "S3M04"
     And bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "3" multipart object(s) of size "7979321856"
     And bucket "TestMultipart" has "3" object part(s) of size "5373709120"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"


  Scenario: Listing object parts for completed multipart upload
    Given a multipart upload in bucket "TestMultipart" for object "Sam"
    When we upload part for object "Sam":
        """
        part_id: 1
        data_size: 20971520
        data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9
        mds_namespace: ns-6
        mds_couple_id: 6666
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "3" multipart object(s) of size "7979321856"
     And bucket "TestMultipart" has "4" object part(s) of size "5394680640"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"
    When we upload part for object "Sam":
        """
        part_id: 2
        data_size: 41943040
        data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
        mds_namespace: ns-6
        mds_couple_id: 66666
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "3" multipart object(s) of size "7979321856"
     And bucket "TestMultipart" has "5" object part(s) of size "5436623680"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"
    When we complete the following multipart upload:
        """
        name: Sam
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9

            - part_id: 2
              data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "4" multipart object(s) of size "8042236416"
     And bucket "TestMultipart" has "3" object part(s) of size "5373709120"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And bucket "TestMultipart" in delete queue has "4" object part(s) of size "1486880768"
    When we list object parts for object "Sam"
    Then we get following object parts:
        """
        - name: Sam
          part_id: 1
          data_size: 20971520
          data_md5: 3d63cf9e-dc8d-40b8-856f-b7c59d66b2f9
          mds_namespace:
          mds_couple_id: 6666
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: Sam
          part_id: 2
          data_size: 41943040
          data_md5: 1478d94a-c9d1-45d1-aa9b-5c4736c2ee58
          mds_namespace:
          mds_couple_id: 66666
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """

  Scenario: Listing object parts for non-existent completed multipart upload
    Given a multipart upload in bucket "TestMultipart" for object "Galadriel"
    When we list object parts for object "Galadriel"
    Then we get empty result


  Scenario: Listing multipart uploads with prefix
    Given a multipart upload in bucket "TestMultipartListing" for object "Europe/Russia/Moscow"
      And a multipart upload in bucket "TestMultipartListing" for object "Europe/Germany/Munich"
      And a multipart upload in bucket "TestMultipartListing" for object "Asia/Japan/Tokyo"
      And a multipart upload in bucket "TestMultipartListing" for object "Asia/Iran/Tegeran"
    When we list multipart uploads with following arguments:
        """
        prefix: Asia
        delimiter:
        start_after_key:
        start_after_created:
        """
    Then we get following objects:
        """
        - name: Asia/Iran/Tegeran
          data_md5:
          part_id: 0

        - name: Asia/Japan/Tokyo
          mds_couple_id:
        """

  Scenario: Listing multipart uploads with start_after_key
    Given a multipart upload in bucket "TestMultipartListing" for object "Europe/Russia/Moscow"
      And a multipart upload in bucket "TestMultipartListing" for object "Europe/Germany/Munich"
      And a multipart upload in bucket "TestMultipartListing" for object "Asia/Japan/Tokyo"
      And a multipart upload in bucket "TestMultipartListing" for object "Asia/Iran/Tegeran"
    When we list multipart uploads with following arguments:
        """
        prefix:
        delimiter:
        start_after_key: Europe
        start_after_created:
        """
    Then we get following objects:
        """
        - name: Europe/Germany/Munich
          data_md5:
          part_id: 0

        - name: Europe/Russia/Moscow
          mds_couple_id:
        """

  Scenario: Listing multipart uploads with prefix and delimiter
    Given a multipart upload in bucket "TestMultipartListing" for object "Europe/Russia/Moscow"
      And a multipart upload in bucket "TestMultipartListing" for object "Europe/Germany/Munich"
      And a multipart upload in bucket "TestMultipartListing" for object "Asia/Japan/Tokyo"
      And a multipart upload in bucket "TestMultipartListing" for object "Asia/Iran/Tegeran"
    When we list multipart uploads with following arguments:
        """
        prefix: Europe/
        delimiter: /
        start_after_key:
        start_after_created:
        """
    Then we get following objects:
        """
        - name: Europe/Germany/
          part_id:

        - name: Europe/Russia/
        """

  Scenario: Listing multipart uploads with prefix, delimiter and start_after_key
    Given a multipart upload in bucket "TestMultipartListing" for object "Europe/Russia/Moscow"
      And a multipart upload in bucket "TestMultipartListing" for object "Europe/Germany/Munich"
      And a multipart upload in bucket "TestMultipartListing" for object "Asia/Japan/Tokyo"
      And a multipart upload in bucket "TestMultipartListing" for object "Asia/Iran/Tegeran"
    When we list multipart uploads with following arguments:
        """
        prefix: Europe/
        delimiter: /
        start_after_key: Europe/Germany/
        start_after_created:
        """
    Then we get following objects:
        """
        - name: Europe/Russia/
        """
