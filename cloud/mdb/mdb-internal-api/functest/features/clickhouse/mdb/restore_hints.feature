@restore-hints
Feature: Restore ClickHouse cluster from backup with hints

  Background:
    Given default headers
    And s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """

  Scenario: Restore hints for HA ClickHouse
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
        }]
    }
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we "GET" via REST at "/mdb/clickhouse/1.0/backups?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:01+00:00",
                "folderId": "folder1",
                "id": "cid1:0",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:00+00:00"
            }
        ]
    }
    """
    And we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "1970-01-01T00:00:01Z",
                "folder_id": "folder1",
                "id": "cid1:0",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:00Z"
            }
        ]
    }
    """
    When we GET "/mdb/clickhouse/1.0/console/restore-hints/cid1:0"
    Then we get response with status 200 and body equals
    """
    {
        "environment": "PRESTABLE",
        "networkId": "",
        "resources": {
            "diskSize": 10737418240,
            "minHostsCount": 1,
            "resourcePresetId": "s1.porto.1"
        }
    }
    """

  Scenario: Restore hints for non HA ClickHouse
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
        }]
    }
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we "GET" via REST at "/mdb/clickhouse/1.0/backups?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:01+00:00",
                "folderId": "folder1",
                "id": "cid1:0",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:00+00:00"
            }
        ]
    }
    """
    And we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "1970-01-01T00:00:01Z",
                "folder_id": "folder1",
                "id": "cid1:0",
                "source_cluster_id": "cid1",
                "source_shard_names": ["shard1"],
                "started_at": "1970-01-01T00:00:00Z"
            }
        ]
    }
    """
    When we GET "/mdb/clickhouse/1.0/console/restore-hints/cid1:0"
    Then we get response with status 200 and body equals
    """
    {
        "environment": "PRESTABLE",
        "networkId": "",
        "resources": {
            "diskSize": 10737418240,
            "minHostsCount": 1,
            "resourcePresetId": "s1.porto.1"
        }
    }
    """
