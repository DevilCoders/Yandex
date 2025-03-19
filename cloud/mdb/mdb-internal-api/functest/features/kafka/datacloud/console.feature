@kafka @grpc_api
Feature: Test Kafka console api

    Background:
        Given default headers

    Scenario: Estimate Create cluster
        When we "EstimateCreate" via DataCloud at "datacloud.kafka.console.v1.ClusterService" with data
        # language=json
        """
        {
            "project_id": "folder1",
            "cloud_type": "aws",
            "region_id": "eu-central1",
            "name": "test_cluster",
            "description": "test cluster",
            "resources": {
                "kafka": {
                    "resource_preset_id": "a1.aws.1",
                    "disk_size": "34359738368",
                    "broker_count": 1,
                    "zone_count": 3
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
                    "cluster_type": "kafka_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "34359738368",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "resource_preset_id": "a1.aws.1",
                    "resource_preset_type": "aws-standard",
                    "roles": ["kafka_cluster"]
                }
            }, {
                "folder_id": "folder1",
                "schema": "mdb.db.dc.v1",
                "tags": {
                    "cloud_provider": "aws",
                    "cloud_region": "eu-central1",
                    "cluster_type": "kafka_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "34359738368",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "resource_preset_id": "a1.aws.1",
                    "resource_preset_type": "aws-standard",
                    "roles": ["kafka_cluster"]
                }
            }, {
                "folder_id": "folder1",
                "schema": "mdb.db.dc.v1",
                "tags": {
                    "cloud_provider": "aws",
                    "cloud_region": "eu-central1",
                    "cluster_type": "kafka_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "34359738368",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "resource_preset_id": "a1.aws.1",
                    "resource_preset_type": "aws-standard",
                    "roles": ["kafka_cluster"]
                }
            }, {
                "folder_id": "folder1",
                "schema": "mdb.db.dc.v1",
                "tags": {
                    "cloud_provider": "aws",
                    "cloud_region": "eu-central1",
                    "cluster_type": "kafka_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "34359738368",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "resource_preset_id": "a1.aws.1",
                    "resource_preset_type": "aws-standard",
                    "roles": ["zk"]
                }
            }, {
                "folder_id": "folder1",
                "schema": "mdb.db.dc.v1",
                "tags": {
                    "cloud_provider": "aws",
                    "cloud_region": "eu-central1",
                    "cluster_type": "kafka_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "34359738368",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "resource_preset_id": "a1.aws.1",
                    "resource_preset_type": "aws-standard",
                    "roles": ["zk"]
                }
            }, {
                "folder_id": "folder1",
                "schema": "mdb.db.dc.v1",
                "tags": {
                    "cloud_provider": "aws",
                    "cloud_region": "eu-central1",
                    "cluster_type": "kafka_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "34359738368",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "resource_preset_id": "a1.aws.1",
                    "resource_preset_type": "aws-standard",
                    "roles": ["zk"]
                }
            }]
        }
        """
