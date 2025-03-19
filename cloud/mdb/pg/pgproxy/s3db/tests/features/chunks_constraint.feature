Feature: Testing chunks related constraints

  Background: Set buckets owner account
    Given buckets owner account "1"

  Scenario: Testing "tg_chunk_check_empty" constraint trigger on s3chunk in s3db
    Given empty DB
    Given a bucket with name "tg_chunk_check_empty"
    When we add object "Object1" with following attributes:
        """
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we delete this chunk
    Then we get an error with code "23503"
