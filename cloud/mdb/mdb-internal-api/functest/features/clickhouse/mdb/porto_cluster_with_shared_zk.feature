Feature: Manage Porto ClickHouse Cluster with Shared ZooKeeper

  Background:
    Given default headers
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
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
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 200
    """
    {
        "createdBy": "user",
        "description": "Create ClickHouse cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,clickhouse,keeper_hosts}', '{"zk1": 1,"zk2": 2,"zk3": 3}')
        WHERE subcid = 'subcid1'
    """
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "man"
        }]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker

  @events
  Scenario: Shard creation works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add shard to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.AddClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard2"
        }
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2"
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.AddClusterShard" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "shard_name": "shard2"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id3"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.clickhouse.v1.Shard",
        "clusterId": "cid1",
        "name": "shard2"
    }
    """
