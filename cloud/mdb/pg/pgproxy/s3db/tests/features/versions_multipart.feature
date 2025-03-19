Feature: Operations with multipart objects in versioning bucket

  Background: Set buckets owner account
    Given empty DB
     And buckets owner account "1"
     And a bucket with name "TestMultipart"

  Scenario: Completing multipart upload with enabled versioning
    When we enable bucket versioning
     And we start multipart upload of object "Bilbo"
     And we upload part for object "Bilbo"
        """
        part_id: 1
        data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        data_size: 100000000
        """
     And we complete the following multipart upload:
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
        data_size: 100000000
        data_md5: 0ae6d8fe-ed81-0320-ef10-04b5e6c62455
        parts_count: 1
        null_version: false
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "1" multipart object(s) of size "100000000"
     And bucket "TestMultipart" has "0" object part(s) of size "0"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And we have no errors in counters

  Scenario: Completing multipart upload with suspended versioning
    When we suspend bucket versioning
     And we start multipart upload of object "Bilbo"
     And we upload part for object "Bilbo"
        """
        part_id: 1
        data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        data_size: 100000000
        """
     And we complete the following multipart upload:
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
        data_size: 100000000
        data_md5: 0ae6d8fe-ed81-0320-ef10-04b5e6c62455
        parts_count: 1
        null_version: true
        """
    Then bucket "TestMultipart" has "0" object(s) of size "0"
     And bucket "TestMultipart" has "1" multipart object(s) of size "100000000"
     And bucket "TestMultipart" has "0" object part(s) of size "0"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And we have no errors in counters

  Scenario: Completing multipart upload keep object version with enabled versioning
    When we add object "Bilbo" with following attributes
        """
        data_size: 100
        """
     And we enable bucket versioning
     And we start multipart upload of object "Bilbo"
     And we upload part for object "Bilbo"
        """
        part_id: 1
        data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        data_size: 100000000
        """
     And we complete the following multipart upload:
        """
        name: Bilbo
        data_md5: 0ae6d8fe-ed81-0320-ef10-04b5e6c62455
        parts_data:
            - part_id: 1
              data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        """
    Then bucket "TestMultipart" has "1" object(s) of size "100"
     And bucket "TestMultipart" has "1" multipart object(s) of size "100000000"
     And bucket "TestMultipart" has "0" object part(s) of size "0"
     And bucket "TestMultipart" in delete queue has "0" object(s) of size "0"
     And we have no errors in counters

  Scenario: Completing multipart upload replace null version with suspended versioning
    When we add object "Bilbo" with following attributes
        """
        data_size: 100
        """
     And we enable bucket versioning
     And we add object "Bilbo" with following attributes
        """
        data_size: 1000
        """
     And we suspend bucket versioning
     And we start multipart upload of object "Bilbo"
     And we upload part for object "Bilbo"
        """
        part_id: 1
        data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        data_size: 100000000
        """
     And we complete the following multipart upload:
        """
        name: Bilbo
        data_md5: 0ae6d8fe-ed81-0320-ef10-04b5e6c62455
        parts_data:
            - part_id: 1
              data_md5: a193043f-0c6d-42ce-843c-996ec0e5a029
        """
    Then bucket "TestMultipart" has "1" object(s) of size "1000"
     And bucket "TestMultipart" has "1" multipart object(s) of size "100000000"
     And bucket "TestMultipart" has "0" object part(s) of size "0"
     And bucket "TestMultipart" in delete queue has "1" object(s) of size "100"
     And we have no errors in counters
