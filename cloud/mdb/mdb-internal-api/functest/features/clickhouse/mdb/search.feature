@search
Feature: ClickHouse search API

  Scenario: Verify search renders
    Given default headers
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "testClickHouseCluster",
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
            "name": "clickDB"
        }],
        "userSpecs": [{
            "name": "clickUser",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "iva"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "testClickHouseCluster",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "clickDB"
        }],
        "user_specs": [{
            "name": "clickUser",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "iva"
        }, {
            "type": "CLICKHOUSE",
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }],
        "description": "test cluster"
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "timestamp": "<TIMESTAMP>",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testClickHouseCluster",
        "service": "managed-clickhouse",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testClickHouseCluster",
            "description": "test cluster",
            "labels": {},
            "users": ["clickUser"],
            "databases": ["clickDB"],
            "hosts": ["iva-1.db.yandex.net",
                      "man-1.db.yandex.net",
                      "man-2.db.yandex.net",
                      "sas-1.db.yandex.net",
                      "vla-1.db.yandex.net"]
        }
    }
    """
