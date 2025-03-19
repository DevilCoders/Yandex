Feature: Management of ClickHouse format schemas

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

  Scenario: Adding format schema works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add format schema in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateFormatSchemaMetadata",
          "clusterId": "cid1",
          "formatSchemaName": "test_schema"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add format schema in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateFormatSchemaMetadata",
            "cluster_id": "cid1",
            "format_schema_name": "test_schema"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.CreateFormatSchema" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "format_schema_name": "test_schema"
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
        "description": "Add format schema in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateFormatSchemaMetadata",
            "cluster_id": "cid1",
            "format_schema_name": "test_schema"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.FormatSchema",
            "cluster_id": "cid1",
            "name": "test_schema",
            "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
            "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas/test_schema"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse" contains
    """
    {
        "format_schemas": {
            "test_schema": {
                "type": "protobuf",
                "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
            }
        }
    }
    """

  Scenario: Adding format schema operation has correct response
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add format schema in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateFormatSchemaMetadata",
          "clusterId": "cid1",
          "formatSchemaName": "test_schema"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add format schema in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateFormatSchemaMetadata",
            "cluster_id": "cid1",
            "format_schema_name": "test_schema"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we "GET" via REST at "/mdb/1.0/operations/worker_task_id2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add format schema in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateFormatSchemaMetadata",
          "clusterId": "cid1",
          "formatSchemaName": "test_schema"
        },
        "response": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.FormatSchema",
            "clusterId": "cid1",
            "name": "test_schema",
            "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
            "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
        }
    }
    """

  Scenario: Adding format schema with invalid name fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "Invalid name!",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nformatSchemaName: format schema name 'Invalid name!' does not conform to naming rules"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "Invalid name!",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "format schema name "Invalid name!" has invalid symbols"

  Scenario: Adding format schema with invalid URI fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://example.com/test_schema.proto"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nuri: URI 'https://example.com/test_schema.proto' is invalid. "
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://example.com/test_schema.proto"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "uri: "https://example.com/test_schema.proto" is invalid. "

  Scenario: Adding multiple format schemas works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema2",
        "type": "FORMAT_SCHEMA_TYPE_CAPNPROTO",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.capnp"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema2",
        "type": "FORMAT_SCHEMA_TYPE_CAPNPROTO",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.capnp"
    }
    """
    Then we get gRPC response OK
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas"
    Then we get response with status 200 and body contains
    """
    {
        "formatSchemas": [
            {
                "clusterId": "cid1",
                "name": "test_schema",
                "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
                "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
            },
            {
                "clusterId": "cid1",
                "name": "test_schema2",
                "type": "FORMAT_SCHEMA_TYPE_CAPNPROTO",
                "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.capnp"
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "format_schemas": [
            {
                "cluster_id": "cid1",
                "name": "test_schema",
                "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
                "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
            },
            {
                "cluster_id": "cid1",
                "name": "test_schema2",
                "type": "FORMAT_SCHEMA_TYPE_CAPNPROTO",
                "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.capnp"
            }
        ]
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse" contains
    """
    {
        "format_schemas": {
            "test_schema": {
                "type": "protobuf",
                "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
            },
            "test_schema2": {
                "type": "capnproto",
                "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.capnp"
            }
        }
    }
    """

  Scenario: List format schemas with pagination works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema2",
        "type": "FORMAT_SCHEMA_TYPE_CAPNPROTO",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.capnp"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema2",
        "type": "FORMAT_SCHEMA_TYPE_CAPNPROTO",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.capnp"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id3" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema3",
        "type": "FORMAT_SCHEMA_TYPE_CAPNPROTO",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.capnp"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema3",
        "type": "FORMAT_SCHEMA_TYPE_CAPNPROTO",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.capnp"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id4" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 2
    }
    """
    Then we get gRPC response with body
    """
    {
        "format_schemas": [
            {
                "cluster_id": "cid1",
                "name": "test_schema",
                "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
                "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
            },
            {
                "cluster_id": "cid1",
                "name": "test_schema2",
                "type": "FORMAT_SCHEMA_TYPE_CAPNPROTO",
                "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.capnp"
            }
        ],
        "next_page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    Then we get gRPC response with body
    """
    {
        "format_schemas": [
            {
                "cluster_id": "cid1",
                "name": "test_schema3",
                "type": "FORMAT_SCHEMA_TYPE_CAPNPROTO",
                "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.capnp"
            }
        ],
        "next_page_token": ""
    }
    """

  Scenario: Adding duplicate format schema fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema2.proto"
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Format schema 'test_schema' already exists"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get gRPC response error with code ALREADY_EXISTS and message "format schema "test_schema" already exists"

  Scenario: Updating format schema works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add format schema in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateFormatSchemaMetadata",
            "cluster_id": "cid1",
            "format_schema_name": "test_schema"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas/test_schema" with data
    """
    {
        "uri": "https://bucket1.storage.yandexcloud.net/new_schema.proto"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify format schema in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateFormatSchemaMetadata",
          "clusterId": "cid1",
          "formatSchemaName": "test_schema"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "uri": "https://bucket1.storage.yandexcloud.net/new_schema.proto"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Modify format schema in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateFormatSchemaMetadata",
            "cluster_id": "cid1",
            "format_schema_name": "test_schema"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.UpdateFormatSchema" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "format_schema_name": "test_schema"
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
        "description": "Modify format schema in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateFormatSchemaMetadata",
            "cluster_id": "cid1",
            "format_schema_name": "test_schema"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.FormatSchema",
            "cluster_id": "cid1",
            "name": "test_schema",
            "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
            "uri": "https://bucket1.storage.yandexcloud.net/new_schema.proto"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas/test_schema"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/new_schema.proto"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/new_schema.proto"
    }
    """

  Scenario: Updating nonexistent format schema fails
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas/test_schema" with data
    """
    {
        "uri": "https://bucket1.storage.yandexcloud.net/new_schema.proto"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Format schema 'test_schema' does not exist"
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "uri": "https://bucket1.storage.yandexcloud.net/new_schema.proto"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "format schema "test_schema" not found"

  Scenario: Deleting format schema works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas/test_schema"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete format schema in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.DeleteFormatSchemaMetadata",
          "clusterId": "cid1",
          "formatSchemaName": "test_schema"
        }
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete format schema in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.DeleteFormatSchemaMetadata",
            "cluster_id": "cid1",
            "format_schema_name": "test_schema"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.DeleteFormatSchema" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "format_schema_name": "test_schema"
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas"
    Then we get response with status 200 and body contains
    """
    {
        "formatSchemas": []
    }
    """

  Scenario: Deleting nonexistent format schema fails
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas/test_schema"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Format schema 'test_schema' does not exist"
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "format schema "test_schema" not found"

