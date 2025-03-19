Feature: Drop multiple objects in bucket with keys
  Background: Connect to some shard
    Given empty DB
    And a bucket with name "languages" of account "1"
    And we add object "golang" with following attributes
      """
      data_size: 10
      data_md5: 11111111-1111-1111-1111-111111111111
      """
    And we add object "python" with following attributes
      """
      data_size: 20
      data_md5: 22222222-2222-2222-2222-222222222222
      """
    And we add object "cpp" with following attributes
      """
      data_size: 30
      data_md5: 33333333-3333-3333-3333-333333333333
      """
    And we enable bucket versioning
    And we add object "golang" with following attributes
      """
      data_size: 100
      data_md5: 11111111-1111-1111-1111-111111111110
      """
    And we add object "python" with following attributes
      """
      data_size: 200
      data_md5: 22222222-2222-2222-2222-222222222220
      """
    And we add object "cpp" with following attributes
      """
      data_size: 300
      data_md5: 33333333-3333-3333-3333-333333333330
      """
    Then bucket "languages" has "6" object(s) of size "660"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "0" object(s) of size "0"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"

  Scenario: Delete part of keys in bucket
    When we drop following multiple object's last versions
      """
      [golang, cpp]
      """
    Then we got result of multiple objects delete
      """
        - name: golang
          delete_marker: false
          delete_marker_created: null
          error: null
        - name: cpp
          delete_marker: false
          delete_marker_created: null
          error: null
      """
    Then bucket "languages" has "4" object(s) of size "260"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "2" object(s) of size "400"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"

  Scenario: Delete all keys in bucket
    When we drop following multiple object's last versions
      """
      [golang, cpp, python]
      """
    Then we got result of multiple objects delete
      """
        - name: golang
          delete_marker: false
          delete_marker_created: null
          error: null
        - name: cpp
          delete_marker: false
          delete_marker_created: null
          error: null
        - name: python
          delete_marker: false
          delete_marker_created: null
          error: null
      """
    When we list all versions in a bucket
    Then we get following list items:
        """
        - name: cpp
        - name: golang
        - name: python
        """
    When we drop following multiple object's last versions
      """
      [golang, cpp, python]
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
    Then bucket "languages" has "0" object(s) of size "0"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "6" object(s) of size "660"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"

  Scenario: Delete part of keys with nonexistent in bucket
    When we drop following multiple object's last versions
      """
      [golang, nonexistent_language]
      """
    Then we got result of multiple objects delete
      """
        - name: golang
          delete_marker: false
          delete_marker_created: null
          error: null
        - name: nonexistent_language
          delete_marker: null
          delete_marker_created: null
          error: S3K01
      """
    Then bucket "languages" has "5" object(s) of size "560"
     And bucket "languages" has "0" multipart object(s) of size "0"
     And bucket "languages" has "0" object part(s) of size "0"
     And bucket "languages" in delete queue has "1" object(s) of size "100"
     And bucket "languages" in delete queue has "0" object part(s) of size "0"

  Scenario: Delete zero keys
    When we drop following multiple objects
      """
      """
    Then we got result of multiple objects delete
      """
      """
     And bucket "languages" has "6" object(s) of size "660"
