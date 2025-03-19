@models
Feature: Management of ClickHouse ML models

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

  Scenario: Adding ML model works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add ML model in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateMlModelMetadata",
          "clusterId": "cid1",
          "mlModelName": "test_model"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add ML model in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateMlModelMetadata",
            "cluster_id": "cid1",
            "ml_model_name": "test_model"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.CreateMlModel" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "ml_model_name": "test_model"
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
        "description": "Add ML model in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateMlModelMetadata",
            "cluster_id": "cid1",
            "ml_model_name": "test_model"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.MlModel",
            "cluster_id": "cid1",
            "name": "test_model",
            "type": "ML_MODEL_TYPE_CATBOOST",
            "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels/test_model"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse" contains
    """
    {
        "models": {
            "test_model": {
                "type": "catboost",
                "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
            }
        }
    }
    """

  Scenario: Adding ML model operation has correct response
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add ML model in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateMlModelMetadata",
          "clusterId": "cid1",
          "mlModelName": "test_model"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add ML model in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateMlModelMetadata",
            "cluster_id": "cid1",
            "ml_model_name": "test_model"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we "GET" via REST at "/mdb/1.0/operations/worker_task_id2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add ML model in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateMlModelMetadata",
          "clusterId": "cid1",
          "mlModelName": "test_model"
        },
        "response": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.MlModel",
            "clusterId": "cid1",
            "name": "test_model",
            "type": "ML_MODEL_TYPE_CATBOOST",
            "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
        }
    }
    """

  Scenario: Adding ML model with invalid name fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "Invalid name!",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nmlModelName: ML model name 'Invalid name!' does not conform to naming rules"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "Invalid name!",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "ML model name "Invalid name!" has invalid symbols"

  Scenario: Adding ML model with invalid URI fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://example.com/test_model.bin"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nuri: URI 'https://example.com/test_model.bin' is invalid. "
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://example.com/test_model.bin"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "uri: "https://example.com/test_model.bin" is invalid. "

  Scenario: Adding multiple ML models works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model2",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model2.bin"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model2",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model2.bin"
    }
    """
    Then we get gRPC response OK
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels"
    Then we get response with status 200 and body contains
    """
    {
        "mlModels": [
            {
                "clusterId": "cid1",
                "name": "test_model",
                "type": "ML_MODEL_TYPE_CATBOOST",
                "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
            },
            {
                "clusterId": "cid1",
                "name": "test_model2",
                "type": "ML_MODEL_TYPE_CATBOOST",
                "uri": "https://bucket1.storage.yandexcloud.net/test_model2.bin"
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "ml_models": [
            {
                "cluster_id": "cid1",
                "name": "test_model",
                "type": "ML_MODEL_TYPE_CATBOOST",
                "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
            },
            {
                "cluster_id": "cid1",
                "name": "test_model2",
                "type": "ML_MODEL_TYPE_CATBOOST",
                "uri": "https://bucket1.storage.yandexcloud.net/test_model2.bin"
            }
        ]
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse" contains
    """
    {
        "models": {
            "test_model": {
                "type": "catboost",
                "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
            },
            "test_model2": {
                "type": "catboost",
                "uri": "https://bucket1.storage.yandexcloud.net/test_model2.bin"
            }
        }
    }
    """

  Scenario: Adding duplicate ML model fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model2.bin"
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "ML model 'test_model' already exists"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response error with code ALREADY_EXISTS and message "ML model "test_model" already exists"

  Scenario: Updating ML model works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels/test_model" with data
    """
    {
        "uri": "https://bucket1.storage.yandexcloud.net/new_model.bin"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ML model in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateMlModelMetadata",
          "clusterId": "cid1",
          "mlModelName": "test_model"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "uri": "https://bucket1.storage.yandexcloud.net/new_model.bin"
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.UpdateMlModel" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "ml_model_name": "test_model"
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
        "description": "Modify ML model in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateMlModelRequest",
            "cluster_id": "cid1",
            "ml_model_name": "test_model"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.MlModel",
            "cluster_id": "cid1",
            "name": "test_model",
            "type": "ML_MODEL_TYPE_CATBOOST",
            "uri": "https://bucket1.storage.yandexcloud.net/new_model.bin"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels/test_model"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/new_model.bin"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/new_model.bin"
    }
    """

  Scenario: Updating nonexistent ML model fails
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels/test_model" with data
    """
    {
        "uri": "https://bucket1.storage.yandexcloud.net/new_model.bin"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "ML model 'test_model' does not exist"
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "uri": "https://bucket1.storage.yandexcloud.net/new_model.bin"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "ML model "test_model" not found"

  Scenario: Deleting ML model works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels/test_model"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete ML model in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.DeleteMlModelMetadata",
          "clusterId": "cid1",
          "mlModelName": "test_model"
        }
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete ML model in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.DeleteMlModelMetadata",
            "cluster_id": "cid1",
            "ml_model_name": "test_model"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.DeleteMlModel" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "ml_model_name": "test_model"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels"
    Then we get response with status 200 and body contains
    """
    {
        "mlModels": []
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "ml_models": []
    }
    """

  Scenario: Deleting nonexistent ML model fails
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels/test_model"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "ML model 'test_model' does not exist"
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "ML model "test_model" not found"

  @errors
  Scenario: Adding ML model to nonexistent cluster
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid42",
        "ml_model_name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "cluster id "cid42" not found"

  @grpc_api
  Scenario: Pagination works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model2",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id3" acquired and finished by worker
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model3",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id4" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 2
    }
    """
    Then we get gRPC response with body
    """
    {
        "ml_models": [
            {
                "cluster_id": "cid1",
                "name": "test_model",
                "type": "ML_MODEL_TYPE_CATBOOST",
                "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
            },
            {
                "cluster_id": "cid1",
                "name": "test_model2",
                "type": "ML_MODEL_TYPE_CATBOOST",
                "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
            }
        ],
        "next_page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    Then we get gRPC response with body
    """
    {
        "ml_models": [
            {
                "cluster_id": "cid1",
                "name": "test_model3",
                "type": "ML_MODEL_TYPE_CATBOOST",
                "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
            }
        ],
        "next_page_token": ""
    }
    """
