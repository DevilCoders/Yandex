@grpc_api
Feature: Create/Modify DataCloud ClickHouse Cluster

  Background:
    Given default headers
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster",
        "description": "test cluster",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "replica_count": 3
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

  Scenario: Create ClickHouse cluster
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "sensitive": false
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid1",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "host": "rw.cid1.yadc.io",
            "user": "admin",
            "password": "",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://rw.cid1.yadc.io:8443",
            "native_protocol": "rw.cid1.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true",
            "odbc_uri": "https://rw.cid1.yadc.io:8443"
        },
        "private_connection_info": {
            "host": "rw.cid1.private.yadc.io",
            "user": "admin",
            "password": "",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://rw.cid1.private.yadc.io:8443",
            "native_protocol": "rw.cid1.private.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true",
            "odbc_uri": "https://rw.cid1.private.yadc.io:8443"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "shard_count": "1",
                "replica_count": "3"
            }
        }
    }
    """
    When we GET via PillarConfig "/v1/config/ach-ec1a-s1-1.cid1.yadc.io"
    Then we get response with status 200
    And body at path "$.data" contains
    """
    {
        "backup": {
            "sleep": 7200,
            "start": {
                    "hours": 0,
                    "minutes": 0,
                    "nanos": 0,
                    "seconds": 0
            },
            "use_backup_service": false
        },
        "runlist": [
            "components.clickhouse_cluster"
        ],
        "versions": {},
        "region_id": "eu-central1",
        "s3_bucket": "yandexcloud-dbaas-cid1",
        "unmanaged": {
            "enable_zk_tls": true
        },
        "clickhouse": {
            "users": {},
            "config": {
                "timezone": "Europe/Moscow",
                "log_level": "information",
                "merge_tree": {
                    "enable_mixed_granularity_parts": true,
                    "replicated_deduplication_window": 100,
                    "replicated_deduplication_window_seconds": 604800
                },
                "compression": [],
                "mark_cache_size": 5368709120,
                "max_connections": 4096,
                "keep_alive_timeout": 3,
                "max_concurrent_queries": 500,
                "max_table_size_to_drop": 53687091200,
                "uncompressed_cache_size": 8589934592,
                "max_partition_size_to_drop": 53687091200,
                "builtin_dictionaries_reload_interval": 3600
            },
            "models": {},
            "shards": {
                "shard_id1": {
                    "weight": 100
                }
            },
            "keeper_hosts": {
                "ach-ec1a-s1-1.cid1.yadc.io": 1,
                "ach-ec1b-s1-2.cid1.yadc.io": 2,
                "ach-ec1c-s1-3.cid1.yadc.io": 3
            },
            "zk_users": {
                "clickhouse": {
                    "password": {
                        "data": "random_string3",
                        "encryption_version": 0
                    }
                }
            },
            "databases": [],
            "ch_version": "22.5.1.2079",
            "cluster_name": "default",
                "admin_password": {
                "hash": {
                    "data": "61a1438d394875ea4d8aec1ac5d8c66e46735ee5f5e3e23e3af558b49385586b",
                    "encryption_version": 0
                },
                "password": {
                    "data": "random_string1",
                    "encryption_version": 0
                }
            },
            "embedded_keeper": true,
            "format_schemas": {},
            "user_management_v2": true,
            "sql_user_management": true,
            "sql_database_management": true,
            "system_users": {
                "mdb_backup_admin": {
                    "hash": {
                        "data": "0bfa31cac3b32d99936b6643c4d410717f1c82e4ba6a85cfbd175fe96fbbc818",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "random_string4",
                        "encryption_version": 0
                    }
                }
            },
            "embedded_keeper": true,
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "interserver_credentials": {
                "user": "interserver",
                "password": {
                    "data": "random_string2",
                    "encryption_version": 0
                }
            }
        },
        "cloud_type": "aws",
        "clickhouse default pillar": true,
        "cloud_storage": {
            "enabled": true,
            "s3": {
                "bucket": "cloud-storage-cid1"
            },
            "settings": {
                "data_cache_enabled": true,
                "data_cache_max_size": 17179869184
            }
        },
        "testing_repos": false,
        "default pillar": true,
        "cluster_private_key": {
            "data": "1",
            "encryption_version": 0
        }
    }
    """
    And body at path "$.data.dbaas" contains
    """
    {
        "fqdn": "ach-ec1a-s1-1.cid1.yadc.io",
        "geo": "eu-central-1a",
        "region": "eu-central1",
        "cloud_provider": "aws",
        "cloud": {
            "cloud_ext_id": "cloud1"
        },
        "vtype": "aws",
        "flavor": {
            "id": "00000000-0000-0000-0000-000000000046",
            "name": "a1.aws.1",
            "type": "aws-standard",
            "vtype": "aws",
            "io_limit": 20971520,
            "cpu_limit": 1.0,
            "gpu_limit": 0,
            "generation": 1,
            "description": "a1.aws.1",
            "platform_id": "mdb-v1",
            "cpu_fraction": 100,
            "memory_limit": 4294967296,
            "cpu_guarantee": 1.0,
            "network_limit": 16777216,
            "io_cores_limit": 0,
            "memory_guarantee": 4294967296,
            "network_guarantee": 16777216
        },
        "folder": {
            "folder_ext_id": "folder1"
        },
        "cluster": {
            "subclusters": {
                "subcid1": {
                    "name": "clickhouse_subcluster",
                    "hosts": {},
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "shards": {
                        "shard_id1": {
                            "name": "s1",
                            "hosts": {
                                "ach-ec1a-s1-1.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s1-2.cid1.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1c-s1-3.cid1.yadc.io": {"geo": "eu-central-1c"}
                            }
                        }
                    }
                }
            }
        },
        "shard_id": "shard_id1",
        "vtype_id": null,
        "cluster_id": "cid1",
        "shard_name": "s1",
        "shard_hosts": [
            "ach-ec1a-s1-1.cid1.yadc.io",
            "ach-ec1b-s1-2.cid1.yadc.io",
            "ach-ec1c-s1-3.cid1.yadc.io"
        ],
        "space_limit": 34359738368,
        "cluster_name": "test_cluster",
        "cluster_type": "clickhouse_cluster",
        "disk_type_id": "local-ssd",
        "cluster_hosts": [
            "ach-ec1a-s1-1.cid1.yadc.io",
            "ach-ec1b-s1-2.cid1.yadc.io",
            "ach-ec1c-s1-3.cid1.yadc.io"
        ],
        "subcluster_id": "subcid1",
        "subcluster_name": "clickhouse_subcluster",
        "assign_public_ip": true,
        "on_dedicated_host": false
    }
    """

  Scenario: List ClickHouse clusters
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster2",
        "description": "test cluster",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "replica_count": 5
            }
        }
    }
    """
    Then we get DataCloud response OK
    When "worker_task_id2" acquired and finished by worker
    Then we run query
    """
    UPDATE dbaas.clusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we "List" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
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
            "id": "cid1",
            "project_id": "folder1",
            "cloud_type": "aws",
            "connection_info": {
                "host": "rw.cid1.yadc.io",
                "user": "admin",
                "password": "",
                "https_port": "8443",
                "tcp_port_secure": "9440",
                "https_uri": "https://rw.cid1.yadc.io:8443",
                "native_protocol": "rw.cid1.yadc.io:9440",
                "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true",
                "odbc_uri": "https://rw.cid1.yadc.io:8443"
            },
            "private_connection_info": {
                "host": "rw.cid1.private.yadc.io",
                "user": "admin",
                "password": "",
                "https_port": "8443",
                "tcp_port_secure": "9440",
                "https_uri": "https://rw.cid1.private.yadc.io:8443",
                "native_protocol": "rw.cid1.private.yadc.io:9440",
                "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true",
                "odbc_uri": "https://rw.cid1.private.yadc.io:8443"
            },
            "region_id": "eu-central1",
            "network_id": "network1",
            "name": "test_cluster",
            "description": "test cluster",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "create_time": "2000-01-01T00:00:00Z",
            "version": "22.5",
            "resources": {
                "clickhouse": {
                    "disk_size": "34359738368",
                    "replica_count": "3",
                    "resource_preset_id": "a1.aws.1",
                    "shard_count": "1"
                }
            },
            "access": null,
            "encryption": null,
            "clickhouse_config": null
        }, {
            "id": "cid2",
            "project_id": "folder1",
            "cloud_type": "aws",
            "connection_info": {
                "host": "rw.cid2.yadc.io",
                "user": "admin",
                "password": "",
                "https_port": "8443",
                "tcp_port_secure": "9440",
                "https_uri": "https://rw.cid2.yadc.io:8443",
                "native_protocol": "rw.cid2.yadc.io:9440",
                "jdbc_uri": "jdbc:clickhouse://rw.cid2.yadc.io:8443/default?ssl=true",
                "odbc_uri": "https://rw.cid2.yadc.io:8443"
            },
            "private_connection_info": {
                "host": "rw.cid2.private.yadc.io",
                "user": "admin",
                "password": "",
                "https_port": "8443",
                "tcp_port_secure": "9440",
                "https_uri": "https://rw.cid2.private.yadc.io:8443",
                "native_protocol": "rw.cid2.private.yadc.io:9440",
                "jdbc_uri": "jdbc:clickhouse://rw.cid2.private.yadc.io:8443/default?ssl=true",
                "odbc_uri": "https://rw.cid2.private.yadc.io:8443"
            },
            "region_id": "eu-central1",
            "network_id": "network1",
            "name": "test_cluster2",
            "description": "test cluster",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "create_time": "2000-01-01T00:00:00Z",
            "version": "22.5",
            "resources": {
                "clickhouse": {
                    "disk_size": "34359738368",
                    "replica_count": "5",
                    "resource_preset_id": "a1.aws.1",
                    "shard_count": "1"
                }
            },
            "access": null,
            "encryption": null,
            "clickhouse_config": null
        }]
    }
    """

  Scenario: List ClickHouse clusters with pagination works
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster2",
        "description": "test cluster",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "replica_count": 5
            }
        }
    }
    """
    Then we get DataCloud response OK
    When "worker_task_id2" acquired and finished by worker
    Then we run query
    """
    UPDATE dbaas.clusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we "List" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "paging": {
            "page_size": 1
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "clusters": [{
            "id": "cid1",
            "project_id": "folder1",
            "cloud_type": "aws",
            "connection_info": {
                "host": "rw.cid1.yadc.io",
                "user": "admin",
                "password": "",
                "https_port": "8443",
                "https_uri": "https://rw.cid1.yadc.io:8443",
                "tcp_port_secure": "9440",
                "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true",
                "native_protocol": "rw.cid1.yadc.io:9440",
                "odbc_uri": "https://rw.cid1.yadc.io:8443"
            },
            "private_connection_info": {
                "host": "rw.cid1.private.yadc.io",
                "user": "admin",
                "password": "",
                "https_port": "8443",
                "https_uri": "https://rw.cid1.private.yadc.io:8443",
                "tcp_port_secure": "9440",
                "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true",
                "native_protocol": "rw.cid1.private.yadc.io:9440",
                "odbc_uri": "https://rw.cid1.private.yadc.io:8443"
            },
            "region_id": "eu-central1",
            "network_id": "network1",
            "name": "test_cluster",
            "description": "test cluster",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "create_time": "2000-01-01T00:00:00Z",
            "version": "22.5",
            "resources": {
                "clickhouse": {
                    "disk_size": "34359738368",
                    "replica_count": "3",
                    "resource_preset_id": "a1.aws.1",
                    "shard_count": "1"
                }
            },
            "access": null,
            "encryption": null,
            "clickhouse_config": null
        }],
        "next_page": {
            "token": "eyJMYXN0Q2x1c3Rlck5hbWUiOiJ0ZXN0X2NsdXN0ZXIifQ=="
        }
    }
    """
    And we "List" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "paging": {
            "page_size": 1,
            "page_token": "eyJMYXN0Q2x1c3Rlck5hbWUiOiJ0ZXN0X2NsdXN0ZXIifQ=="
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "clusters": [{
            "id": "cid2",
            "project_id": "folder1",
            "cloud_type": "aws",
            "connection_info": {
                "host": "rw.cid2.yadc.io",
                "user": "admin",
                "password": "",
                "https_port": "8443",
                "https_uri": "https://rw.cid2.yadc.io:8443",
                "tcp_port_secure": "9440",
                "jdbc_uri": "jdbc:clickhouse://rw.cid2.yadc.io:8443/default?ssl=true",
                "native_protocol": "rw.cid2.yadc.io:9440",
                "odbc_uri": "https://rw.cid2.yadc.io:8443"
            },
            "private_connection_info": {
                "host": "rw.cid2.private.yadc.io",
                "user": "admin",
                "password": "",
                "https_port": "8443",
                "https_uri": "https://rw.cid2.private.yadc.io:8443",
                "tcp_port_secure": "9440",
                "jdbc_uri": "jdbc:clickhouse://rw.cid2.private.yadc.io:8443/default?ssl=true",
                "native_protocol": "rw.cid2.private.yadc.io:9440",
                "odbc_uri": "https://rw.cid2.private.yadc.io:8443"
            },
            "region_id": "eu-central1",
            "network_id": "network1",
            "name": "test_cluster2",
            "description": "test cluster",
            "status": "CLUSTER_STATUS_UNKNOWN",
            "create_time": "2000-01-01T00:00:00Z",
            "version": "22.5",
            "resources": {
                "clickhouse": {
                    "disk_size": "34359738368",
                    "replica_count": "5",
                    "resource_preset_id": "a1.aws.1",
                    "shard_count": "1"
                }
            },
            "access": null,
            "encryption": null,
            "clickhouse_config": null
        }],
        "next_page": {
            "token": "eyJMYXN0Q2x1c3Rlck5hbWUiOiJ0ZXN0X2NsdXN0ZXIyIn0="
        }
    }
    """
    And we "List" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "paging": {
            "page_size": 1,
            "page_token": "eyJMYXN0Q2x1c3Rlck5hbWUiOiJ0ZXN0X2NsdXN0ZXIyIn0="
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "clusters": [],
        "next_page": { "token": "" }
    }
    """

  Scenario: Delete ClickHouse cluster
    When we "Delete" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "List" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
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
        "clusters": []
    }
    """

  Scenario: Modify cluster works
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "new_name",
        "description": "new cluster description",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.3",
                "disk_size": 68719476736
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
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "sensitive": true
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid1",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "host": "rw.cid1.yadc.io",
            "user": "admin",
            "password": "random_string1",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443",
            "native_protocol": "rw.cid1.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
            "odbc_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443"
        },
        "private_connection_info": {
            "host": "rw.cid1.private.yadc.io",
            "user": "admin",
            "password": "random_string1",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443",
            "native_protocol": "rw.cid1.private.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
            "odbc_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "new_name",
        "description": "new cluster description",
        "status": "CLUSTER_STATUS_UNKNOWN",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "disk_size": "68719476736",
                "replica_count": "3",
                "resource_preset_id": "a1.aws.3",
                "shard_count": "1"
            }
        }
    }
    """
    When we GET via PillarConfig "/v1/config/ach-ec1a-s1-1.cid1.yadc.io"
    Then we get response with status 200
    And body at path "$.data" contains
    """
    {
        "backup": {
            "sleep": 7200,
            "start": {
                    "hours": 0,
                    "minutes": 0,
                    "nanos": 0,
                    "seconds": 0
            },
            "use_backup_service": false
        },
        "runlist": [
            "components.clickhouse_cluster"
        ],
        "versions": {},
        "region_id": "eu-central1",
        "s3_bucket": "yandexcloud-dbaas-cid1",
        "unmanaged": {
            "enable_zk_tls": true
        },
        "clickhouse": {
            "users": {},
            "config": {
                "timezone": "Europe/Moscow",
                "log_level": "information",
                "merge_tree": {
                    "enable_mixed_granularity_parts": true,
                    "replicated_deduplication_window": 100,
                    "replicated_deduplication_window_seconds": 604800
                },
                "compression": [],
                "mark_cache_size": 5368709120,
                "max_connections": 4096,
                "keep_alive_timeout": 3,
                "max_concurrent_queries": 500,
                "max_table_size_to_drop": 53687091200,
                "uncompressed_cache_size": 8589934592,
                "max_partition_size_to_drop": 53687091200,
                "builtin_dictionaries_reload_interval": 3600
            },
            "models": {},
            "shards": {
                "shard_id1": {
                    "weight": 100
                }
            },
            "keeper_hosts": {
                "ach-ec1a-s1-1.cid1.yadc.io": 1,
                "ach-ec1b-s1-2.cid1.yadc.io": 2,
                "ach-ec1c-s1-3.cid1.yadc.io": 3
            },
            "zk_users": {
                "clickhouse": {
                    "password": {
                        "data": "random_string3",
                        "encryption_version": 0
                    }
                }
            },
            "databases": [],
            "ch_version": "22.5.1.2079",
            "cluster_name": "default",
            "admin_password": {
                "hash": {
                    "data": "61a1438d394875ea4d8aec1ac5d8c66e46735ee5f5e3e23e3af558b49385586b",
                    "encryption_version": 0
                },
                "password": {
                    "data": "random_string1",
                    "encryption_version": 0
                }
            },
            "embedded_keeper": true,
            "format_schemas": {},
            "user_management_v2": true,
            "sql_user_management": true,
            "sql_database_management": true,
            "system_users": {
                "mdb_backup_admin": {
                    "hash": {
                        "data": "0bfa31cac3b32d99936b6643c4d410717f1c82e4ba6a85cfbd175fe96fbbc818",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "random_string4",
                        "encryption_version": 0
                    }
                }
            },
            "embedded_keeper": true,
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "interserver_credentials": {
                "user": "interserver",
                "password": {
                    "data": "random_string2",
                    "encryption_version": 0
                }
            }
        },
        "cloud_type": "aws",
        "clickhouse default pillar": true,
        "cloud_storage": {
            "enabled": true,
            "s3": {
                "bucket": "cloud-storage-cid1"
            },
            "settings": {
                "data_cache_enabled": true,
                "data_cache_max_size": 17179869184
            }
        },
        "testing_repos": false,
        "default pillar": true,
        "cluster_private_key": {
            "data": "1",
            "encryption_version": 0
        }
    }
    """
    And body at path "$.data.dbaas" contains
    """
    {
        "fqdn": "ach-ec1a-s1-1.cid1.yadc.io",
        "geo": "eu-central-1a",
        "region": "eu-central1",
        "cloud_provider": "aws",
        "cloud": {
            "cloud_ext_id": "cloud1"
        },
        "vtype": "aws",
        "flavor": {
            "id": "00000000-0000-0000-0000-000000000048",
            "name": "a1.aws.3",
            "type": "aws-standard",
            "vtype": "aws",
            "io_limit": 83886080,
            "cpu_limit": 4.0,
            "gpu_limit": 0,
            "generation": 1,
            "description": "a1.aws.3",
            "platform_id": "mdb-v1",
            "cpu_fraction": 100,
            "memory_limit": 17179869184,
            "cpu_guarantee": 4.0,
            "network_limit": 67108864,
            "io_cores_limit": 0,
            "memory_guarantee": 17179869184,
            "network_guarantee": 67108864
        },
        "folder": {
            "folder_ext_id": "folder1"
        },
        "cluster": {
            "subclusters": {
                "subcid1": {
                    "name": "clickhouse_subcluster",
                    "hosts": {},
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "shards": {
                        "shard_id1": {
                            "name": "s1",
                            "hosts": {
                                "ach-ec1a-s1-1.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s1-2.cid1.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1c-s1-3.cid1.yadc.io": {"geo": "eu-central-1c"}
                            }
                        }
                    }
                }
            }
        },
        "shard_id": "shard_id1",
        "vtype_id": null,
        "cluster_id": "cid1",
        "shard_name": "s1",
        "shard_hosts": [
            "ach-ec1a-s1-1.cid1.yadc.io",
            "ach-ec1b-s1-2.cid1.yadc.io",
            "ach-ec1c-s1-3.cid1.yadc.io"
        ],
        "space_limit": 68719476736,
        "cluster_name": "new_name",
        "cluster_type": "clickhouse_cluster",
        "disk_type_id": "local-ssd",
        "cluster_hosts": [
            "ach-ec1a-s1-1.cid1.yadc.io",
            "ach-ec1b-s1-2.cid1.yadc.io",
            "ach-ec1c-s1-3.cid1.yadc.io"
        ],
        "subcluster_id": "subcid1",
        "subcluster_name": "clickhouse_subcluster",
        "assign_public_ip": true,
        "on_dedicated_host": false
    }
    """

  Scenario: Decrease disk size fails
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "resources": {
            "clickhouse": {
                "disk_size": 68719476736
            }
        }
    }
    """
    Then we get DataCloud response OK
    When "worker_task_id2" acquired and finished by worker
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "resources": {
            "clickhouse": {
                "disk_size": 34359738368
            }
        }
    }
    """
    Then we get DataCloud response error with code INVALID_ARGUMENT and message "unable to decrease disk size"


  Scenario: Upgrade cluster works
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "version": "22.3"
    }
    """
    Then we get DataCloud response OK
    And in worker_queue exists "worker_task_id2" id and data contains
    """
    {
        "task_type": "clickhouse_cluster_upgrade"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "sensitive": true
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid1",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "host": "rw.cid1.yadc.io",
            "user": "admin",
            "password": "random_string1",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443",
            "native_protocol": "rw.cid1.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
            "odbc_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443"
        },
        "private_connection_info": {
            "host": "rw.cid1.private.yadc.io",
            "user": "admin",
            "password": "random_string1",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443",
            "native_protocol": "rw.cid1.private.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
            "odbc_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN",
        "version": "22.3",
        "resources": {
            "clickhouse": {
                "disk_size": "34359738368",
                "replica_count": "3",
                "resource_preset_id": "a1.aws.1",
                "shard_count": "1"
            }
        }
    }
    """

  Scenario: Upgrade cluster with resource changes fails
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "version": "22.3",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.3",
                "disk_size": 68719476736
            }
        }
    }
    """
    Then we get DataCloud response error with code INVALID_ARGUMENT and message "version update cannot be mixed with update of host resources"

  Scenario: Add new ClickHouse replicas works
    When we run query
    """
    UPDATE dbaas.clouds
    SET cpu_quota = 240, memory_quota = 1030792151040, ssd_space_quota = 64424509440000
    """
    And we run query
    """
    UPDATE dbaas.clouds_revs
    SET cpu_quota = 240, memory_quota = 1030792151040, ssd_space_quota = 64424509440000
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
                "replica_count": 6
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
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "sensitive": true
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid1",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "host": "rw.cid1.yadc.io",
            "user": "admin",
            "password": "random_string1",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443",
            "native_protocol": "rw.cid1.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
            "odbc_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443"
        },
        "private_connection_info": {
            "host": "rw.cid1.private.yadc.io",
            "user": "admin",
            "password": "random_string1",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443",
            "native_protocol": "rw.cid1.private.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
            "odbc_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "new_name",
        "description": "new cluster description",
        "status": "CLUSTER_STATUS_UNKNOWN"
    }
    """
    When we GET via PillarConfig "/v1/config/ach-ec1a-s1-4.cid1.yadc.io"
    """
    {
        "backup": {
            "sleep": 7200,
            "start": {
                    "hours": 0,
                    "minutes": 0,
                    "nanos": 0,
                    "seconds": 0
            },
            "use_backup_service": false
        },
        "runlist": [
            "components.clickhouse_cluster"
        ],
        "versions": {},
        "region_id": "eu-central1",
        "s3_bucket": "yandexcloud-dbaas-cid1",
        "unmanaged": {
            "enable_zk_tls": true
        },
        "clickhouse": {
            "users": {},
            "config": {
                "timezone": "Europe/Moscow",
                "log_level": "information",
                "merge_tree": {
                    "enable_mixed_granularity_parts": true,
                    "replicated_deduplication_window": 100,
                    "replicated_deduplication_window_seconds": 604800
                },
                "compression": [],
                "mark_cache_size": 5368709120,
                "max_connections": 4096,
                "keep_alive_timeout": 3,
                "max_concurrent_queries": 500,
                "max_table_size_to_drop": 53687091200,
                "uncompressed_cache_size": 8589934592,
                "max_partition_size_to_drop": 53687091200,
                "builtin_dictionaries_reload_interval": 3600
            },
            "models": {},
            "shards": {
                "shard_id1": {
                    "weight": 100
                }
            },
            "keeper_hosts": {"ach-ec1a-s1-1.cid1.yadc.io": 1, "ach-ec1b-s1-2.cid1.yadc.io": 2, "ach-ec1c-s1-3.cid1.yadc.io": 3},
            "zk_users": {
                "clickhouse": {
                    "password": {
                        "data": "random_string3",
                        "encryption_version": 0
                    }
                }
            },
            "databases": [],
            "ch_version": "22.5.1.2079",
            "cluster_name": "default",
            "admin_password": {
                "hash": {
                    "data": "61a1438d394875ea4d8aec1ac5d8c66e46735ee5f5e3e23e3af558b49385586b",
                    "encryption_version": 0
                },
                "password": {
                    "data": "random_string1",
                    "encryption_version": 0
                }
            },
            "embedded_keeper": true,
            "format_schemas": {},
            "user_management_v2": true,
            "sql_user_management": true,
            "sql_database_management": true,
            "system_users": {
                "mdb_backup_admin": {
                    "hash": {
                        "data": "0bfa31cac3b32d99936b6643c4d410717f1c82e4ba6a85cfbd175fe96fbbc818",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "random_string4",
                        "encryption_version": 0
                    }
                }
            },
            "embedded_keeper": true,
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "interserver_credentials": {
                "user": "interserver",
                "password": {
                    "data": "random_string2",
                    "encryption_version": 0
                }
            }
        },
        "cloud_type": "aws",
        "clickhouse default pillar": true,
        "cloud_storage": {
            "enabled": true,
            "s3": {
                "bucket": "cloud-storage-cid1"
            },
            "settings": {
                "data_cache_enabled": true,
                "data_cache_max_size": 17179869184
            }
        },
        "testing_repos": false,
        "default pillar": true,
        "cluster_private_key": {
            "data": "1",
            "encryption_version": 0
        }
    }
    """
    And body at path "$.data.dbaas" contains
    """
    {
        "fqdn": "ach-ec1a-s1-4.cid1.yadc.io",
        "geo": "eu-central-1a",
        "region": "eu-central1",
        "cloud_provider": "aws",
        "cloud": {
            "cloud_ext_id": "cloud1"
        },
        "vtype": "aws",
        "flavor": {
            "id": "00000000-0000-0000-0000-000000000048",
            "name": "a1.aws.3",
            "type": "aws-standard",
            "vtype": "aws",
            "io_limit": 83886080,
            "cpu_limit": 4.0,
            "gpu_limit": 0,
            "generation": 1,
            "description": "a1.aws.3",
            "platform_id": "mdb-v1",
            "cpu_fraction": 100,
            "memory_limit": 17179869184,
            "cpu_guarantee": 4.0,
            "network_limit": 67108864,
            "io_cores_limit": 0,
            "memory_guarantee": 17179869184,
            "network_guarantee": 67108864
        },
        "folder": {
            "folder_ext_id": "folder1"
        },
        "cluster": {
            "subclusters": {
                "subcid1": {
                    "name": "clickhouse_subcluster",
                    "hosts": {},
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "shards": {
                        "shard_id1": {
                            "name": "s1",
                            "hosts": {
                                "ach-ec1a-s1-1.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1a-s1-4.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s1-2.cid1.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1b-s1-5.cid1.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1c-s1-3.cid1.yadc.io": {"geo": "eu-central-1c"},
                                "ach-ec1c-s1-6.cid1.yadc.io": {"geo": "eu-central-1c"}
                            }
                        }
                    }
                }
            }
        },
        "shard_id": "shard_id1",
        "vtype_id": null,
        "cluster_id": "cid1",
        "shard_name": "s1",
        "shard_hosts": [
            "ach-ec1a-s1-1.cid1.yadc.io",
            "ach-ec1a-s1-4.cid1.yadc.io",
            "ach-ec1b-s1-2.cid1.yadc.io",
            "ach-ec1b-s1-5.cid1.yadc.io",
            "ach-ec1c-s1-3.cid1.yadc.io",
            "ach-ec1c-s1-6.cid1.yadc.io"
        ],
        "space_limit": 68719476736,
        "cluster_name": "new_name",
        "cluster_type": "clickhouse_cluster",
        "disk_type_id": "local-ssd",
        "cluster_hosts": [
            "ach-ec1a-s1-1.cid1.yadc.io",
            "ach-ec1a-s1-4.cid1.yadc.io",
            "ach-ec1b-s1-2.cid1.yadc.io",
            "ach-ec1b-s1-5.cid1.yadc.io",
            "ach-ec1c-s1-3.cid1.yadc.io",
            "ach-ec1c-s1-6.cid1.yadc.io"
        ],
        "subcluster_id": "subcid1",
        "subcluster_name": "clickhouse_subcluster",
        "assign_public_ip": true,
        "on_dedicated_host": false
    }
    """

  Scenario: Delete ClickHouse replicas works
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.3",
                "disk_size": 68719476736,
                "replica_count": 1
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
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "sensitive": true
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid1",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "host": "rw.cid1.yadc.io",
            "user": "admin",
            "password": "random_string1",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443",
            "native_protocol": "rw.cid1.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
            "odbc_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443"
        },
        "private_connection_info": {
            "host": "rw.cid1.private.yadc.io",
            "user": "admin",
            "password": "random_string1",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443",
            "native_protocol": "rw.cid1.private.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
            "odbc_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN"
    }
    """
    When we GET via PillarConfig "/v1/config/ach-ec1a-s1-1.cid1.yadc.io"
    """
    {
        "backup": {
            "sleep": 7200,
            "start": {
                    "hours": 0,
                    "minutes": 0,
                    "nanos": 0,
                    "seconds": 0
            },
            "use_backup_service": false
        },
        "runlist": [
            "components.clickhouse_cluster"
        ],
        "versions": {},
        "region_id": "eu-central1",
        "s3_bucket": "yandexcloud-dbaas-cid1",
        "unmanaged": {
            "enable_zk_tls": true
        },
        "clickhouse": {
            "users": {},
            "config": {
                "timezone": "Europe/Moscow",
                "log_level": "information",
                "merge_tree": {
                    "enable_mixed_granularity_parts": true,
                    "replicated_deduplication_window": 100,
                    "replicated_deduplication_window_seconds": 604800
                },
                "compression": [],
                "mark_cache_size": 5368709120,
                "max_connections": 4096,
                "keep_alive_timeout": 3,
                "max_concurrent_queries": 500,
                "max_table_size_to_drop": 53687091200,
                "uncompressed_cache_size": 8589934592,
                "max_partition_size_to_drop": 53687091200,
                "builtin_dictionaries_reload_interval": 3600
            },
            "models": {},
            "shards": {
                "shard_id1": {
                    "weight": 100
                }
            },
            "keeper_hosts": {"ach-ec1a-s1-1.cid1.yadc.io": 1, "ach-ec1b-s1-2.cid1.yadc.io": 2, "ach-ec1c-s1-3.cid1.yadc.io": 3},
            "zk_users": {
                "clickhouse": {
                    "password": {
                        "data": "random_string3",
                        "encryption_version": 0
                    }
                }
            },
            "databases": [],
            "ch_version": "22.5.1.2079",
            "cluster_name": "default",
            "admin_password": {
                "hash": {
                    "data": "61a1438d394875ea4d8aec1ac5d8c66e46735ee5f5e3e23e3af558b49385586b",
                    "encryption_version": 0
                },
                "password": {
                    "data": "random_string1",
                    "encryption_version": 0
                }
            },
            "embedded_keeper": true,
            "format_schemas": {},
            "user_management_v2": true,
            "sql_user_management": true,
            "sql_database_management": true,
            "system_users": {
                "mdb_backup_admin": {
                    "hash": {
                        "data": "0bfa31cac3b32d99936b6643c4d410717f1c82e4ba6a85cfbd175fe96fbbc818",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "random_string4",
                        "encryption_version": 0
                    }
                }
            },
            "embedded_keeper": true,
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "interserver_credentials": {
                "user": "interserver",
                "password": {
                    "data": "random_string2",
                    "encryption_version": 0
                }
            }
        },
        "cloud_type": "aws",
        "clickhouse default pillar": true,
        "cloud_storage": {
            "enabled": true,
            "s3": {
                "bucket": "cloud-storage-cid1"
            },
            "settings": {
                "data_cache_enabled": true,
                "data_cache_max_size": 17179869184
            }
        },
        "testing_repos": false,
        "default pillar": true,
        "cluster_private_key": {
            "data": "1",
            "encryption_version": 0
        }
    }
    """
    And body at path "$.data.dbaas" contains
    """
    {
        "fqdn": "ach-ec1a-s1-1.cid1.yadc.io",
        "geo": "eu-central-1a",
        "region": "eu-central1",
        "cloud_provider": "aws",
        "cloud": {
            "cloud_ext_id": "cloud1"
        },
        "vtype": "aws",
        "flavor": {
            "id": "00000000-0000-0000-0000-000000000048",
            "name": "a1.aws.3",
            "type": "aws-standard",
            "vtype": "aws",
            "io_limit": 83886080,
            "cpu_limit": 4.0,
            "gpu_limit": 0,
            "generation": 1,
            "description": "a1.aws.3",
            "platform_id": "mdb-v1",
            "cpu_fraction": 100,
            "memory_limit": 17179869184,
            "cpu_guarantee": 4.0,
            "network_limit": 67108864,
            "io_cores_limit": 0,
            "memory_guarantee": 17179869184,
            "network_guarantee": 67108864
        },
        "folder": {
            "folder_ext_id": "folder1"
        },
        "cluster": {
            "subclusters": {
                "subcid1": {
                    "name": "clickhouse_subcluster",
                    "hosts": {},
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "shards": {
                        "shard_id1": {
                            "name": "s1",
                            "hosts": {
                                "ach-ec1a-s1-1.cid1.yadc.io": {"geo": "eu-central-1a"}
                            }
                        }
                    }
                }
            }
        },
        "shard_id": "shard_id1",
        "vtype_id": null,
        "cluster_id": "cid1",
        "shard_name": "s1",
        "shard_hosts": [
            "ach-ec1a-s1-1.cid1.yadc.io"
        ],
        "space_limit": 68719476736,
        "cluster_name": "test_cluster",
        "cluster_type": "clickhouse_cluster",
        "disk_type_id": "local-ssd",
        "cluster_hosts": [
            "ach-ec1a-s1-1.cid1.yadc.io"
        ],
        "subcluster_id": "subcid1",
        "subcluster_name": "clickhouse_subcluster",
        "assign_public_ip": true,
        "on_dedicated_host": false
    }
    """

  Scenario: Delete all ClickHouse replicas fails
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "resources": {
            "clickhouse": {
                "replica_count": 0
            }
        }
    }
    """
    Then we get DataCloud response error with code INVALID_ARGUMENT and message "invalid replica count '0', shards must have at least one replica"

  Scenario: Update cluster admin password
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "sensitive": true
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid1",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "host": "rw.cid1.yadc.io",
            "user": "admin",
            "password": "random_string1",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443",
            "native_protocol": "rw.cid1.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
            "odbc_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443"
        },
        "private_connection_info": {
            "host": "rw.cid1.private.yadc.io",
            "user": "admin",
            "password": "random_string1",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443",
            "native_protocol": "rw.cid1.private.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
            "odbc_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN"
    }
    """
    When we "ResetCredentials" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "sensitive": true
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid1",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "host": "rw.cid1.yadc.io",
            "user": "admin",
            "password": "random_string5",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string5@rw.cid1.yadc.io:8443",
            "native_protocol": "rw.cid1.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true&user=admin&password=random_string5",
            "odbc_uri": "https://admin:random_string5@rw.cid1.yadc.io:8443"
        },
        "private_connection_info": {
            "host": "rw.cid1.private.yadc.io",
            "user": "admin",
            "password": "random_string5",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://admin:random_string5@rw.cid1.private.yadc.io:8443",
            "native_protocol": "rw.cid1.private.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true&user=admin&password=random_string5",
            "odbc_uri": "https://admin:random_string5@rw.cid1.private.yadc.io:8443"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN"
    }
    """

  Scenario: Get sensitive data without permission acts like sensitive is false
    Given default headers with "ro-token" token
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "sensitive": true
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid1",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "host": "rw.cid1.yadc.io",
            "user": "admin",
            "password": "",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://rw.cid1.yadc.io:8443",
            "native_protocol": "rw.cid1.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true",
            "odbc_uri": "https://rw.cid1.yadc.io:8443"
        },
        "private_connection_info": {
            "host": "rw.cid1.private.yadc.io",
            "user": "admin",
            "password": "",
            "https_port": "8443",
            "tcp_port_secure": "9440",
            "https_uri": "https://rw.cid1.private.yadc.io:8443",
            "native_protocol": "rw.cid1.private.yadc.io:9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true",
            "odbc_uri": "https://rw.cid1.private.yadc.io:8443"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": "34359738368",
                "shard_count": "1",
                "replica_count": "3"
            }
        }
    }
    """

  Scenario: invalid project throws correct error
    When we "List" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {}
    """
    Then we get DataCloud response error with code INVALID_ARGUMENT and message "project id must be specified"
    When we "List" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "unknown"
    }
    """
    Then we get DataCloud response error with code NOT_FOUND and message "project id "unknown" not found"

  Scenario: Create/Get of CH cluster with Access params works
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster2",
        "description": "test cluster",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "replica_count": 5
            }
        },
        "access": {
            "ipv4_cidr_blocks": {
                "values":[{"value":"0.0.0.0/0"}]
                },
            "ipv6_cidr_blocks": {
                "values":[{"value":"::/0"}]
                },
            "data_services": {"values":[1, 2]}
        }
    }
    """
    Then we get DataCloud response OK
    When "worker_task_id2" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2",
        "sensitive": false
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid2",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "host": "rw.cid2.yadc.io",
            "user": "admin",
            "password": "",
            "https_port": "8443",
            "https_uri": "https://rw.cid2.yadc.io:8443",
            "tcp_port_secure": "9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid2.yadc.io:8443/default?ssl=true",
            "native_protocol": "rw.cid2.yadc.io:9440",
            "odbc_uri": "https://rw.cid2.yadc.io:8443"
        },
        "private_connection_info": {
            "host": "rw.cid2.private.yadc.io",
            "user": "admin",
            "password": "",
            "https_port": "8443",
            "https_uri": "https://rw.cid2.private.yadc.io:8443",
            "tcp_port_secure": "9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid2.private.yadc.io:8443/default?ssl=true",
            "native_protocol": "rw.cid2.private.yadc.io:9440",
            "odbc_uri": "https://rw.cid2.private.yadc.io:8443"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster2",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN",
        "create_time": "**IGNORE**",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "disk_size": "34359738368",
                "replica_count": "5",
                "resource_preset_id": "a1.aws.1",
                "shard_count": "1"
            }
        },
        "access": {
            "ipv4_cidr_blocks": {
                "values":[{"value":"0.0.0.0/0","description":""}]
                },
            "ipv6_cidr_blocks": {
                "values":[{"value":"::/0","description":""}]
                },
            "data_services": {"values":["DATA_SERVICE_VISUALIZATION", "DATA_SERVICE_TRANSFER"]}
        }

    }
    """

  Scenario: Modify cluster access works
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
  """
  {
      "cluster_id": "cid1",
      "version": "22.3",
      "access": {
        "ipv4_cidr_blocks": { "values":[{"value":"0.0.0.0/0","description":"test access"}]},
        "data_services": {"values":["DATA_SERVICE_VISUALIZATION"]}
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
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
  """
  {
      "cluster_id": "cid1",
      "sensitive": true
  }
  """
    Then we get DataCloud response with body
  """
  {
      "id": "cid1",
      "project_id": "folder1",
      "cloud_type": "aws",
      "connection_info": {
          "host": "rw.cid1.yadc.io",
          "user": "admin",
          "password": "random_string1",
          "https_port": "8443",
          "tcp_port_secure": "9440",
          "https_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443",
          "native_protocol": "rw.cid1.yadc.io:9440",
          "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
          "odbc_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443"
      },
      "private_connection_info": {
          "host": "rw.cid1.private.yadc.io",
          "user": "admin",
          "password": "random_string1",
          "https_port": "8443",
          "tcp_port_secure": "9440",
          "https_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443",
          "native_protocol": "rw.cid1.private.yadc.io:9440",
          "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
          "odbc_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443"
      },
      "region_id": "eu-central1",
      "network_id": "network1",
      "name": "test_cluster",
      "description": "test cluster",
      "status": "CLUSTER_STATUS_UNKNOWN",
      "version": "22.3",
      "resources": {
          "clickhouse": {
              "disk_size": "34359738368",
              "replica_count": "3",
              "resource_preset_id": "a1.aws.1",
              "shard_count": "1"
          }
      },
      "access": {
        "ipv4_cidr_blocks": { "values":[ {"value":"0.0.0.0/0", "description":"test access" } ] },
        "ipv6_cidr_blocks": null,
        "data_services": {"values":["DATA_SERVICE_VISUALIZATION"]}
      }
  }
  """
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
  """
  {
      "cluster_id": "cid1",
      "version": "22.3",
      "access": {
        "ipv4_cidr_blocks": { "values":[]},
        "data_services": {"values":["DATA_SERVICE_VISUALIZATION"]}
      }
  }
  """
    Then we get DataCloud response with body
  """
  {
      "operation_id": "worker_task_id3"
  }
  """
    When "worker_task_id3" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
  """
  {
      "cluster_id": "cid1",
      "sensitive": true
  }
  """
    Then we get DataCloud response with body
  """
  {
      "id": "cid1",
      "project_id": "folder1",
      "cloud_type": "aws",
      "connection_info": {
          "host": "rw.cid1.yadc.io",
          "user": "admin",
          "password": "random_string1",
          "https_port": "8443",
          "tcp_port_secure": "9440",
          "https_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443",
          "native_protocol": "rw.cid1.yadc.io:9440",
          "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
          "odbc_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443"
      },
      "private_connection_info": {
          "host": "rw.cid1.private.yadc.io",
          "user": "admin",
          "password": "random_string1",
          "https_port": "8443",
          "tcp_port_secure": "9440",
          "https_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443",
          "native_protocol": "rw.cid1.private.yadc.io:9440",
          "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
          "odbc_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443"
      },
      "region_id": "eu-central1",
      "network_id": "network1",
      "name": "test_cluster",
      "description": "test cluster",
      "status": "CLUSTER_STATUS_UNKNOWN",
      "version": "22.3",
      "resources": {
          "clickhouse": {
              "disk_size": "34359738368",
              "replica_count": "3",
              "resource_preset_id": "a1.aws.1",
              "shard_count": "1"
          }
      },
      "access": {
        "ipv4_cidr_blocks": null,
        "ipv6_cidr_blocks": null,
        "data_services": { "values":["DATA_SERVICE_VISUALIZATION"] }
      }
  }
  """
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
  """
  {
      "cluster_id": "cid1",
      "version": "22.3",
      "access": {
        "data_services": {"values":[]}
      }
  }
  """
    Then we get DataCloud response with body
  """
  {
      "operation_id": "worker_task_id4"
  }
  """
    When "worker_task_id4" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
  """
  {
      "cluster_id": "cid1",
      "sensitive": true
  }
  """
    Then we get DataCloud response with body
  """
  {
      "id": "cid1",
      "project_id": "folder1",
      "cloud_type": "aws",
      "connection_info": {
          "host": "rw.cid1.yadc.io",
          "user": "admin",
          "password": "random_string1",
          "https_port": "8443",
          "tcp_port_secure": "9440",
          "https_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443",
          "native_protocol": "rw.cid1.yadc.io:9440",
          "jdbc_uri": "jdbc:clickhouse://rw.cid1.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
          "odbc_uri": "https://admin:random_string1@rw.cid1.yadc.io:8443"
      },
      "private_connection_info": {
          "host": "rw.cid1.private.yadc.io",
          "user": "admin",
          "password": "random_string1",
          "https_port": "8443",
          "tcp_port_secure": "9440",
          "https_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443",
          "native_protocol": "rw.cid1.private.yadc.io:9440",
          "jdbc_uri": "jdbc:clickhouse://rw.cid1.private.yadc.io:8443/default?ssl=true&user=admin&password=random_string1",
          "odbc_uri": "https://admin:random_string1@rw.cid1.private.yadc.io:8443"
      },
      "region_id": "eu-central1",
      "network_id": "network1",
      "name": "test_cluster",
      "description": "test cluster",
      "status": "CLUSTER_STATUS_UNKNOWN",
      "version": "22.3",
      "resources": {
          "clickhouse": {
              "disk_size": "34359738368",
              "replica_count": "3",
              "resource_preset_id": "a1.aws.1",
              "shard_count": "1"
          }
      },
      "access": null
  }
  """

  Scenario: Create/Get of CH cluster with Encryption params works
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster2",
        "description": "test cluster",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "replica_count": 5
            }
        },
        "encryption": {
            "enabled": true
        }
    }
    """
    Then we get DataCloud response OK
    When "worker_task_id2" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2",
        "sensitive": false
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid2",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "host": "rw.cid2.yadc.io",
            "user": "admin",
            "password": "",
            "https_port": "8443",
            "https_uri": "https://rw.cid2.yadc.io:8443",
            "tcp_port_secure": "9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid2.yadc.io:8443/default?ssl=true",
            "native_protocol": "rw.cid2.yadc.io:9440",
            "odbc_uri": "https://rw.cid2.yadc.io:8443"
        },
        "private_connection_info": {
            "host": "rw.cid2.private.yadc.io",
            "user": "admin",
            "password": "",
            "https_port": "8443",
            "https_uri": "https://rw.cid2.private.yadc.io:8443",
            "tcp_port_secure": "9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid2.private.yadc.io:8443/default?ssl=true",
            "native_protocol": "rw.cid2.private.yadc.io:9440",
            "odbc_uri": "https://rw.cid2.private.yadc.io:8443"
        },
        "region_id": "eu-central1",
        "network_id": "network1",
        "name": "test_cluster2",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN",
        "create_time": "**IGNORE**",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "disk_size": "34359738368",
                "replica_count": "5",
                "resource_preset_id": "a1.aws.1",
                "shard_count": "1"
            }
        },
        "encryption": {
            "enabled": true
        }

    }
    """
    When we GET via PillarConfig "/v1/config/ach-ec1a-s1-1.cid2.yadc.io"
    Then we get response with status 200
    And body at path "$.data.encryption" contains
    """
    {
      "enabled": true
    }
    """
    When we update pillar for cluster "cid2" on path "{data,encryption,key}" with value '{"id": "id1", "type": "aws"}'
    When we GET via PillarConfig "/v1/config/ach-ec1a-s1-1.cid2.yadc.io"
    Then we get response with status 200
    And body at path "$.data.encryption" contains
    """
    {
      "key": {"id": "id1", "type": "aws"},
      "enabled": true
    }
    """
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2",
        "version": "22.3",
        "access": {
          "data_services": {"values":[]}
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id3"
    }
    """
    When "worker_task_id3" acquired and finished by worker
    When we GET via PillarConfig "/v1/config/ach-ec1a-s1-1.cid2.yadc.io"
    Then we get response with status 200
    And body at path "$.data.encryption" contains
    """
    {
      "key": {"id": "id1", "type": "aws"},
      "enabled": true
    }
    """

  Scenario: Stop/Start cluster works
    When we "Stop" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    { "cluster_id": "cid1"}
    """
    Then we get DataCloud response with body ignoring empty
    """
    { "status": "CLUSTER_STATUS_STOPPING" }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    { "cluster_id": "cid1"}
    """
    Then we get DataCloud response with body ignoring empty
    """
    { "status": "CLUSTER_STATUS_STOPPED" }
    """
    When we "Start" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    { "cluster_id": "cid1"}
    """
    Then we get DataCloud response with body ignoring empty
    """
    { "status": "CLUSTER_STATUS_STARTING" }
    """
    When "worker_task_id3" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    { "cluster_id": "cid1"}
    """
    Then we get DataCloud response with body ignoring empty
    """
    { "status": "CLUSTER_STATUS_UNKNOWN" }
    """

  Scenario: Create/Get of CH cluster with specified NetworkID
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster2",
        "description": "test cluster",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "replica_count": 5
            }
        },
        "encryption": {
            "enabled": true
        },
        "network_id": "some-network-id"
    }
    """
    Then we get DataCloud response OK
    When "worker_task_id2" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2",
        "sensitive": false
    }
    """
    Then we get DataCloud response with body
    """
    {
        "id": "cid2",
        "project_id": "folder1",
        "cloud_type": "aws",
        "connection_info": {
            "host": "rw.cid2.yadc.io",
            "user": "admin",
            "password": "",
            "https_port": "8443",
            "https_uri": "https://rw.cid2.yadc.io:8443",
            "tcp_port_secure": "9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid2.yadc.io:8443/default?ssl=true",
            "native_protocol": "rw.cid2.yadc.io:9440",
            "odbc_uri": "https://rw.cid2.yadc.io:8443"
        },
        "private_connection_info": {
            "host": "rw.cid2.private.yadc.io",
            "user": "admin",
            "password": "",
            "https_port": "8443",
            "https_uri": "https://rw.cid2.private.yadc.io:8443",
            "tcp_port_secure": "9440",
            "jdbc_uri": "jdbc:clickhouse://rw.cid2.private.yadc.io:8443/default?ssl=true",
            "native_protocol": "rw.cid2.private.yadc.io:9440",
            "odbc_uri": "https://rw.cid2.private.yadc.io:8443"
        },
        "region_id": "eu-central1",
        "network_id": "some-network-id",
        "name": "test_cluster2",
        "description": "test cluster",
        "status": "CLUSTER_STATUS_UNKNOWN",
        "create_time": "**IGNORE**",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "disk_size": "34359738368",
                "replica_count": "5",
                "resource_preset_id": "a1.aws.1",
                "shard_count": "1"
            }
        },
        "encryption": {
            "enabled": true
        }

    }
    """
