@grpc_api
Feature: Create/Modify ClickHouse shards

  Background:
    Given default headers
    When we "Create" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "new_name",
        "description": "test cluster",
        "version": "22.5",
        "resources": {
            "clickhouse": {
                "resource_preset_id": "a1.aws.3",
                "disk_size": 34359738368,
                "replica_count": 3,
                "shard_count": 2
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

  Scenario: Create sharded ClickHouse works
    When we "ListHosts" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "paging": {
            "page_size": 3
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "hosts": [
            {
                "cluster_id": "cid1",
                "name": "ach-ec1a-s1-1.cid1.yadc.io",
                "private_name": "ach-ec1a-s1-1.cid1.private.yadc.io",
                "shard_name": "s1"
            }, {
                "cluster_id": "cid1",
                "name": "ach-ec1a-s2-1.cid1.yadc.io",
                "private_name": "ach-ec1a-s2-1.cid1.private.yadc.io",
                "shard_name": "s2"
            }, {
                "cluster_id": "cid1",
                "name": "ach-ec1b-s1-2.cid1.yadc.io",
                "private_name": "ach-ec1b-s1-2.cid1.private.yadc.io",
                "shard_name": "s1"
            }
        ],
        "next_page": {
            "token": "eyJPZmZzZXQiOjN9"
        }
    }
    """
    When we "ListHosts" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "paging": {
            "page_token": "eyJPZmZzZXQiOjN9"
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "hosts": [
            {
                "cluster_id": "cid1",
                "name": "ach-ec1b-s2-2.cid1.yadc.io",
                "private_name": "ach-ec1b-s2-2.cid1.private.yadc.io",
                "shard_name": "s2"
            },
            {
                "cluster_id": "cid1",
                "name": "ach-ec1c-s1-3.cid1.yadc.io",
                "private_name": "ach-ec1c-s1-3.cid1.private.yadc.io",
                "shard_name": "s1"
            },
            {
                "cluster_id": "cid1",
                "name": "ach-ec1c-s2-3.cid1.yadc.io",
                "private_name": "ach-ec1c-s2-3.cid1.private.yadc.io",
                "shard_name": "s2"
            }
        ],
        "next_page": {
            "token": ""
        }
    }
    """

  Scenario: Add new ClickHouse shards works
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
                "shard_count": 3
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
        "region_id": "eu-central1",
        "name": "new_name",
        "description": "new cluster description",
        "status": "CLUSTER_STATUS_UNKNOWN"
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
                "shard_id1": { "weight": 100 },
                "shard_id2": { "weight": 100 },
                "shard_id3": { "weight": 100 }
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
            },
            "sql_database_management": true
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
                        },
                        "shard_id2": {
                            "name": "s2",
                            "hosts": {
                                "ach-ec1a-s2-1.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s2-2.cid1.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1c-s2-3.cid1.yadc.io": {"geo": "eu-central-1c"}
                            }
                        },
                        "shard_id3": {
                            "name": "s3",
                            "hosts": {
                                "ach-ec1a-s3-1.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s3-2.cid1.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1c-s3-3.cid1.yadc.io": {"geo": "eu-central-1c"}
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
            "ach-ec1a-s2-1.cid1.yadc.io",
            "ach-ec1a-s3-1.cid1.yadc.io",
            "ach-ec1b-s1-2.cid1.yadc.io",
            "ach-ec1b-s2-2.cid1.yadc.io",
            "ach-ec1b-s3-2.cid1.yadc.io",
            "ach-ec1c-s1-3.cid1.yadc.io",
            "ach-ec1c-s2-3.cid1.yadc.io",
            "ach-ec1c-s3-3.cid1.yadc.io"
        ],
        "subcluster_id": "subcid1",
        "subcluster_name": "clickhouse_subcluster",
        "assign_public_ip": true,
        "on_dedicated_host": false
    }
    """

  Scenario: Add new ClickHouse shards with increased replica count works
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
                "shard_count": 3,
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
        "region_id": "eu-central1",
        "name": "new_name",
        "description": "new cluster description",
        "status": "CLUSTER_STATUS_UNKNOWN"
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
                "shard_id1": { "weight": 100 },
                "shard_id2": { "weight": 100 },
                "shard_id3": { "weight": 100 }
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
            },
            "sql_database_management": true
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
                                "ach-ec1c-s1-3.cid1.yadc.io": {"geo": "eu-central-1c"},
                                "ach-ec1a-s1-4.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s1-5.cid1.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1c-s1-6.cid1.yadc.io": {"geo": "eu-central-1c"}
                            }
                        },
                        "shard_id2": {
                            "name": "s2",
                            "hosts": {
                                "ach-ec1a-s2-1.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s2-2.cid1.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1c-s2-3.cid1.yadc.io": {"geo": "eu-central-1c"},
                                "ach-ec1a-s2-4.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s2-5.cid1.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1c-s2-6.cid1.yadc.io": {"geo": "eu-central-1c"}
                            }
                        },
                        "shard_id3": {
                            "name": "s3",
                            "hosts": {
                                "ach-ec1a-s3-1.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1a-s3-4.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s3-2.cid1.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1b-s3-5.cid1.yadc.io": {"geo": "eu-central-1b"},
                                "ach-ec1c-s3-3.cid1.yadc.io": {"geo": "eu-central-1c"},
                                "ach-ec1c-s3-6.cid1.yadc.io": {"geo": "eu-central-1c"}
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
            "ach-ec1a-s2-1.cid1.yadc.io",
            "ach-ec1a-s2-4.cid1.yadc.io",
            "ach-ec1a-s3-1.cid1.yadc.io",
            "ach-ec1a-s3-4.cid1.yadc.io",
            "ach-ec1b-s1-2.cid1.yadc.io",
            "ach-ec1b-s1-5.cid1.yadc.io",
            "ach-ec1b-s2-2.cid1.yadc.io",
            "ach-ec1b-s2-5.cid1.yadc.io",
            "ach-ec1b-s3-2.cid1.yadc.io",
            "ach-ec1b-s3-5.cid1.yadc.io",
            "ach-ec1c-s1-3.cid1.yadc.io",
            "ach-ec1c-s1-6.cid1.yadc.io",
            "ach-ec1c-s2-3.cid1.yadc.io",
            "ach-ec1c-s2-6.cid1.yadc.io",
            "ach-ec1c-s3-3.cid1.yadc.io",
            "ach-ec1c-s3-6.cid1.yadc.io"
        ],
        "subcluster_id": "subcid1",
        "subcluster_name": "clickhouse_subcluster",
        "assign_public_ip": true,
        "on_dedicated_host": false
    }
    """

  Scenario: Delete ClickHouse shards works
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "resources": {
            "clickhouse": {
                "shard_count": 1
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
                "shard_id1": { "weight": 100 }
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
            },
            "sql_database_management": true
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
        "space_limit": 34359738368,
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

  Scenario: Delete ClickHouse shards with new replicas works
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "resources": {
            "clickhouse": {
                "shard_count": 1,
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
                "shard_id1": { "weight": 100 }
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
            },
            "sql_database_management": true
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
        "space_limit": 34359738368,
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

  Scenario: Delete All CLickHouse shards fails
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "resources": {
            "clickhouse": {
                "shard_count": 0
            }
        }
    }
    """
    Then we get DataCloud response error with code INVALID_ARGUMENT and message "invalid shard count '0', cluster must have at least one shard"

  Scenario: Create ClickHouse shards and delete replicas works
    When we "Update" via DataCloud at "datacloud.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "resources": {
            "clickhouse": {
                "shard_count": 3,
                "replica_count": 2
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
                "shard_id1": { "weight": 100 },
                "shard_id2": { "weight": 100 },
                "shard_id3": { "weight": 100 }
            },
            "keeper_hosts": {
                "ach-ec1a-s1-1.cid1.yadc.io": 1,
                "ach-ec1b-s1-2.cid1.yadc.io": 2,
                "ach-ec1b-s2-2.cid1.yadc.io": 4
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
            },
            "sql_database_management": true
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
                                "ach-ec1b-s1-2.cid1.yadc.io": {"geo": "eu-central-1b"}
                            }
                        },
                        "shard_id2": {
                            "name": "s2",
                            "hosts": {
                                "ach-ec1a-s2-1.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s2-2.cid1.yadc.io": {"geo": "eu-central-1b"}
                            }
                        },
                        "shard_id3": {
                            "name": "s3",
                            "hosts": {
                                "ach-ec1a-s3-1.cid1.yadc.io": {"geo": "eu-central-1a"},
                                "ach-ec1b-s3-2.cid1.yadc.io": {"geo": "eu-central-1b"}
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
            "ach-ec1b-s1-2.cid1.yadc.io"
        ],
        "space_limit": 34359738368,
        "cluster_name": "new_name",
        "cluster_type": "clickhouse_cluster",
        "disk_type_id": "local-ssd",
        "cluster_hosts": [
            "ach-ec1a-s1-1.cid1.yadc.io",
            "ach-ec1a-s2-1.cid1.yadc.io",
            "ach-ec1a-s3-1.cid1.yadc.io",
            "ach-ec1b-s1-2.cid1.yadc.io",
            "ach-ec1b-s2-2.cid1.yadc.io",
            "ach-ec1b-s3-2.cid1.yadc.io"
        ],
        "subcluster_id": "subcid1",
        "subcluster_name": "clickhouse_subcluster",
        "assign_public_ip": true,
        "on_dedicated_host": false
    }
    """
