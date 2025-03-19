@grpc_api
Feature: Upgrade from non-HA to HA config in DataCloud ClickHouse Cluster

  Background:
    Given headers
    """
    {
        "X-YaCloud-SubjectToken": "rw-token",
        "Access-Id": "00000000-0000-0000-0000-000000000000",
        "Access-Secret": "dummy"
    }
    """
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster",
        "description": "test cluster",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "replica_count": 1
            }
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "cluster_id": "cid1",
        "operation_id": "worker_task_id1"
    }
    """
    When "worker_task_id1" acquired and finished by worker

  Scenario: Create non-HA ClickHouse cluster
    When we "ListHosts" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we save DataCloud response body at path "$.hosts[0].name" as "cluster_host"
    When we GET via PillarConfig "/v1/config/{{.cluster_host}}"
    Then body at path "$.data.clickhouse" contains
    """
    {
        "keeper_hosts": {
            "{{.cluster_host}}": 1
        },
        "embedded_keeper": true
    }
    """

  Scenario: Update cluster to HA-config
    When we "ListHosts" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we save DataCloud response body at path "$.hosts[0].name" as "cluster_init_host"
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "new_name",
        "description": "new cluster description",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.3",
                "disk_size": 68719476736,
                "replica_count": 3
            }
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "ListHosts" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we save DataCloud response body at path "$.hosts[0].name" as "cluster_first_host"
    Then we save DataCloud response body at path "$.hosts[1].name" as "cluster_second_host"
    Then we save DataCloud response body at path "$.hosts[2].name" as "cluster_third_host"
    When we GET via PillarConfig "/v1/config/{{.cluster_first_host}}"
    Then body at path "$.data.clickhouse" contains
    """
    {
        "keeper_hosts": {
            {{if eq .cluster_first_host .cluster_init_host}}
                "{{ .cluster_first_host }}": 1,
                "{{ .cluster_second_host }}": 2,
                "{{ .cluster_third_host }}": 3
            {{end}}
            {{if eq .cluster_second_host .cluster_init_host}}
                "{{ .cluster_first_host }}": 2,
                "{{ .cluster_second_host }}": 1,
                "{{ .cluster_third_host }}": 3
            {{end}}
            {{if eq .cluster_third_host .cluster_init_host}}
                "{{ .cluster_first_host }}": 2,
                "{{ .cluster_second_host }}": 3,
                "{{ .cluster_third_host }}": 1
            {{end}}
        },
        "embedded_keeper": true
    }
    """

  Scenario: Update early created cluster with zk_hosts to HA-config
    When we "ListHosts" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we save DataCloud response body at path "$.hosts[0].name" as "cluster_init_host"
    When we run query
    """
        UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,clickhouse,zk_hosts}', '["{{.cluster_init_host}}"]')
        WHERE subcid = 'subcid1'
    """
    When we run query
    """
        UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,clickhouse}', (value->'data'->'clickhouse') - 'keeper_hosts')
        WHERE subcid = 'subcid1'
    """
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "new_name",
        "description": "new cluster description",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.3",
                "disk_size": 68719476736,
                "replica_count": 3
            }
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "ListHosts" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we save DataCloud response body at path "$.hosts[0].name" as "cluster_first_host"
    Then we save DataCloud response body at path "$.hosts[1].name" as "cluster_second_host"
    Then we save DataCloud response body at path "$.hosts[2].name" as "cluster_third_host"
    When we GET via PillarConfig "/v1/config/{{.cluster_init_host}}"
    Then body at path "$.data.clickhouse" contains
    """
    {
        "zk_hosts": [
            {{ $expectedZkHosts := mkStringSlice .cluster_first_host .cluster_second_host .cluster_third_host | sortStrings }}
            {{ range $index, $host := $expectedZkHosts }}
                {{if $index}},{{end}}"{{$host}}"
            {{end}}
        ],
        "embedded_keeper": true
    }
    """