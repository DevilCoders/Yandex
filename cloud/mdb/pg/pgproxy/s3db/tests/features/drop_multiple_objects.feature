Feature: Drop multiple objects in bucket with keys
  Background: Connect to some shard
    Given empty DB
    And a bucket with name "languages" of account "1"
    And we add object "golang" with following attributes
      """
      data_size: 10
      data_md5: 11111111-1111-1111-1111-111111111111
      mds_namespace: ns-1
      mds_couple_id: 111
      mds_key_version: 1
      mds_key_uuid: 11111111-1111-1111-1111-111111111111
      """
    And we add object "python" with following attributes
      """
      data_size: 20
      data_md5: 22222222-2222-2222-2222-222222222222
      mds_namespace: ns-2
      mds_couple_id: 222
      mds_key_version: 2
      mds_key_uuid: 22222222-2222-2222-2222-222222222222
      storage_class: 1
      """
    And we add object "cpp" with following attributes
      """
      data_size: 30
      data_md5: 33333333-3333-3333-3333-333333333333
      mds_namespace: ns-3
      mds_couple_id: 333
      mds_key_version: 3
      mds_key_uuid: 33333333-3333-3333-3333-333333333333
      storage_class: 2
      """
    Then bucket "languages" has "3" object(s) of size "60"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "0" object(s) of size "0"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"

  Scenario: Delete part of keys in bucket
    When we drop following multiple objects
      """
      - name: golang
        created: null
      - name: cpp
        created: null
      """
    Then we got result of multiple objects delete
      """
        - name: golang
          created: null
          delete_marker: false
          delete_marker_created: null
          error: null
        - name: cpp
          created: null
          delete_marker: false
          delete_marker_created: null
          error: null
      """
    Then bucket "languages" has "1" object(s) of size "20"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "2" object(s) of size "40"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"
    When we list all in the storage delete queue
    Then we get the following deleted object(s) as a result:
      """
      - name: cpp
        data_size: 30
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 3
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        storage_class: 2
      - name: golang
        data_size: 10
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: null
       """

    When we list all objects in a bucket
    Then we get following objects
      """
      - name: python
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
      """
     And we have no errors in counters

  Scenario: Delete all keys in bucket
    When we drop following multiple objects
      """
      - name: golang
        created: null
      - name: cpp
        created: null
      - name: python
        created: null
      """
    Then we got result of multiple objects delete
      """
        - name: golang
          created: null
          delete_marker: false
          delete_marker_created: null
          error: null
        - name: cpp
          created: null
          delete_marker: false
          delete_marker_created: null
          error: null
        - name: python
          created: null
          delete_marker: false
          delete_marker_created: null
          error: null
      """

    When we list all objects in a bucket
    Then we get following objects
      """
      """
    Then bucket "languages" has "0" object(s) of size "00"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "3" object(s) of size "60"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters

  Scenario: Delete part of keys with nonexisteng in bucket
    When we drop following multiple objects
      """
      - name: golang
        created: null
      - name: nonexistent_language
        created: null
      """
    Then we got result of multiple objects delete
      """
        - name: golang
          created: null
          delete_marker: false
          delete_marker_created: null
          error: null
        - name: nonexistent_language
          created: null
          delete_marker: null
          delete_marker_created: null
          error: S3K01
      """

    When we list all objects in a bucket
    Then we get following objects
      """
      - name: cpp
        data_size: 30
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 3
        mds_key_uuid: 33333333-3333-3333-3333-333333333333

      - name: python
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
      """
    Then bucket "languages" has "2" object(s) of size "50"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "1" object(s) of size "10"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters

  Scenario: Delete zero keys
    When we drop following multiple objects
      """
      """
    Then we got result of multiple objects delete
      """
      """

    When we list all objects in a bucket
    Then we get following objects
      """
      - name: cpp
        data_size: 30
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 3
        mds_key_uuid: 33333333-3333-3333-3333-333333333333

      - name: golang
        data_size: 10
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111

      - name: python
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
      """
    Then bucket "languages" has "3" object(s) of size "60"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "0" object(s) of size "0"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"

  Scenario: Delete object with wrong created, still delete
    When we drop following multiple objects
      """
      - name: cpp
        created: "1970-01-01 00:00:00+00"
      """
    Then we got result of multiple objects delete
      """
        - name: cpp
          created: null
          delete_marker: false
          delete_marker_created: null
          error: null
      """
    Then bucket "languages" has "2" object(s) of size "30"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "1" object(s) of size "30"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters

  Scenario: Delete object with created
    When we drop following multiple recent objects
      """
      - name: cpp
      """
    Then we got result of multiple recent objects delete
      """
        - name: cpp
          delete_marker: false
          delete_marker_created: null
          error: null
      """
    Then bucket "languages" has "2" object(s) of size "30"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "1" object(s) of size "30"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"

  Scenario: Delete part of keys with nonexisteng in versioned bucket
    When we enable bucket versioning
    When we drop following multiple objects
      """
      - name: golang
        created: null
      - name: nonexistent_language
        created: null
      """
    Then we got result of multiple objects delete
      """
        - name: golang
          delete_marker: true
          error: null
        - name: nonexistent_language
          delete_marker: true
          error: null
      """

    When we list all objects in a bucket
    Then we get following objects
      """
      - name: cpp
        data_size: 30
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 3
        mds_key_uuid: 33333333-3333-3333-3333-333333333333

      - name: python
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
      """
    When we list all versions in a bucket
    Then we get following list items:
        """
        - name: cpp
        - name: golang
        - name: nonexistent_language
        - name: python
        """
    Then bucket "languages" has "5" object(s) of size "86"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "0" object(s) of size "0"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters

  Scenario: Delete part of keys with nonexisteng in suspended bucket
    When we suspend bucket versioning
    When we drop following multiple objects
      """
      - name: golang
        created: null
      - name: nonexistent_language
        created: null
      """
    Then we got result of multiple objects delete
      """
        - name: golang
          created: null
          delete_marker: true
          error: null
        - name: nonexistent_language
          created: null
          delete_marker: true
          error: null
      """

    When we list all objects in a bucket
    Then we get following objects
      """
      - name: cpp
        data_size: 30
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 3
        mds_key_uuid: 33333333-3333-3333-3333-333333333333

      - name: python
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
      """
    When we list all versions in a bucket
    Then we get following list items:
        """
        - name: cpp
        - name: golang
        - name: nonexistent_language
        - name: python
        """
    Then bucket "languages" has "4" object(s) of size "76"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "1" object(s) of size "10"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters

  Scenario: Delete part of keys twice in versioned bucket
    When we enable bucket versioning
    When we drop following multiple objects
      """
      - name: golang
        created: null
      """
    Then we got result of multiple objects delete
      """
        - name: golang
          delete_marker: true
          error: null
      """
    When we drop following multiple objects
      """
      - name: golang
        created: null
      """
    Then we got result of multiple objects delete
      """
        - name: golang
          delete_marker: true
          error: null
      """

    When we list all objects in a bucket
    Then we get following objects
      """
      - name: cpp
        data_size: 30
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 3
        mds_key_uuid: 33333333-3333-3333-3333-333333333333

      - name: python
        data_size: 20
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 2
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
      """
    When we list all versions in a bucket
    Then we get following list items:
        """
        - name: cpp
        - name: golang
        - name: python
        """
    Then bucket "languages" has "5" object(s) of size "72"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "0" object(s) of size "0"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"
     And we have no errors in counters
