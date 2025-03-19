Feature: Management of ClickHouse users

  Background:
    Given default headers
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }],
        "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker

  @users @get
  Scenario: User get works
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/users/test"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test",
        "permissions": [
            {
                "databaseName": "testdb"
            }
        ],
        "settings": {}
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test",
        "permissions": [{
            "database_name": "testdb",
            "data_filters": []
        }],
        "quotas": []
    }
    """

  @users @list
  Scenario: User list works
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb"
                    }
                ],
                "settings": {},
                "quotas" : []
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "users": [{
            "cluster_id": "cid1",
            "name": "test",
            "permissions": [{
                "database_name": "testdb",
                "data_filters": []
            }],
            "quotas": [],
            "settings": {
               "add_http_cors_header": null,
               "allow_ddl": null,
               "compile": null,
               "compile_expressions": null,
               "connect_timeout": null,
               "count_distinct_implementation": "COUNT_DISTINCT_IMPLEMENTATION_UNSPECIFIED",
               "distinct_overflow_mode": "OVERFLOW_MODE_UNSPECIFIED",
               "distributed_aggregation_memory_efficient": null,
               "distributed_ddl_task_timeout": null,
               "distributed_product_mode": "DISTRIBUTED_PRODUCT_MODE_UNSPECIFIED",
               "empty_result_for_aggregation_by_empty_set": null,
               "enable_http_compression": null,
               "fallback_to_stale_replicas_for_distributed_queries": null,
               "force_index_by_date": null,
               "force_primary_key": null,
               "group_by_overflow_mode": "GROUP_BY_OVERFLOW_MODE_UNSPECIFIED",
               "group_by_two_level_threshold": null,
               "group_by_two_level_threshold_bytes": null,
               "http_connection_timeout": null,
               "http_headers_progress_interval": null,
               "http_receive_timeout": null,
               "http_send_timeout": null,
               "input_format_defaults_for_omitted_fields": null,
               "input_format_values_interpret_expressions": null,
               "insert_quorum": null,
               "insert_quorum_timeout": null,
               "join_overflow_mode": "OVERFLOW_MODE_UNSPECIFIED",
               "join_use_nulls": null,
               "joined_subquery_requires_alias": null,
               "low_cardinality_allow_in_native_format": null,
               "max_ast_depth": null,
               "max_ast_elements": null,
               "max_block_size": null,
               "max_bytes_before_external_group_by": null,
               "max_bytes_before_external_sort": null,
               "max_bytes_in_distinct": null,
               "max_bytes_in_join": null,
               "max_bytes_in_set": null,
               "max_bytes_to_read": null,
               "max_bytes_to_sort": null,
               "max_bytes_to_transfer": null,
               "max_columns_to_read": null,
               "max_execution_time": null,
               "max_expanded_ast_elements": null,
               "max_insert_block_size": null,
               "max_memory_usage": null,
               "max_memory_usage_for_user": null,
               "max_network_bandwidth": null,
               "max_network_bandwidth_for_user": null,
               "max_query_size": null,
               "max_replica_delay_for_distributed_queries": null,
               "max_result_bytes": null,
               "max_result_rows": null,
               "max_rows_in_distinct": null,
               "max_rows_in_join": null,
               "max_rows_in_set": null,
               "max_rows_to_group_by": null,
               "max_rows_to_read": null,
               "max_rows_to_sort": null,
               "max_rows_to_transfer": null,
               "max_temporary_columns": null,
               "max_temporary_non_const_columns": null,
               "max_threads": null,
               "merge_tree_max_bytes_to_use_cache": null,
               "merge_tree_max_rows_to_use_cache": null,
               "merge_tree_min_bytes_for_concurrent_read": null,
               "merge_tree_min_rows_for_concurrent_read": null,
               "min_bytes_for_wide_part": null,
               "min_bytes_to_use_direct_io": null,
               "min_count_to_compile": null,
               "min_count_to_compile_expression": null,
               "min_execution_speed": null,
               "min_execution_speed_bytes": null,
               "min_insert_block_size_bytes": null,
               "min_insert_block_size_rows": null,
               "min_rows_for_wide_part": null,
               "output_format_json_quote_64bit_integers": null,
               "output_format_json_quote_denormals": null,
               "priority": null,
               "quota_mode": "QUOTA_MODE_UNSPECIFIED",
               "read_overflow_mode": "OVERFLOW_MODE_UNSPECIFIED",
               "readonly": null,
               "receive_timeout": null,
               "replication_alter_partitions_sync": null,
               "result_overflow_mode": "OVERFLOW_MODE_UNSPECIFIED",
               "select_sequential_consistency": null,
               "send_progress_in_http_headers": null,
               "send_timeout": null,
               "set_overflow_mode": "OVERFLOW_MODE_UNSPECIFIED",
               "skip_unavailable_shards": null,
               "sort_overflow_mode": "OVERFLOW_MODE_UNSPECIFIED",
               "timeout_overflow_mode": "OVERFLOW_MODE_UNSPECIFIED",
               "transfer_overflow_mode": "OVERFLOW_MODE_UNSPECIFIED",
               "transform_null_in": null,
               "use_uncompressed_cache": null
            }
        }]
    }
    """

  @users @list
  Scenario: List users with pagination works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": [{
                "databaseName": "testdb"
            }]
        }
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password",
            "permissions": [{
                "database_name": "testdb"
            }]
        }
    }
    """
    Then we get gRPC response OK
    Given "worker_task_id2" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test3",
            "password": "password",
            "permissions": [{
                "databaseName": "testdb"
            }]
        }
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test3",
            "password": "password",
            "permissions": [{
                "database_name": "testdb"
            }]
        }
    }
    """
    Then we get gRPC response OK
    Given "worker_task_id3" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 2
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "users": [
            {
                "cluster_id": "cid1",
                "name": "test",
                "permissions": [{
                    "database_name": "testdb"
                }],
                "settings": {}
            }, {
                "cluster_id": "cid1",
                "name": "test2",
                "permissions": [{
                    "database_name": "testdb"
                }],
                "settings": {}
            }
        ],
        "next_page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "users": [{
            "cluster_id": "cid1",
            "name": "test3",
            "permissions": [{
                "database_name": "testdb"
            }],
            "settings": {}
        }]
    }
    """

  @users @get
  Scenario: Nonexistent user get fails
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "User 'test2' does not exist"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "test2" not found"

  @users @create
  Scenario Outline: Adding user <name> with invalid params fails
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "<name>",
            "password": "<password>",
            "permissions": [{
                "databaseName": "<database>"
            }]
        }
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<error message>"
    }
    """
    Examples:
      | name        | password | database | status | error code | error message                                                                                  |
      | test        | password | testdb   | 409    | 6          | User 'test' already exists                                                                     |
      | test2       | password | nodb     | 422    | 3          | Database 'nodb' does not exist                                                                 |
      | b@dn@me!    | password | testdb   | 422    | 3          | The request is invalid.\nuserSpec.name: User name 'b@dn@me!' does not conform to naming rules  |
      | 0_badname   | password | testdb   | 422    | 3          | The request is invalid.\nuserSpec.name: User name '0_badname' does not conform to naming rules |
      | user        | short    | testdb   | 422    | 3          | The request is invalid.\nuserSpec.password: Password must be between 8 and 128 characters long |
      | default     | password | testdb   | 422    | 3          | User name 'default' is not allowed                                                             |
      | mdb_metrics | password | testdb   | 422    | 3          | User name 'mdb_metrics' is not allowed                                                         |
      | mdb_33      | password | testdb   | 422    | 3          | User name 'mdb_33' is not allowed                                                              |

  @users @create
  Scenario Outline: Adding user via gRPC with invalid params fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "<name>",
            "password": "<password>",
            "permissions": [{
                "database_name": "<database>"
            }]
        }
    }
    """
    Then we get gRPC response error with code <code> and message "<error message>"
    Examples:
      | name        | password | database | code             | error message                             |
      | test        | password | testdb   | ALREADY_EXISTS   | user "test" already exists                |
      | test2       | password | nodb     | NOT_FOUND        | database "nodb" not found                 |
      | b@dn@me!    | password | testdb   | INVALID_ARGUMENT | user name "b@dn@me!" has invalid symbols  |
      | 0_badname   | password | testdb   | INVALID_ARGUMENT | user name "0_badname" has invalid symbols |
      | user        | short    | testdb   | INVALID_ARGUMENT | password is too short                     |
      | default     | password | testdb   | INVALID_ARGUMENT | invalid user name "default"               |
      | mdb_metrics | password | testdb   | INVALID_ARGUMENT | invalid user name "mdb_metrics"           |
      | mdb_33      | password | testdb   | INVALID_ARGUMENT | invalid user name "mdb_33"                |

  # do not test this case with gRPC api, because can't differentiate empty and null list in protobuf message.
  @users @create @events
  Scenario: Adding user with empty database acl works
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.CreateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [],
        "settings": {}
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test2",
        "permissions": [],
        "quotas": []
    }
    """

  @users @create
  Scenario: Adding user with default database acl works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password"
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "settings": {}
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test2",
        "permissions": [{
            "database_name": "testdb",
            "data_filters": []
        }],
        "quotas": []
    }
    """
    Given for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.CreateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Create user in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.User",
            "cluster_id": "cid1",
            "name": "test2",
            "permissions": [{
                "database_name": "testdb"
            }],
            "settings": {}
        }
    }
    """

  @users @update @events
  Scenario: Modifying user with permission for database works
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2" with data
    """
    {
        "permissions": [{
            "databaseName": "testdb"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2",
        "permissions": [{
            "database_name": "testdb"
        }],
        "update_mask": {
            "paths": ["permissions"]
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Update user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.UpdateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "settings": {}
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test2",
        "permissions": [{
            "database_name": "testdb",
            "data_filters": []
        }],
        "quotas": []
    }
    """
    Given "worker_task_id3" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id3"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Update user in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.User",
            "cluster_id": "cid1",
            "name": "test2",
            "permissions": [{"database_name": "testdb"}],
            "settings": {}
        }
    }
    """
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2" with data
    """
    {
        "permissions": []
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2",
        "permissions": [],
        "update_mask": {
            "paths": ["permissions"]
        }
    }
    """
    And we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [],
        "settings": {}
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test2",
        "permissions": [],
        "quotas": []
    }
    """

  @users @grant-permission @events
  Scenario: Adding permission for database works
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2:grantPermission" with data
    """
    {
        "permission": {
            "databaseName": "testdb"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Grant permission to user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.GrantUserPermissionMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we "GrantPermission" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2",
        "permission": {
            "database_name": "testdb"
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Grant permission to user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.GrantUserPermissionMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.GrantUserPermission" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id3"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Grant permission to user in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.GrantUserPermissionMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.User",
            "cluster_id": "cid1",
            "name": "test2",
            "permissions": [{"database_name": "testdb"}],
            "settings": {}
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "settings": {}
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test2",
        "permissions": [{
            "database_name": "testdb",
            "data_filters": []
        }],
        "quotas": []
    }
    """

  @users @grant-permission @events
  Scenario: Adding row-level permission works
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2:grantPermission" with data
    """
    {
        "permission": {
            "databaseName": "testdb",
            "dataFilters": [
                {
                    "tableName": "foo",
                    "filter": "bar = 1"
                }
            ]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Grant permission to user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.GrantUserPermissionMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we "GrantPermission" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2",
        "permission": {
            "database_name": "testdb",
            "data_filters": [
              {
                "table_name": "foo",
                "filter": "bar = 1"
              }
            ]
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Grant permission to user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.GrantUserPermissionMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.GrantUserPermission" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb",
            "dataFilters": [
                {
                    "tableName": "foo",
                    "filter": "bar = 1"
                }
            ]
        }],
        "settings": {}
    }
    """

  @users @grant-permission
  Scenario Outline: Adding permission for database with invalid params fails
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/users/<name>:grantPermission" with data
    """
    {
        "permission": {
            "databaseName": "<database>"
        }
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | name   | database | status | error code | message                        |
      | nouser | testdb   | 404    | 5          | User 'nouser' does not exist   |
      | test   | nodb     | 422    | 3          | Database 'nodb' does not exist |

  @users @grant-permission
  Scenario Outline: Adding permission for database via gRPC with invalid params fails
    When we "GrantPermission" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "<name>",
        "permission": {
            "database_name": "<database>"
        }
    }
    """
    Then we get gRPC response error with code <code> and message "<message>"
    Examples:
      | name   | database | code      | message                   |
      | nouser | testdb   | NOT_FOUND | user "nouser" not found   |
      | test   | nodb     | NOT_FOUND | database "nodb" not found |

  @users @revoke-permission @events
  Scenario: Revoking permission for database works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": [{
                "databaseName": "testdb"
            }]
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password",
            "permissions": [{
                "database_name": "testdb"
            }]
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2:revokePermission" with data
    """
    {
        "databaseName": "testdb"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Revoke permission from user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.RevokeUserPermissionMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we "RevokePermission" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2",
        "database_name": "testdb"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Revoke permission from user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.RevokeUserPermissionMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.RevokeUserPermission" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id3"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Revoke permission from user in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.RevokeUserPermissionMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.User",
            "cluster_id": "cid1",
            "name": "test2",
            "settings": {}
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [],
        "settings": {}
    }
    """

  @users @revoke-permission
  Scenario Outline: Revoking permission for database with invalid params fails
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    And we POST "/mdb/clickhouse/1.0/clusters/cid1/users/<name>:revokePermission" with data
    """
    {
        "databaseName": "<database>"
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | name   | database | status | error code | message                                             |
      | nouser | testdb   | 404    | 5          | User 'nouser' does not exist                        |
      | test   | nodb     | 422    | 3          | Database 'nodb' does not exist                      |
      | test2  | testdb   | 409    | 6          | User 'test2' has no access to the database 'testdb' |

  @users @revoke-permission
  Scenario Outline: Revoking permission for database via gRPC with invalid params fails
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    When we "RevokePermission" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "<name>",
        "database_name": "<database>"
    }
    """
    Then we get gRPC response error with code <code> and message "<message>"
    Examples:
      | name   | database | code                | message                                             |
      | nouser | testdb   | NOT_FOUND           | user "nouser" not found                             |
      | test   | nodb     | NOT_FOUND           | database "nodb" not found                           |
      | test2  | testdb   | FAILED_PRECONDITION | user "test2" has no access to the database "testdb" |

  @users @delete @events
  Scenario: Deleting user works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.DeleteUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.DeleteUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.DeleteUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb"
                    }
                ],
                "settings": {},
                "quotas" : []
            }
        ]
    }
    """

  @users @delete
  Scenario: Deleting nonexistent user fails
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "User 'test2' does not exist"
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "test2" not found"

  @users @delete
  Scenario: Deleting system user default fails
    When we DELETE "/mdb/clickhouse/1.0/clusters/cid1/users/default"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "User 'default' does not exist"
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "default"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "default" not found"

  @users @update @events
  Scenario: Changing user password works
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test" with data
    """
    {
        "password": "changed password"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test",
        "password": "changed password",
        "update_mask": {
            "paths": ["password"]
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Update user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.UpdateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test"
        }
    }
    """

  @users @update
  Scenario: Changing system user default password fails
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/default" with data
    """
    {
        "password": "changed password"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "User 'default' does not exist"
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "default",
        "password": "changed password",
        "update_mask": {
            "paths": ["password"]
        }
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "default" not found"

  @users @create
  Scenario: Adding user with custom settings works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "settings": {
                "readonly": 2,
                "allowDdl": false,

                "connectTimeout": 20000,
                "receiveTimeout": 400000,
                "sendTimeout": 400000,

                "insertQuorum": 2,
                "insertQuorumTimeout": 30000,
                "selectSequentialConsistency": true,
                "replicationAlterPartitionsSync": 2,

                "maxReplicaDelayForDistributedQueries": 300000,
                "fallbackToStaleReplicasForDistributedQueries": true,
                "distributedProductMode": "DISTRIBUTED_PRODUCT_MODE_ALLOW",
                "distributedAggregationMemoryEfficient": true,
                "distributedDdlTaskTimeout": 360000,
                "skipUnavailableShards": true,

                "compile": true,
                "minCountToCompile": 2,
                "compileExpressions": true,
                "minCountToCompileExpression": 2,

                "maxBlockSize": 32768,
                "minInsertBlockSizeRows": 1048576,
                "minInsertBlockSizeBytes": 268435456,
                "maxInsertBlockSize": 524288,
                "minBytesToUseDirectIo": 52428800,
                "useUncompressedCache": true,
                "mergeTreeMaxRowsToUseCache": 1048576,
                "mergeTreeMaxBytesToUseCache": 2013265920,
                "mergeTreeMinRowsForConcurrentRead": 163840,
                "mergeTreeMinBytesForConcurrentRead": 251658240,
                "maxBytesBeforeExternalGroupBy": 0,
                "maxBytesBeforeExternalSort": 0,
                "groupByTwoLevelThreshold": 100000,
                "groupByTwoLevelThresholdBytes": 100000000,

                "priority": 1,
                "maxThreads": 10,
                "maxMemoryUsage": 21474836480,
                "maxMemoryUsageForUser": 53687091200,
                "maxNetworkBandwidth": 1073741824,
                "maxNetworkBandwidthForUser": 2147483648,

                "forceIndexByDate": true,
                "forcePrimaryKey": true,
                "maxRowsToRead": 1000000,
                "maxBytesToRead": 2000000,
                "readOverflowMode": "OVERFLOW_MODE_THROW",
                "maxRowsToGroupBy": 1000001,
                "groupByOverflowMode": "GROUP_BY_OVERFLOW_MODE_ANY",
                "maxRowsToSort": 1000002,
                "maxBytesToSort": 2000002,
                "sortOverflowMode": "OVERFLOW_MODE_THROW",
                "maxResultRows": 1000003,
                "maxResultBytes": 2000003,
                "resultOverflowMode": "OVERFLOW_MODE_THROW",
                "maxRowsInDistinct": 1000004,
                "maxBytesInDistinct": 2000004,
                "distinctOverflowMode": "OVERFLOW_MODE_THROW",
                "maxRowsToTransfer": 1000005,
                "maxBytesToTransfer": 2000005,
                "transferOverflowMode": "OVERFLOW_MODE_THROW",
                "maxExecutionTime": 600000,
                "timeoutOverflowMode": "OVERFLOW_MODE_THROW",
                "maxRowsInSet": 1000006,
                "maxBytesInSet": 2000006,
                "setOverflowMode": "OVERFLOW_MODE_THROW",
                "maxRowsInJoin": 1000007,
                "maxBytesInJoin": 2000007,
                "joinOverflowMode": "OVERFLOW_MODE_THROW",
                "maxColumnsToRead": 25,
                "maxTemporaryColumns": 20,
                "maxTemporaryNonConstColumns": 15,
                "maxQuerySize": 524288,
                "maxAstDepth": 2000,
                "maxAstElements": 100000,
                "maxExpandedAstElements": 1000000,
                "minExecutionSpeed": 1000008,
                "minExecutionSpeedBytes": 2000008,

                "inputFormatValuesInterpretExpressions": true,
                "inputFormatDefaultsForOmittedFields": true,
                "outputFormatJsonQuote_64bitIntegers": true,
                "outputFormatJsonQuoteDenormals": true,
                "lowCardinalityAllowInNativeFormat": true,
                "emptyResultForAggregationByEmptySet": true,

                "httpConnectionTimeout": 3000,
                "httpReceiveTimeout": 1800000,
                "httpSendTimeout": 1800000,
                "enableHttpCompression": true,
                "sendProgressInHttpHeaders": true,
                "httpHeadersProgressInterval": 1000,
                "addHttpCorsHeader": true,

                "quotaMode": "QUOTA_MODE_KEYED",

                "countDistinctImplementation": "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12",
                "joinedSubqueryRequiresAlias": true,
                "joinUseNulls": true,
                "transformNullIn": true
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password",
            "settings": {
                "readonly": 2,
                "allow_ddl": false,

                "connect_timeout": 20000,
                "receive_timeout": 400000,
                "send_timeout": 400000,

                "insert_quorum": 2,
                "insert_quorum_timeout": 30000,
                "select_sequential_consistency": true,
                "replication_alter_partitions_sync": 2,

                "max_replica_delay_for_distributed_queries": 300000,
                "fallback_to_stale_replicas_for_distributed_queries": true,
                "distributed_product_mode": "DISTRIBUTED_PRODUCT_MODE_ALLOW",
                "distributed_aggregation_memory_efficient": true,
                "distributed_ddl_task_timeout": 360000,
                "skip_unavailable_shards": true,

                "compile": true,
                "min_count_to_compile": 2,
                "compile_expressions": true,
                "min_count_to_compile_expression": 2,

                "max_block_size": 32768,
                "min_insert_block_size_rows": 1048576,
                "min_insert_block_size_bytes": 268435456,
                "max_insert_block_size": 524288,
                "min_bytes_to_use_direct_io": 52428800,
                "use_uncompressed_cache": true,
                "merge_tree_max_rows_to_use_cache": 1048576,
                "merge_tree_max_bytes_to_use_cache": 2013265920,
                "merge_tree_min_rows_for_concurrent_read": 163840,
                "merge_tree_min_bytes_for_concurrent_read": 251658240,
                "max_bytes_before_external_group_by": 0,
                "max_bytes_before_external_sort": 0,
                "group_by_two_level_threshold": 100000,
                "group_by_two_level_threshold_bytes": 100000000,

                "priority": 1,
                "max_threads": 10,
                "max_memory_usage": 21474836480,
                "max_memory_usage_for_user": 53687091200,
                "max_network_bandwidth": 1073741824,
                "max_network_bandwidth_for_user": 2147483648,

                "force_index_by_date": true,
                "force_primary_key": true,
                "max_rows_to_read": 1000000,
                "max_bytes_to_read": 2000000,
                "read_overflow_mode": "OVERFLOW_MODE_THROW",
                "max_rows_to_group_by": 1000001,
                "group_by_overflow_mode": "GROUP_BY_OVERFLOW_MODE_ANY",
                "max_rows_to_sort": 1000002,
                "max_bytes_to_sort": 2000002,
                "sort_overflow_mode": "OVERFLOW_MODE_THROW",
                "max_result_rows": 1000003,
                "max_result_bytes": 2000003,
                "result_overflow_mode": "OVERFLOW_MODE_THROW",
                "max_rows_in_distinct": 1000004,
                "max_bytes_in_distinct": 2000004,
                "distinct_overflow_mode": "OVERFLOW_MODE_THROW",
                "max_rows_to_transfer": 1000005,
                "max_bytes_to_transfer": 2000005,
                "transfer_overflow_mode": "OVERFLOW_MODE_THROW",
                "max_execution_time": 600000,
                "timeout_overflow_mode": "OVERFLOW_MODE_THROW",
                "max_rows_in_set": 1000006,
                "max_bytes_in_set": 2000006,
                "set_overflow_mode": "OVERFLOW_MODE_THROW",
                "max_rows_in_join": 1000007,
                "max_bytes_in_join": 2000007,
                "join_overflow_mode": "OVERFLOW_MODE_THROW",
                "max_columns_to_read": 25,
                "max_temporary_columns": 20,
                "max_temporary_non_const_columns": 15,
                "max_query_size": 524288,
                "max_ast_depth": 2000,
                "max_ast_elements": 100000,
                "max_expanded_ast_elements": 1000000,
                "min_execution_speed": 1000008,
                "min_execution_speed_bytes": 2000008,

                "input_format_values_interpret_expressions": true,
                "input_format_defaults_for_omitted_fields": true,
                "output_format_json_quote_64bit_integers": true,
                "output_format_json_quote_denormals": true,
                "low_cardinality_allow_in_native_format": true,
                "empty_result_for_aggregation_by_empty_set": true,

                "http_connection_timeout": 3000,
                "http_receive_timeout": 1800000,
                "http_send_timeout": 1800000,
                "enable_http_compression": true,
                "send_progress_in_http_headers": true,
                "http_headers_progress_interval": 1000,
                "add_http_cors_header": true,

                "quota_mode": "QUOTA_MODE_KEYED",

                "count_distinct_implementation": "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12",
                "joined_subquery_requires_alias": true,
                "join_use_nulls": true,
                "transform_null_in": true
            }
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "settings": {
            "readonly": 2,
            "allowDdl": false,

            "connectTimeout": 20000,
            "receiveTimeout": 400000,
            "sendTimeout": 400000,

            "insertQuorum": 2,
            "insertQuorumTimeout": 30000,
            "selectSequentialConsistency": true,
            "replicationAlterPartitionsSync": 2,

            "maxReplicaDelayForDistributedQueries": 300000,
            "fallbackToStaleReplicasForDistributedQueries": true,
            "distributedProductMode": "DISTRIBUTED_PRODUCT_MODE_ALLOW",
            "distributedAggregationMemoryEfficient": true,
            "distributedDdlTaskTimeout": 360000,
            "skipUnavailableShards": true,

            "compile": true,
            "minCountToCompile": 2,
            "compileExpressions": true,
            "minCountToCompileExpression": 2,

            "maxBlockSize": 32768,
            "minInsertBlockSizeRows": 1048576,
            "minInsertBlockSizeBytes": 268435456,
            "maxInsertBlockSize": 524288,
            "minBytesToUseDirectIo": 52428800,
            "useUncompressedCache": true,
            "mergeTreeMaxRowsToUseCache": 1048576,
            "mergeTreeMaxBytesToUseCache": 2013265920,
            "mergeTreeMinRowsForConcurrentRead": 163840,
            "mergeTreeMinBytesForConcurrentRead": 251658240,
            "maxBytesBeforeExternalGroupBy": 0,
            "maxBytesBeforeExternalSort": 0,
            "groupByTwoLevelThreshold": 100000,
            "groupByTwoLevelThresholdBytes": 100000000,

            "priority": 1,
            "maxThreads": 10,
            "maxMemoryUsage": 21474836480,
            "maxMemoryUsageForUser": 53687091200,
            "maxNetworkBandwidth": 1073741824,
            "maxNetworkBandwidthForUser": 2147483648,

            "forceIndexByDate": true,
            "forcePrimaryKey": true,
            "maxRowsToRead": 1000000,
            "maxBytesToRead": 2000000,
            "readOverflowMode": "OVERFLOW_MODE_THROW",
            "maxRowsToGroupBy": 1000001,
            "groupByOverflowMode": "GROUP_BY_OVERFLOW_MODE_ANY",
            "maxRowsToSort": 1000002,
            "maxBytesToSort": 2000002,
            "sortOverflowMode": "OVERFLOW_MODE_THROW",
            "maxResultRows": 1000003,
            "maxResultBytes": 2000003,
            "resultOverflowMode": "OVERFLOW_MODE_THROW",
            "maxRowsInDistinct": 1000004,
            "maxBytesInDistinct": 2000004,
            "distinctOverflowMode": "OVERFLOW_MODE_THROW",
            "maxRowsToTransfer": 1000005,
            "maxBytesToTransfer": 2000005,
            "transferOverflowMode": "OVERFLOW_MODE_THROW",
            "maxExecutionTime": 600000,
            "timeoutOverflowMode": "OVERFLOW_MODE_THROW",
            "maxRowsInSet": 1000006,
            "maxBytesInSet": 2000006,
            "setOverflowMode": "OVERFLOW_MODE_THROW",
            "maxRowsInJoin": 1000007,
            "maxBytesInJoin": 2000007,
            "joinOverflowMode": "OVERFLOW_MODE_THROW",
            "maxColumnsToRead": 25,
            "maxTemporaryColumns": 20,
            "maxTemporaryNonConstColumns": 15,
            "maxQuerySize": 524288,
            "maxAstDepth": 2000,
            "maxAstElements": 100000,
            "maxExpandedAstElements": 1000000,
            "minExecutionSpeed": 1000008,
            "minExecutionSpeedBytes": 2000008,

            "inputFormatValuesInterpretExpressions": true,
            "inputFormatDefaultsForOmittedFields": true,
            "outputFormatJsonQuote_64bitIntegers": true,
            "outputFormatJsonQuoteDenormals": true,
            "lowCardinalityAllowInNativeFormat": true,
            "emptyResultForAggregationByEmptySet": true,

            "httpConnectionTimeout": 3000,
            "httpReceiveTimeout": 1800000,
            "httpSendTimeout": 1800000,
            "enableHttpCompression": true,
            "sendProgressInHttpHeaders": true,
            "httpHeadersProgressInterval": 1000,
            "addHttpCorsHeader": true,

            "quotaMode": "QUOTA_MODE_KEYED",

            "countDistinctImplementation": "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12",
            "joinedSubqueryRequiresAlias": true,
            "joinUseNulls": true,
            "transformNullIn": true
        },
        "quotas" : []
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test2",
        "permissions": [{
            "database_name": "testdb",
            "data_filters": []
        }],
        "quotas": [],
        "settings": {
            "readonly": "2",
            "allow_ddl": false,

            "connect_timeout": "20000",
            "receive_timeout": "400000",
            "send_timeout": "400000",

            "insert_quorum": "2",
            "insert_quorum_timeout": "30000",
            "select_sequential_consistency": true,
            "replication_alter_partitions_sync": "2",

            "max_replica_delay_for_distributed_queries": "300000",
            "fallback_to_stale_replicas_for_distributed_queries": true,
            "distributed_product_mode": "DISTRIBUTED_PRODUCT_MODE_ALLOW",
            "distributed_aggregation_memory_efficient": true,
            "distributed_ddl_task_timeout": "360000",
            "skip_unavailable_shards": true,

            "compile": true,
            "min_count_to_compile": "2",
            "compile_expressions": true,
            "min_count_to_compile_expression": "2",

            "max_block_size": "32768",
            "min_insert_block_size_rows": "1048576",
            "min_insert_block_size_bytes": "268435456",
            "max_insert_block_size": "524288",
            "min_bytes_to_use_direct_io": "52428800",
            "use_uncompressed_cache": true,
            "merge_tree_max_rows_to_use_cache": "1048576",
            "merge_tree_max_bytes_to_use_cache": "2013265920",
            "merge_tree_min_rows_for_concurrent_read": "163840",
            "merge_tree_min_bytes_for_concurrent_read": "251658240",
            "max_bytes_before_external_group_by": "0",
            "max_bytes_before_external_sort": "0",
            "group_by_two_level_threshold": "100000",
            "group_by_two_level_threshold_bytes": "100000000",
            "min_rows_for_wide_part": null,
            "min_bytes_for_wide_part": null,

            "priority": "1",
            "max_threads": "10",
            "max_memory_usage": "21474836480",
            "max_memory_usage_for_user": "53687091200",
            "max_network_bandwidth": "1073741824",
            "max_network_bandwidth_for_user": "2147483648",

            "force_index_by_date": true,
            "force_primary_key": true,
            "max_rows_to_read": "1000000",
            "max_bytes_to_read": "2000000",
            "read_overflow_mode": "OVERFLOW_MODE_THROW",
            "max_rows_to_group_by": "1000001",
            "group_by_overflow_mode": "GROUP_BY_OVERFLOW_MODE_ANY",
            "max_rows_to_sort": "1000002",
            "max_bytes_to_sort": "2000002",
            "sort_overflow_mode": "OVERFLOW_MODE_THROW",
            "max_result_rows": "1000003",
            "max_result_bytes": "2000003",
            "result_overflow_mode": "OVERFLOW_MODE_THROW",
            "max_rows_in_distinct": "1000004",
            "max_bytes_in_distinct": "2000004",
            "distinct_overflow_mode": "OVERFLOW_MODE_THROW",
            "max_rows_to_transfer": "1000005",
            "max_bytes_to_transfer": "2000005",
            "transfer_overflow_mode": "OVERFLOW_MODE_THROW",
            "max_execution_time": "600000",
            "timeout_overflow_mode": "OVERFLOW_MODE_THROW",
            "max_rows_in_set": "1000006",
            "max_bytes_in_set": "2000006",
            "set_overflow_mode": "OVERFLOW_MODE_THROW",
            "max_rows_in_join": "1000007",
            "max_bytes_in_join": "2000007",
            "join_overflow_mode": "OVERFLOW_MODE_THROW",
            "max_columns_to_read": "25",
            "max_temporary_columns": "20",
            "max_temporary_non_const_columns": "15",
            "max_query_size": "524288",
            "max_ast_depth": "2000",
            "max_ast_elements": "100000",
            "max_expanded_ast_elements": "1000000",
            "min_execution_speed": "1000008",
            "min_execution_speed_bytes": "2000008",

            "input_format_values_interpret_expressions": true,
            "input_format_defaults_for_omitted_fields": true,
            "output_format_json_quote_64bit_integers": true,
            "output_format_json_quote_denormals": true,
            "low_cardinality_allow_in_native_format": true,
            "empty_result_for_aggregation_by_empty_set": true,

            "http_connection_timeout": "3000",
            "http_receive_timeout": "1800000",
            "http_send_timeout": "1800000",
            "enable_http_compression": true,
            "send_progress_in_http_headers": true,
            "http_headers_progress_interval": "1000",
            "add_http_cors_header": true,

            "quota_mode": "QUOTA_MODE_KEYED",

            "count_distinct_implementation": "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12",
            "joined_subquery_requires_alias": true,
            "join_use_nulls": true,
            "transform_null_in": true
        }
    }
    """

  @users @create
  Scenario Outline: Adding user with invalid settings fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "settings": {
                "<name>": <value>
            }
        }
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<error message>"
    }
    """
    Examples:
      | name                | value     | status | error code | error message                                                                                                                                                         |
      | readonly            | "invalid" | 422    | 3          | The request is invalid.\nuserSpec.settings.readonly: Not a valid integer.                                                                                             |
      | insertQuorumTimeout | 30        | 422    | 3          | The request is invalid.\nuserSpec.settings.insertQuorumTimeout: Value must be a multiple of 1000 (1 second)                                                           |
      | readOverflowMode    | "any"     | 422    | 3          | The request is invalid.\nuserSpec.settings.readOverflowMode: Invalid value 'any', allowed values: OVERFLOW_MODE_BREAK, OVERFLOW_MODE_THROW, OVERFLOW_MODE_UNSPECIFIED |

  @users @create
  Scenario Outline: Adding user with invalid settings via gRPC fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password",
            "settings": {
                "<name>": <value>
            }
        }
    }
    """
    Then we get gRPC response error with code <error code> and message "<error message>"
    Examples:
      | name                | value | error code       | error message                                                         |
      | insertQuorumTimeout | "30"  | INVALID_ARGUMENT | invalid insert_quorum_timeout value: must be at least 1000 (1 second) |

  @users @update
  Scenario: Changing user settings works
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test" with data
    """
    {
        "settings": {
            "maxMemoryUsage": 21474836480,
            "maxMemoryUsageForUser": 53687091200
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test",
        "settings": {
            "max_memory_usage": 21474836480,
            "max_memory_usage_for_user": 53687091200
        },
        "update_mask": {
            "paths": [
                "settings.max_memory_usage",
                "settings.max_memory_usage_for_user"
            ]
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Update user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test"
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/users/test"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "settings": {
            "maxMemoryUsage": 21474836480,
            "maxMemoryUsageForUser": 53687091200
        }
    }
    """

  @users @update
  Scenario: Set user settings to default works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "settings": {
                "readonly": 2,
                "maxRowsToRead": 100000000,
                "readOverflowMode": "OVERFLOW_MODE_THROW"
            }
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password",
            "settings": {
                "readonly": 2,
                "max_rows_to_read": 100000000,
                "read_overflow_mode": "OVERFLOW_MODE_THROW"
            }
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "settings": {
            "readonly": 2,
            "maxRowsToRead": 100000000,
            "readOverflowMode": "OVERFLOW_MODE_THROW"
        }
    }
    """
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2" with data
    """
    {
        "settings": {
            "maxRowsToRead": null,
            "readOverflowMode": "OVERFLOW_MODE_UNSPECIFIED"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2",
        "settings": {
            "max_rows_to_read": null,
            "read_overflow_mode": "OVERFLOW_MODE_UNSPECIFIED"
        },
        "update_mask": {
            "paths": [
                "settings.max_rows_to_read",
                "settings.read_overflow_mode"
            ]
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "settings": {
            "readonly": 2
        }
    }
    """
