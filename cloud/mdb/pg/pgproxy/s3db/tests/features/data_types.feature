Feature: Comparing data types definitions on DB hosts and PL/Proxy hosts

  Scenario: Types on s3meta hosts and PL/Proxy hosts
    Given dbname "s3proxy"
      And dbname "s3meta01"
    Then the following data types are the same:
        | schema    |     name     |
        | v1_code   | granted_role |
        | v1_code   | access_key   |
        | v1_code   | bucket       |
        | v1_code   | chunk        |
        | v1_code   | bucket_chunk |


  Scenario: Types on s3db hosts and PL/Proxy hosts
    Given dbname "s3proxy"
      And dbname "s3db01"
    Then the following data types are the same:
        | schema    |            name             |
        | v1_code   | object                      |
        | v1_code   | object_part                 |
        | v1_code   | object_part_data            |
        | s3        | object_part                 |
        | v1_code   | chunk_counters              |
        | v1_code   | multiple_drop_object        |
        | v1_code   | multiple_drop_object_result |


  Scenario: Enum types on s3meta hosts and PL/Proxy hosts
    Given dbname "s3proxy"
      And dbname "s3meta01"
     Then the following enum types are the same:
        | schema    |         name           |
        | s3        | account_status_type    |
        | s3        | role_type              |
        | s3        | bucket_versioning_type |
