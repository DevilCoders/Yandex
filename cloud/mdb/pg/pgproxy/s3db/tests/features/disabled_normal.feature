Feature: Operations with basic objects in disabled versioning bucket

  Background: Set buckets owner account
    Given buckets owner account "1"

  Scenario: Adding normal object
    Given a bucket with name "foo"
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
        null_version: True
        delete_marker: False
        parts_count:
        parts:
        """
    Then bucket "foo" has "1" object(s) of size "10"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "0" object(s) of size "0"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters

  Scenario: Adding empty normal objects
    Given a bucket with name "foo"
    When we add object "empty_with_metadata" with following attributes:
        """
        data_size: 0
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    Then our object has attributes:
        """
        name: empty_with_metadata
        data_size: 0
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        null_version: True
        delete_marker: False
        parts_count:
        parts:
        """
    Then bucket "foo" has "2" object(s) of size "10"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "0" object(s) of size "0"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
    When we add object "empty_without_metadata" with following attributes:
        """
        data_size: 0
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace:
        mds_couple_id: NULL
        mds_key_version: NULL
        mds_key_uuid: NULL
        """
    Then our object has attributes:
        """
        name: empty_without_metadata
        data_size: 0
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace:
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        null_version: True
        delete_marker: False
        parts_count:
        parts:
        """
    Then bucket "foo" has "3" object(s) of size "10"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "0" object(s) of size "0"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"

  Scenario: Adding same object
    Given a bucket with name "foo"
    When we add object "bar" with following attributes:
        """
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then our object has attributes:
        """
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "foo" has "3" object(s) of size "20"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "1" object(s) of size "10"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
    # Now upload exactly the same object again (emulate retry).
    When we add object "bar" with following attributes:
        """
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then our object has attributes:
        """
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "foo" has "3" object(s) of size "20"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "1" object(s) of size "10"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
    # Upload empty object
    When we add object "empty" with following attributes:
        """
        data_size: 0
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace:
        mds_couple_id: NULL
        mds_key_version: NULL
        mds_key_uuid: NULL
        """
    Then our object has attributes:
        """
        data_size: 0
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace:
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        """
    Then bucket "foo" has "4" object(s) of size "20"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "1" object(s) of size "10"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
    # Now upload exactly the same empty object again (emulate retry).
    When we add object "empty" with following attributes:
        """
        data_size: 0
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace:
        mds_couple_id: NULL
        mds_key_version: NULL
        mds_key_uuid: NULL
        """
    Then our object has attributes:
        """
        data_size: 0
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace:
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        """
    Then bucket "foo" has "4" object(s) of size "20"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "1" object(s) of size "10"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters

  Scenario: Adding huge object
    Given a bucket with name "foo"
    When we add object "baz" with following attributes:
        """
        data_size: 5368709121
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 1
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
    Then we get an error with code "S3K03"
     And bucket "foo" has "4" object(s) of size "20"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "1" object(s) of size "10"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"


  Scenario: Getting object info shows actual version
    Given a bucket with name "foo"
    When we get info about object with name "bar"
    Then our object has attributes:
        """
        name: bar
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        null_version: True
        delete_marker: False
        parts_count:
        parts:
        """

  Scenario: Getting info about non-existent object
    Given a bucket with name "foo"
    When we get info about object with name "faramir"
    Then we get following objects:
        """
        """


  # NOTE: Currently only 'disabled' versioning is supported where all objects
  # have "null" version
  #Scenario: Getting info about non-existent version
  # Given a bucket with name "foo"
  # When we get info about object with name "bar" and created "1970-01-01 03:00:00+00"
  # Then we get an error with code "S3K02"


  Scenario: Dropping normal object
    Given a bucket with name "foo"
    When we drop object with name "bar"
    Then our object has attributes:
        """
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "foo" has "3" object(s) of size "0"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "2" object(s) of size "30"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters

  Scenario: Dropping non-existent object
    Given a bucket with name "foo"
    When we drop object with name "bar"
    Then we get following objects:
        """
        """
     And bucket "foo" has "3" object(s) of size "0"
     And bucket "foo" has "0" multipart object(s) of size "0"
     And bucket "foo" has "0" object part(s) of size "0"
     And bucket "foo" in delete queue has "2" object(s) of size "30"
     And bucket "foo" in delete queue has "0" object part(s) of size "0"


  # NOTE: Currently only 'disabled' versioning is supported where all objects
  # have "null" version
  #Scenario: Dropping non-existent object version
  # Given a bucket with name "foo"
  # When we drop object with name "aragorn" and created "1970-01-01 03:00:00+00"
  # Then we get an error with code "S3K02"
  #  And bucket "foo" has "0" object(s) of size "0"
  #  And bucket "foo" has "0" multipart object(s) of size "0"
  #  And bucket "foo" has "2" deleted object(s) of size "30"
  #  And bucket "foo" has "0" object part(s) of size "0"
  #  And bucket "foo" has "0" deleted object part(s) of size "0"
