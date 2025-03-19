Feature: Operations with basic objects in versioning bucket

  Background: Set buckets owner account
    Given empty DB
     And buckets owner account "1"
     And a bucket with name "foo"

  Scenario: Adding normal objects
    when we enable bucket versioning
    When we add object "bar" with following attributes:
        """
        data_size: 10
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    Then our object has attributes:
        """
        name: bar
        data_size: 10
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        null_version: False
        delete_marker: False
        parts_count:
        parts:
        """
    Then bucket "foo" has "1" object(s) of size "10"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "0" object(s) of size "0"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
    When we add object "bar" with following attributes:
        """
        data_size: 100
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then our object has attributes:
        """
        name: bar
        data_size: 100
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        null_version: False
        delete_marker: False
        parts_count:
        parts:
        """
    Then bucket "foo" has "2" object(s) of size "110"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "0" object(s) of size "0"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
    When we suspend bucket versioning
     And we add object "bar" with following attributes:
        """
        data_size: 1000
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 3
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
    Then our object has attributes:
        """
        name: bar
        data_size: 1000
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 3
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        null_version: True
        delete_marker: False
        parts_count:
        parts:
        """
    Then bucket "foo" has "3" object(s) of size "1110"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "0" object(s) of size "0"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
    When we add object "bar" with following attributes:
        """
        data_size: 10000
        """
    Then bucket "foo" has "3" object(s) of size "10110"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "1" object(s) of size "1000"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters

  Scenario: Adding huge object
    When we enable bucket versioning
     And we add object "baz" with following attributes:
        """
        data_size: 5368709121
        """
    Then we get an error with code "S3K03"
    When we suspend bucket versioning
     And we add object "baz" with following attributes:
        """
        data_size: 5368709121
        """
    Then we get an error with code "S3K03"

  Scenario: Getting object info shows actual version
    When we enable bucket versioning
    When we add object "bar" with following attributes:
        """
        data_size: 10
        data_md5: 11111111-1111-1111-1111-111111111111
        """
     And we add object "bar" with following attributes:
        """
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        """
    When we get info about object with name "bar"
    Then our object has attributes:
        """
        name: bar
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        null_version: False
        delete_marker: False
        """
    When we get info about previous version of object "bar"
    Then our object has attributes:
        """
        name: bar
        data_size: 10
        data_md5: 11111111-1111-1111-1111-111111111111
        null_version: False
        delete_marker: False
        """
    When we suspend bucket versioning
     And we add object "bar" with following attributes:
        """
        data_size: 30
        data_md5: 33333333-3333-3333-3333-333333333333
        """
    When we get info about object with name "bar"
    Then our object has attributes:
        """
        name: bar
        data_size: 30
        data_md5: 33333333-3333-3333-3333-333333333333
        null_version: True
        delete_marker: False
        """
    When we get info about previous version of object "bar"
    Then our object has attributes:
        """
        name: bar
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        null_version: False
        delete_marker: False
        """
     And we have no errors in counters

  Scenario: Add delete mark
    When we enable bucket versioning
    When we add object "bar" with following attributes:
        """
        data_size: 10
        """
     And we add delete marker with name "bar"
     And we get info about object with name "bar"
    Then our object has attributes:
        """
        name: bar
        null_version: False
        delete_marker: True
        """
    Then bucket "foo" has "2" object(s) of size "13"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "0" object(s) of size "0"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
    When we get info about previous version of object "bar"
    Then our object has attributes:
        """
        name: bar
        data_size: 10
        null_version: False
        delete_marker: False
        """
    When we remove last version of object "bar"
    Then bucket "foo" has "1" object(s) of size "10"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "0" object(s) of size "0"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"

    When we suspend bucket versioning
     And we add delete marker with name "bar"
     And we get info about object with name "bar"
    Then our object has attributes:
        """
        name: bar
        null_version: True
        delete_marker: True
        """
    When we get info about previous version of object "bar"
    Then our object has attributes:
        """
        name: bar
        data_size: 10
        null_version: False
        delete_marker: False
        """
    Then bucket "foo" has "2" object(s) of size "13"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "0" object(s) of size "0"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters
