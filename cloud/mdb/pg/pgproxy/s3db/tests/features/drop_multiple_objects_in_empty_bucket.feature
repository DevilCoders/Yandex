Feature: Drop multiple objects in empty bucket

  Background: Connect to some shard
    Given empty DB
    And a bucket with name "languages" of account "1"

  Scenario: Check empty bucket object dropping
    When we drop following multiple objects
      """
      - name: golang
        created: null
      - name: python
        created: null
      """
    Then we got result of multiple objects delete
      """
        - name: golang
          created: null
          delete_marker: null
          delete_marker_created: null
          error: S3K01
        - name: python
          created: null
          delete_marker: null
          delete_marker_created: null
          error: S3K01
      """
    Then bucket "languages" has "0" object(s) of size "0"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "0" object(s) of size "0"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"

  Scenario: Drop key with unicode symbol
    Given we add object "Ключ с юникодными символами" with following attributes
      """
      data_size: 10
      data_md5: 11111111-1111-1111-1111-111111111111
      mds_namespace: ns-1
      mds_couple_id: 111
      mds_key_version: 1
      mds_key_uuid: 11111111-1111-1111-1111-111111111111
      """

    When we drop following multiple objects
      """
      - name: Ключ с юникодными символами
        created: null
      """
    Then we got result of multiple objects delete
      """
        - name: Ключ с юникодными символами
          created: null
          delete_marker: false
          delete_marker_created: null
          error: null
      """
    Then bucket "languages" has "0" object(s) of size "0"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "1" object(s) of size "10"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"


