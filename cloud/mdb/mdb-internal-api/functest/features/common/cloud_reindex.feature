@search
Feature: Cloud reindex

  Background: Create redis, deleted it, create postgres
    Given feature flags
    """
    ["MDB_REDIS_62"]
    """
    And default headers
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "testRedis",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_2": {
                "password": "p@ssw#$rd!?"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    When "worker_task_id1" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "testPostgre",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test_user"
       }],
       "userSpecs": [{
           "name": "test_user",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "sas"
       }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """
    Given worker task "worker_task_id2" created at "2019-07-16T10:00:30+00:00"
    When we DELETE "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id3"
    }
    """
    Given worker task "worker_task_id3" created at "2000-01-01T10:00:30+00:00"

  Scenario: Cloud reindex
    When we POST "/mdb/1.0/search/cloud/cloud1:reindex" with data
    """
    {
        "reindex_timestamp": "2021-01-01T10:00:30+00:00"
    }
    """
    Then we get response with status 200 and body equals
    """
    {
        "docs": [
            {
                "attributes": {
                    "databases": [
                        "testdb"
                    ],
                    "description": "",
                    "hosts": [
                        "sas-1.db.yandex.net"
                    ],
                    "labels": {},
                    "name": "testPostgre",
                    "users": [
                        "test_user"
                    ]
                },
                "cloud_id": "cloud1",
                "folder_id": "folder1",
                "permission": "mdb.all.read",
                "resource_id": "cid2",
                "resource_path": [{
                    "resource_type": "resource-manager.folder",
                    "resource_id": "folder1"
                }],
                "name": "testPostgre",
                "resource_type": "cluster",
                "service": "managed-postgresql",
                "timestamp": "2019-07-16T10:00:30+00:00",
                "reindex_timestamp": "2021-01-01T10:00:30+00:00"
            }
        ]
    }
    """

  Scenario: Cloud reindex with deleted clusters
    When we POST "/mdb/1.0/search/cloud/cloud1:reindex" with data
    """
    {
        "reindex_timestamp": "2021-01-01T10:00:30+00",
        "include_deleted": true
    }
    """
    Then we get response with status 200 and body equals
    """
    {
        "docs": [
            {
                "attributes": {
                    "databases": [
                        "testdb"
                    ],
                    "description": "",
                    "hosts": [
                        "sas-1.db.yandex.net"
                    ],
                    "labels": {},
                    "name": "testPostgre",
                    "users": [
                        "test_user"
                    ]
                },
                "cloud_id": "cloud1",
                "folder_id": "folder1",
                "permission": "mdb.all.read",
                "resource_id": "cid2",
                "name": "testPostgre",
                "resource_type": "cluster",
                "resource_path": [{
                    "resource_type": "resource-manager.folder",
                    "resource_id": "folder1"
                }],
                "service": "managed-postgresql",
                "timestamp": "2019-07-16T10:00:30+00:00",
                "reindex_timestamp": "2021-01-01T10:00:30+00:00"
            },
            {
                "cloud_id": "cloud1",
                "deleted": "2000-01-01T10:00:30+00:00",
                "folder_id": "folder1",
                "permission": "mdb.all.read",
                "resource_id": "cid1",
                "name": "testRedis",
                "resource_type": "cluster",
                "resource_path": [{
                    "resource_type": "resource-manager.folder",
                    "resource_id": "folder1"
                }],
                "service": "managed-redis",
                "timestamp": "2000-01-01T10:00:30+00:00",
                "reindex_timestamp": "2021-01-01T10:00:30+00:00"
            }
        ]
    }
    """

  @MDB-6115
  Scenario: Reindex cloud with purged cluster
    # delete redis task complete
    And "worker_task_id2" acquired and finished by worker
    # delete_metadata redis task complete
    And "worker_task_id3" acquired and finished by worker
    # purge redis task complete
    And "worker_task_id4" acquired and finished by worker
    When we POST "/mdb/1.0/search/cloud/cloud1:reindex" with data
    """
    {
        "include_deleted": true
    }
    """
    Then we get response with status 200
    And body at path "$.docs[*].deleted" contains only
    """
    ["2000-01-01T10:00:30+00:00", ""]
    """
    And body at path "$.docs[*].service" contains only
    """
    ["managed-redis", "managed-postgresql"]
    """

  @grpc_api @MDB-8479
  Scenario: Reindex cloud with GRPC-only cluster
    When we add default feature flag "MDB_KAFKA_CLUSTER"
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "description": "Create Apache Kafka cluster"
    }
    """
    When we POST "/mdb/1.0/search/cloud/cloud1:reindex" with data
    """
    {}
    """
    Then we get response with status 200
    And body at path "$.docs[*].service" contains only
    """
    ["managed-redis", "managed-postgresql"]
    """

  @MDB-13432
  Scenario: Reindex ignore maintenance tasks when calculate documents timestamps
    . Verify that created and finished maintenance task
    . doesn't change search document timestamp.
    When we POST "/mdb/1.0/search/cloud/cloud1:reindex"
    Then we get response with status 200
    And body at path "$.docs[*].timestamp" contains only
    """
    ["2019-07-16T10:00:30+00:00"]
    """
    When we run query
    """
    WITH locked_cluster AS (
        SELECT cid, rev AS actual_rev, next_rev, folder_id FROM code.lock_future_cluster('cid2')
    ), planned_task AS (
        SELECT code.plan_maintenance_task(
            i_max_delay      => '2000-01-22T00:00:00+00:00'::timestamp with time zone,
            i_task_id        => 'worker_task_maintenance_id1',
            i_cid            =>  cid,
            i_config_id      => 'postgresql_minor_upgrade',
            i_folder_id      => folder_id,
            i_operation_type => 'postgresql_cluster_modify',
            i_task_type      => 'postgresql_cluster_maintenance',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'maintenance',
            i_rev            => actual_rev,
            i_target_rev     => next_rev,
            i_plan_ts        => '2021-01-08 00:00:00+00',
            i_create_ts      => '2021-01-01 00:00:00+00',
            i_info           => 'Upgrade Postgresql'
        ), cid, actual_rev, next_rev
        FROM locked_cluster
    )
    SELECT code.complete_future_cluster_change(cid, actual_rev, next_rev)
    FROM planned_task
    """
    And "worker_task_maintenance_id1" acquired and finished by worker
    And we POST "/mdb/1.0/search/cloud/cloud1:reindex"
    Then we get response with status 200
    And body at path "$.docs[*].timestamp" contains only
    """
    ["2019-07-16T10:00:30+00:00"]
    """
