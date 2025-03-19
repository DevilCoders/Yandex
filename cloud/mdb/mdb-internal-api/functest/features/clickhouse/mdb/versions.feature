Feature: Management of ClickHouse versions

    Background:
        Given default headers

    Scenario: List versions works
        When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.VersionsService"
        Then we get gRPC response with body
        # language=json
        """
        {
            "version": [
                {"id": "22.5",  "name": "22.5",     "updatable_to": [        "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "22.3",  "name": "22.3 LTS", "updatable_to": ["22.5",         "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "22.1",  "name": "22.1",     "updatable_to": ["22.5", "22.3",         "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.11", "name": "21.11",    "updatable_to": ["22.5", "22.3", "22.1",          "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.10", "name": "21.10",    "updatable_to": ["22.5", "22.3", "22.1", "21.11",          "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.9",  "name": "21.9",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10",         "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.8",  "name": "21.8 LTS", "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9",         "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.7",  "name": "21.7",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8",         "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.4",  "name": "21.4",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7",         "21.3", "21.2"], "deprecated": false},
                {"id": "21.3",  "name": "21.3 LTS", "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4",         "21.2"], "deprecated": false},
                {"id": "21.2",  "name": "21.2",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3"        ], "deprecated": false},
                {"id": "21.1",  "name": "21.1",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": true}
            ],
            "next_page_token": ""
        }
        """

    Scenario: List versions with page size works
        When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.VersionsService" with data
        # language=json
        """
        {
            "page_size": 2
        }
        """
        Then we get gRPC response with body
        # language=json
        """
        {
            "version": [
                {"id": "22.5",  "name": "22.5",     "updatable_to": [        "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "22.3",  "name": "22.3 LTS", "updatable_to": ["22.5",         "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false}
            ],
            "next_page_token": "eyJPZmZzZXQiOjJ9"
        }
        """

    Scenario: List versions with page token works
        When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.VersionsService" with data
        # language=json
        """
        {
            "page_token": "eyJPZmZzZXQiOjJ9"
        }
        """
        Then we get gRPC response with body
        # language=json
        """
        {
            "version": [
                {"id": "22.1",  "name": "22.1",     "updatable_to": ["22.5", "22.3",         "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.11", "name": "21.11",    "updatable_to": ["22.5", "22.3", "22.1",          "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.10", "name": "21.10",    "updatable_to": ["22.5", "22.3", "22.1", "21.11",          "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.9",  "name": "21.9",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10",         "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.8",  "name": "21.8 LTS", "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9",         "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.7",  "name": "21.7",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8",         "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.4",  "name": "21.4",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7",         "21.3", "21.2"], "deprecated": false},
                {"id": "21.3",  "name": "21.3 LTS", "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4",         "21.2"], "deprecated": false},
                {"id": "21.2",  "name": "21.2",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3"        ], "deprecated": false},
                {"id": "21.1",  "name": "21.1",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": true}
            ],
            "next_page_token": ""
        }
        """

    Scenario: Create cluster with testing version and without MDB_CLICKHOUSE_TESTING_VERSIONS feature fails
        When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
        # language=json
        """
        {
            "name": "test",
            "environment": "PRESTABLE",
            "configSpec": {
                "version": "21.3.1.6142",
                "clickhouse": {
                    "resources": {
                        "resourcePresetId": "s2.porto.1",
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
                "zoneId": "sas"
            }],
            "description": "test cluster",
            "networkId": "IN-PORTO-NO-NETWORK-API"
        }
        """
        Then we get response with status 422 and body contains
        # language=json
        """
        {
            "code": 3,
            "message": "Version '21.3.1.6142' is not available, allowed versions are: 21.1, 21.2, 21.3, 21.4, 21.7, 21.8, 21.11, 22.1, 22.3, 22.5"
        }
        """

    Scenario: Create cluster with testing version and MDB_CLICKHOUSE_TESTING_VERSIONS feature succeeds
        Given feature flags
        # language=json
        """
        ["MDB_CLICKHOUSE_TESTING_VERSIONS"]
        """
        When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
        # language=json
        """
        {
            "name": "test",
            "environment": "PRESTABLE",
            "configSpec": {
                "version": "21.3.1.6142",
                "clickhouse": {
                    "resources": {
                        "resourcePresetId": "s2.porto.1",
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
                "zoneId": "sas"
            }],
            "description": "test cluster",
            "networkId": "IN-PORTO-NO-NETWORK-API"
        }
        """
        Then we get response with status 200
        When we GET "/api/v1.0/config/sas-1.db.yandex.net"
        Then we get response with status 200
        And body at path "$.data.clickhouse" contains
        # language=json
        """
        {
            "ch_version": "21.3.1.6142"
        }
        """
        And body at path "$.data" contains
        # language=json
        """
        {
            "testing_repos": true
        }
        """

    Scenario: Create cluster with stable version and MDB_CLICKHOUSE_TESTING_VERSIONS feature succeeds
        Given feature flags
        # language=json
        """
        ["MDB_CLICKHOUSE_TESTING_VERSIONS"]
        """
        When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
        # language=json
        """
        {
            "name": "test",
            "environment": "PRESTABLE",
            "configSpec": {
                "version": "21.1",
                "clickhouse": {
                    "resources": {
                        "resourcePresetId": "s2.porto.1",
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
                "zoneId": "sas"
            }],
            "description": "test cluster",
            "networkId": "IN-PORTO-NO-NETWORK-API"
        }
        """
        Then we get response with status 200
        When we GET "/api/v1.0/config/sas-1.db.yandex.net"
        Then we get response with status 200
        And body at path "$.data" contains
        # language=json
        """
        {
            "testing_repos": true
        }
        """
