@grpc_api
Feature: Test Datacloud OperationService API

  Background:
    Given default headers
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "aws_cluster",
        "description": "test cluster",
        "version": "22.1",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "replica_count": 3
            }
        }
    }
    """
    Then we get DataCloud response OK
    When "worker_task_id1" acquired and finished by worker
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "yandex",
        "region_id": "ru-central-1",
        "name": "yandex_cluster",
        "description": "test cluster",
        "version": "21.11",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "s1.porto.1",
                "disk_size": 34359738368,
                "replica_count": 1
            }
        }
    }
    """
    Then we get DataCloud response OK
    Then we run query
    """
    UPDATE dbaas.clusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """

  Scenario: Get Operation works
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id1"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "worker_task_id1",
        "project_id": "folder1",
        "description": "Create ClickHouse cluster",
        "created_by": "user",
        "create_time": "**IGNORE**",
        "start_time": "**IGNORE**",
        "finish_time": "**IGNORE**",
        "metadata": {
            "cluster_id": "cid1"
        },
        "status": "STATUS_DONE"
    }
    """

  Scenario: List Operation works
    When we "List" via DataCloud at "datacloud.clickhouse.v1.OperationService" with data
    """
    {
        "project_id": "folder1"
    }
    """
    Then we get DataCloud response with body ignoring empty
    """
    {
        "operations": [{
            "id": "worker_task_id2",
            "project_id": "folder1",
            "description": "Create ClickHouse cluster",
            "created_by": "user",
            "create_time": "**IGNORE**",
            "finish_time": "**IGNORE**",
            "metadata": {
                "cluster_id": "cid2"
            },
            "status": "STATUS_PENDING"
        }, {
            "id": "worker_task_id1",
            "project_id": "folder1",
            "description": "Create ClickHouse cluster",
            "created_by": "user",
            "create_time": "**IGNORE**",
            "start_time": "**IGNORE**",
            "finish_time": "**IGNORE**",
            "metadata": {
                "cluster_id": "cid1"
            },
            "status": "STATUS_DONE"
        }]
    }
    """
    When we "List" via DataCloud at "datacloud.clickhouse.v1.OperationService" with data
    """
    {
        "project_id": "folder1",
        "paging": {
            "page_size": 1
        }
    }
    """
    Then we get DataCloud response with body ignoring empty
    """
    {
        "operations": [{
            "id": "worker_task_id2",
            "project_id": "folder1",
            "description": "Create ClickHouse cluster",
            "created_by": "user",
            "create_time": "**IGNORE**",
            "finish_time": "**IGNORE**",
            "metadata": {
                "cluster_id": "cid2"
            },
            "status": "STATUS_PENDING"
        }],
        "next_page": {
            "token": "**IGNORE**"
        }
    }
    """
    When we "List" via DataCloud at "datacloud.clickhouse.v1.OperationService" with data and last page token
    """
    {
        "project_id": "folder1",
        "paging": {
            "page_size": 1,
            "page_token": "**IGNORE**"
        }
    }
    """
    Then we get DataCloud response with body ignoring empty
    """
    {
        "operations": [{
            "id": "worker_task_id1",
            "project_id": "folder1",
            "description": "Create ClickHouse cluster",
            "created_by": "user",
            "create_time": "**IGNORE**",
            "start_time": "**IGNORE**",
            "finish_time": "**IGNORE**",
            "metadata": {
                "cluster_id": "cid1"
            },
            "status": "STATUS_DONE"
        }]
    }
    """
