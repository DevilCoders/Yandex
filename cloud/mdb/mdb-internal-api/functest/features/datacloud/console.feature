@grpc_api
Feature: Test Datacloud console api

  Background:
    Given default headers
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "clickhouse_aws_cluster",
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
        "name": "clickhouse_yandex_cluster",
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
    When "worker_task_id2" acquired and finished by worker
    When we run query
    """
    UPDATE dbaas.clouds
    SET clusters_quota = 4
    """
    When we "Create" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "kafka_aws_cluster",
        "description": "test cluster",
        "version": "2.8",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "broker_count": 1,
                "zone_count": 3
            }
        }
    }
    """
    Then we get DataCloud response OK
    When "worker_task_id3" acquired and finished by worker
    When we "Create" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "yandex",
        "region_id": "ru-central-1",
        "name": "kafka_yandex_cluster",
        "description": "test cluster",
        "version": "2.8",
        "resources": {
            "kafka": {
                "resource_preset_id": "s1.porto.1",
                "disk_size": 34359738368,
                "broker_count": 1,
                "zone_count": 1
            }
        }
    }
    """
    Then we get DataCloud response OK
    When "worker_task_id4" acquired and finished by worker
    Then we run query
    """
    UPDATE dbaas.clusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """

  Scenario: ListClusters works
    When we "List" via DataCloud at "datacloud.console.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "paging": {
            "page_size": 100
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "clusters": [{
            "create_time": "2000-01-01T00:00:00Z",
            "description": "test cluster",
            "id": "cid1",
            "name": "clickhouse_aws_cluster",
            "project_id": "folder1",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "type": "CLUSTER_TYPE_CLICKHOUSE",
            "cloud_type": "aws",
            "region_id": "eu-central1",
            "version": "22.1",
            "resources": [{
                "host_type": "HOST_TYPE_CLICKHOUSE",
                "host_count": "3",
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368"
            }]
        }, {
            "create_time": "2000-01-01T00:00:00Z",
            "description": "test cluster",
            "id": "cid2",
            "name": "clickhouse_yandex_cluster",
            "project_id": "folder1",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "type": "CLUSTER_TYPE_CLICKHOUSE",
            "cloud_type": "yandex",
            "region_id": "ru-central-1",
            "version": "21.11",
            "resources": [{
                "host_type": "HOST_TYPE_CLICKHOUSE",
                "host_count": "1",
                "resource_preset_id": "s1.porto.1",
                "disk_size": "34359738368"
            }]
        }, {
            "create_time": "2000-01-01T00:00:00Z",
            "description": "test cluster",
            "id": "cid3",
            "name": "kafka_aws_cluster",
            "project_id": "folder1",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "type": "CLUSTER_TYPE_KAFKA",
            "cloud_type": "aws",
            "region_id": "eu-central1",
            "version": "2.8",
            "resources": [{
                "host_type": "HOST_TYPE_KAFKA",
                "host_count": "3",
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368"
            }]
        }, {
            "create_time": "2000-01-01T00:00:00Z",
            "description": "test cluster",
            "id": "cid4",
            "name": "kafka_yandex_cluster",
            "project_id": "folder1",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "type": "CLUSTER_TYPE_KAFKA",
            "cloud_type": "yandex",
            "region_id": "ru-central-1",
            "version": "2.8",
            "resources": [{
                "host_type": "HOST_TYPE_KAFKA",
                "host_count": "1",
                "resource_preset_id": "s1.porto.1",
                "disk_size": "34359738368"
            }]
        }],
        "next_page": {
            "token": ""
        }
    }
    """

  Scenario: ListClusters with pagination works
    When we "List" via DataCloud at "datacloud.console.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "paging": {
            "page_size": 2
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "clusters": [{
            "create_time": "2000-01-01T00:00:00Z",
            "description": "test cluster",
            "id": "cid1",
            "name": "clickhouse_aws_cluster",
            "project_id": "folder1",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "type": "CLUSTER_TYPE_CLICKHOUSE",
            "cloud_type": "aws",
            "region_id": "eu-central1",
            "version": "22.1",
            "resources": [{
                "host_type": "HOST_TYPE_CLICKHOUSE",
                "host_count": "3",
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368"
            }]
        }, {
            "create_time": "2000-01-01T00:00:00Z",
            "description": "test cluster",
            "id": "cid2",
            "name": "clickhouse_yandex_cluster",
            "project_id": "folder1",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "type": "CLUSTER_TYPE_CLICKHOUSE",
            "cloud_type": "yandex",
            "region_id": "ru-central-1",
            "version": "21.11",
            "resources": [{
                "host_type": "HOST_TYPE_CLICKHOUSE",
                "host_count": "1",
                "resource_preset_id": "s1.porto.1",
                "disk_size": "34359738368"
            }]
        }],
        "next_page": {
            "token": "eyJMYXN0Q2x1c3Rlck5hbWUiOiJjbGlja2hvdXNlX3lhbmRleF9jbHVzdGVyIn0="
        }
    }
    """
    When we "List" via DataCloud at "datacloud.console.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "paging": {
            "page_size": 2,
            "page_token": "eyJMYXN0Q2x1c3Rlck5hbWUiOiJjbGlja2hvdXNlX3lhbmRleF9jbHVzdGVyIn0="
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "clusters": [{
            "create_time": "2000-01-01T00:00:00Z",
            "description": "test cluster",
            "id": "cid3",
            "name": "kafka_aws_cluster",
            "project_id": "folder1",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "type": "CLUSTER_TYPE_KAFKA",
            "cloud_type": "aws",
            "region_id": "eu-central1",
            "version": "2.8",
            "resources": [{
                "host_type": "HOST_TYPE_KAFKA",
                "host_count": "3",
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368"
            }]
        }, {
            "create_time": "2000-01-01T00:00:00Z",
            "description": "test cluster",
            "id": "cid4",
            "name": "kafka_yandex_cluster",
            "project_id": "folder1",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "type": "CLUSTER_TYPE_KAFKA",
            "cloud_type": "yandex",
            "region_id": "ru-central-1",
            "version": "2.8",
            "resources": [{
                "host_type": "HOST_TYPE_KAFKA",
                "host_count": "1",
                "resource_preset_id": "s1.porto.1",
                "disk_size": "34359738368"
            }]
        }],
        "next_page": {
            "token": "eyJMYXN0Q2x1c3Rlck5hbWUiOiJrYWZrYV95YW5kZXhfY2x1c3RlciJ9"
        }
    }
    """
    When we "List" via DataCloud at "datacloud.console.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "paging": {
            "page_size": 2,
            "page_token": "eyJMYXN0Q2x1c3Rlck5hbWUiOiJrYWZrYV95YW5kZXhfY2x1c3RlciJ9"
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "clusters": []
    }
    """

  Scenario: ProjectByClusterID works
    When we "ProjectByClusterID" via DataCloud at "datacloud.console.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "project_id": "folder1"
    }
    """
