@grpc_api
Feature: Test ClickHouse console api

    Background:
        Given default headers

    Scenario: Get ClickHouse clusters config
        When we "GetClustersConfig" via DataCloud at "datacloud.clickhouse.console.v1.ClusterService" with data
        # language=json
        """
        {
            "project_id": "folder1",
            "cloud_type": "aws",
            "region_id": "eu-central1"
        }
        """
        Then we get DataCloud response with body
        # language=json
        """
        {
            "name_validator": {
                "max": "63",
                "min": "1",
                "regexp": "^[a-zA-Z0-9_-]+$",
                "blocklist": []
            },
            "name_hint": "clickhouse1",
            "versions": [
                {
                    "deprecated": true,
                    "id": "21.1",
                    "name": "21.1",
                    "updatable_to": [
                        "21.2",
                        "21.3",
                        "21.4",
                        "21.7",
                        "21.8",
                        "21.9",
                        "21.10",
                        "21.11",
                        "22.1",
                        "22.3",
                        "22.5"
                    ]
                },
                {
                    "deprecated": false,
                    "id": "21.2",
                    "name": "21.2",
                    "updatable_to": [
                        "21.3",
                        "21.4",
                        "21.7",
                        "21.8",
                        "21.9",
                        "21.10",
                        "21.11",
                        "22.1",
                        "22.3",
                        "22.5"
                    ]
                },
                {
                    "deprecated": false,
                    "id": "21.3",
                    "name": "21.3 LTS",
                    "updatable_to": [
                        "21.2",
                        "21.4",
                        "21.7",
                        "21.8",
                        "21.9",
                        "21.10",
                        "21.11",
                        "22.1",
                        "22.3",
                        "22.5"
                    ]
                },
                {
                    "deprecated": false,
                    "id": "21.4",
                    "name": "21.4",
                    "updatable_to": [
                        "21.2",
                        "21.3",
                        "21.7",
                        "21.8",
                        "21.9",
                        "21.10",
                        "21.11",
                        "22.1",
                        "22.3",
                        "22.5"
                    ]
                },
                {
                    "deprecated": false,
                    "id": "21.7",
                    "name": "21.7",
                    "updatable_to": [
                        "21.2",
                        "21.3",
                        "21.4",
                        "21.8",
                        "21.9",
                        "21.10",
                        "21.11",
                        "22.1",
                        "22.3",
                        "22.5"
                    ]
                },
                {
                    "deprecated": false,
                    "id": "21.8",
                    "name": "21.8 LTS",
                    "updatable_to": [
                        "21.2",
                        "21.3",
                        "21.4",
                        "21.7",
                        "21.9",
                        "21.10",
                        "21.11",
                        "22.1",
                        "22.3",
                        "22.5"
                    ]
                },
                {
                    "deprecated": false,
                    "id": "21.9",
                    "name": "21.9",
                    "updatable_to": [
                        "21.2",
                        "21.3",
                        "21.4",
                        "21.7",
                        "21.8",
                        "21.10",
                        "21.11",
                        "22.1",
                        "22.3",
                        "22.5"
                    ]
                },
                {
                    "deprecated": false,
                    "id": "21.10",
                    "name": "21.10",
                    "updatable_to": [
                        "21.2",
                        "21.3",
                        "21.4",
                        "21.7",
                        "21.8",
                        "21.9",
                        "21.11",
                        "22.1",
                        "22.3",
                        "22.5"
                    ]
                },
                {
                    "deprecated": false,
                    "id": "21.11",
                    "name": "21.11",
                    "updatable_to": [
                        "21.2",
                        "21.3",
                        "21.4",
                        "21.7",
                        "21.8",
                        "21.9",
                        "21.10",
                        "22.1",
                        "22.3",
                        "22.5"
                    ]
                },
                {
                    "deprecated": false,
                    "id": "22.1",
                    "name": "22.1",
                    "updatable_to": [
                        "21.2",
                        "21.3",
                        "21.4",
                        "21.7",
                        "21.8",
                        "21.9",
                        "21.10",
                        "21.11",
                        "22.3",
                        "22.5"
                    ]
                },
                {
                    "deprecated": false,
                    "id": "22.3",
                    "name": "22.3 LTS",
                    "updatable_to": [
                        "21.2",
                        "21.3",
                        "21.4",
                        "21.7",
                        "21.8",
                        "21.9",
                        "21.10",
                        "21.11",
                        "22.1",
                        "22.5"
                    ]
                },
                {
                    "deprecated": false,
                    "id": "22.5",
                    "name": "22.5",
                    "updatable_to": [
                        "21.2",
                        "21.3",
                        "21.4",
                        "21.7",
                        "21.8",
                        "21.9",
                        "21.10",
                        "21.11",
                        "22.1",
                        "22.3"
                    ]
                }
            ],
            "default_version": "21.3",
            "clickhouse": {
                "default_resource_preset_id": "test_preset_id",
                "resource_presets": [
                    {
                        "cpu_limit": "1",
                        "max_disk_size": "2199023255552",
                        "memory_limit": "4294967296",
                        "min_disk_size": "34359738368",
                        "resource_preset_id": "a1.aws.1",
                        "disk_sizes": [
                            "34359738368",
                            "68719476736",
                            "137438953472",
                            "274877906944",
                            "549755813888",
                            "1099511627776",
                            "2199023255552"
                        ],
                        "default_disk_size": "68719476736"
                    },
                    {
                        "cpu_limit": "2",
                        "max_disk_size": "2199023255552",
                        "memory_limit": "8589934592",
                        "min_disk_size": "34359738368",
                        "resource_preset_id": "a1.aws.2",
                        "disk_sizes": [
                            "34359738368",
                            "68719476736",
                            "137438953472",
                            "274877906944",
                            "549755813888",
                            "1099511627776",
                            "2199023255552"
                        ],
                        "default_disk_size": "68719476736"
                    },
                    {
                        "cpu_limit": "4",
                        "max_disk_size": "2199023255552",
                        "memory_limit": "17179869184",
                        "min_disk_size": "34359738368",
                        "resource_preset_id": "a1.aws.3",
                        "disk_sizes": [
                            "34359738368",
                            "68719476736",
                            "137438953472",
                            "274877906944",
                            "549755813888",
                            "1099511627776",
                            "2199023255552"
                        ],
                        "default_disk_size": "68719476736"
                    },
                    {
                        "cpu_limit": "8",
                        "max_disk_size": "2199023255552",
                        "memory_limit": "34359738368",
                        "min_disk_size": "34359738368",
                        "resource_preset_id": "a1.aws.4",
                        "disk_sizes": [
                            "34359738368",
                            "68719476736",
                            "137438953472",
                            "274877906944",
                            "549755813888",
                            "1099511627776",
                            "2199023255552"
                        ],
                        "default_disk_size": "68719476736"
                    },
                    {
                        "cpu_limit": "16",
                        "max_disk_size": "2199023255552",
                        "memory_limit": "68719476736",
                        "min_disk_size": "34359738368",
                        "resource_preset_id": "a1.aws.5",
                        "disk_sizes": [
                            "34359738368",
                            "68719476736",
                            "137438953472",
                            "274877906944",
                            "549755813888",
                            "1099511627776",
                            "2199023255552"
                        ],
                        "default_disk_size": "68719476736"
                    }
                ]
            }
        }
        """

    Scenario: Estimate Create cluster
        When we "EstimateCreate" via DataCloud at "datacloud.clickhouse.console.v1.ClusterService" with data
        # language=json
        """
        {
            "project_id": "folder1",
            "cloud_type": "aws",
            "region_id": "eu-central1",
            "name": "test_cluster",
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
        Then we get DataCloud response with body ignoring empty
        # language=json
        """
        {
            "metrics": [{
                "folder_id": "folder1",
                "schema": "mdb.db.dc.v1",
                "tags": {
                    "cloud_provider": "aws",
                    "cloud_region": "eu-central1",
                    "cluster_type": "clickhouse_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "34359738368",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "public_ip": "1",
                    "resource_preset_id": "a1.aws.1",
                    "resource_preset_type": "aws-standard",
                    "roles": ["clickhouse_cluster"]
                }
            }, {
                "folder_id": "folder1",
                "schema": "mdb.db.dc.v1",
                "tags": {
                    "cloud_provider": "aws",
                    "cloud_region": "eu-central1",
                    "cluster_type": "clickhouse_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "34359738368",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "public_ip": "1",
                    "resource_preset_id": "a1.aws.1",
                    "resource_preset_type": "aws-standard",
                    "roles": ["clickhouse_cluster"]
                }
            }, {
                "folder_id": "folder1",
                "schema": "mdb.db.dc.v1",
                "tags": {
                    "cloud_provider": "aws",
                    "cloud_region": "eu-central1",
                    "cluster_type": "clickhouse_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "34359738368",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "public_ip": "1",
                    "resource_preset_id": "a1.aws.1",
                    "resource_preset_type": "aws-standard",
                    "roles": ["clickhouse_cluster"]
                }
            }]
        }
        """

    Scenario: List clouds available for ClickHouse cluster
        When we "List" via DataCloud at "datacloud.clickhouse.console.v1.CloudService" with data
        # language=json
        """
        {}
        """
        Then we get DataCloud response with body
        # language=json
        """
        {
            "clouds": [{
                "cloud_type": "aws",
                "region_ids": ["eu-central1", "us-east2"]
            }, {
                "cloud_type": "yandex",
                "region_ids": ["ru-central-1"]
            }],
            "next_page": {
                "token": ""
            }
        }
        """

    Scenario: List clouds available for ClickHouse cluster paging works
        When we "List" via DataCloud at "datacloud.clickhouse.console.v1.CloudService" with data
        # language=json
        """
        {
            "paging": {
                "page_size": 1
            }
        }
        """
        Then we get DataCloud response with body
        # language=json
        """
        {
            "clouds": [{
                "cloud_type": "aws",
                "region_ids": ["eu-central1", "us-east2"]
            }],
            "next_page": {
                "token": "eyJPZmZzZXQiOjF9"
            }
        }
        """
        When we "List" via DataCloud at "datacloud.clickhouse.console.v1.CloudService" with data
        # language=json
        """
        {
            "paging": {
                "page_token": "eyJPZmZzZXQiOjF9"
            }
        }
        """
        Then we get DataCloud response with body
        # language=json
        """
        {
            "clouds": [{
                "cloud_type": "yandex",
                "region_ids": ["ru-central-1"]
            }],
            "next_page": {
                "token": ""
            }
        }
        """
