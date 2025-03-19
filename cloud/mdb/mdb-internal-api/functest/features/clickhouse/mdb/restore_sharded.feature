@grpc_api
Feature: Restore sharded ClickHouse cluster from backups

    Background:
        Given default headers
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "folder_id": "folder1",
            "name": "test",
            "environment": "PRESTABLE",
            "config_spec": {
                "version": "21.3",
                "clickhouse": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    },
                    "config": {
                        "geobase_uri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                        "kafka": {
                            "security_protocol": "SECURITY_PROTOCOL_SSL",
                            "sasl_mechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                            "sasl_username": "kafka_username",
                            "sasl_password": "kafka_pass"
                        },
                        "kafka_topics": [{
                            "name": "kafka_topic",
                            "settings": {
                                "security_protocol": "SECURITY_PROTOCOL_SSL",
                                "sasl_mechanism": "SASL_MECHANISM_GSSAPI",
                                "sasl_username": "topic_username",
                                "sasl_password": "topic_pass"
                            }
                        }],
                        "rabbitmq": {
                            "username": "test_user",
                            "password": "test_password"
                        }
                    }
                }
            },
            "database_specs": [{
                "name": "testdb"
            }],
            "user_specs": [{
                "name": "test",
                "password": "test_password"
            }],
            "host_specs": [{
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "ZOOKEEPER",
                "zone_id": "man"
            }, {
                "type": "ZOOKEEPER",
                "zone_id": "vla"
            },{
                "type": "ZOOKEEPER",
                "zone_id": "iva"
            }],
            "description": "test cluster"
        }
        """
        Then we get gRPC response OK
        And "worker_task_id1" acquired and finished by worker
        When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "cluster_id": "cid1",
            "shard_name": "shard2",
            "host_specs": [
                {
                    "type": "CLICKHOUSE",
                    "zone_id": "sas"
                }
            ]
        }
        """
        Then we get gRPC response OK
        Given "worker_task_id2" acquired and finished by worker
        Given s3 response
        # language=json
        """
        {
            "Contents": [
                {
                    "Key": "ch_backup/cid1/shard1/0/backup_struct.json",
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
                },
                {
                    "Key": "ch_backup/cid1/shard2/1/backup_struct.json",
                    "LastModified": 1,
                    "Body": {
                        "meta": {
                            "date_fmt": "%Y-%m-%d %H:%M:%S",
                            "start_time": "1970-01-01 00:00:10",
                            "end_time": "1970-01-01 00:00:11",
                            "labels": {
                                "shard_name": "shard2"
                            },
                            "state": "created"
                        }
                    }
                }
            ]
        }
        """
        When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
        # language=json
        """
        {
            "folder_id": "folder1"
        }
        """
        Then we get gRPC response with body
        # language=json
        """
        {
            "backups": [
                {
                    "created_at": "1970-01-01T00:00:11Z",
                    "folder_id": "folder1",
                    "id": "cid1:1",
                    "source_cluster_id": "cid1",
                    "source_shard_names": ["shard2"],
                    "started_at": "1970-01-01T00:00:10Z"
                },
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
        Given all "cid1" revs committed before "1970-01-01T00:00:00+00:00"

    @restore
    Scenario: Restoring from multiple shard backups belonging to different clusters fails
        When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "folder_id": "folder1",
            "name": "test_restored",
            "environment": "PRESTABLE",
            "config_spec": {
                "clickhouse": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418245
                    }
                }
            },
            "host_specs": [{
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }],
            "backup_id": "cid1:0",
            "additional_backup_ids": ["cid2:0"]
        }
        """
        Then we get gRPC response error with code FAILED_PRECONDITION and message "backups ["cid2:0"] does not belong to the same cluster with id "cid1""

    @restore
    Scenario: Restoring same shard backed up by different backups fails
        Given s3 response
        # language=json
        """
        {
            "Contents": [
                {
                    "Key": "ch_backup/cid1/shard1/0/backup_struct.json",
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
                },
                {
                    "Key": "ch_backup/cid1/shard1/1/backup_struct.json",
                    "LastModified": 1,
                    "Body": {
                        "meta": {
                            "date_fmt": "%Y-%m-%d %H:%M:%S",
                            "start_time": "1970-01-01 00:00:10",
                            "end_time": "1970-01-01 00:00:11",
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
        When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "folder_id": "folder1",
            "name": "test_restored",
            "environment": "PRESTABLE",
            "config_spec": {
                "clickhouse": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418245
                    }
                }
            },
            "host_specs": [{
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }],
            "backup_id": "cid1:0",
            "additional_backup_ids": ["cid1:1"]
        }
        """
        Then we get gRPC response error with code FAILED_PRECONDITION and message "two or more backups are associated to the same shard. there must be only one backup per shard. shard "shard1": backup ids: ["cid1:0", "cid1:1"]"

    @restore
    Scenario: Restoring from multiple shard backups works
        When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "folder_id": "folder1",
            "name": "test_restored",
            "environment": "PRESTABLE",
            "config_spec": {
                "clickhouse": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                }
            },
            "host_specs": [{
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }],
            "backup_id": "cid1:0",
            "additional_backup_ids": ["cid1:1"]
        }
        """
        Then we get gRPC response with body
        # language=json
        """
        {
            "created_by": "user",
            "description": "Create new ClickHouse cluster from the backup",
            "done": false,
            "id": "worker_task_id3",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.RestoreClusterMetadata",
                "backup_id": "cid1:1,cid1:0",
                "cluster_id": "cid2"
            }
        }
        """
        Given "worker_task_id3" acquired and finished by worker
        When we "ListShards" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "cluster_id": "cid2"
        }
        """
        Then we get gRPC response with body ignoring empty
        # language=json
        """
        {
            "shards": [{
                "cluster_id": "cid2",
                "name": "shard1",
                "config": {
                    "clickhouse": {
                        "config": "**IGNORE**",
                        "resources": {
                            "disk_size": "10737418240",
                            "disk_type_id": "local-ssd",
                            "resource_preset_id": "s1.porto.1"
                        },
                        "weight": "100"
                    }
                }
            }, {
                "cluster_id": "cid2",
                "name": "shard2",
                "config": {
                    "clickhouse": {
                        "config": "**IGNORE**",
                        "resources": {
                            "disk_size": "10737418240",
                            "disk_type_id": "local-ssd",
                            "resource_preset_id": "s1.porto.1"
                        },
                        "weight": "100"
                    }
                }
            }]
        }
        """

    @restore @shard_groups
    Scenario: Restoring from multiple shard backups with shard groups works
        When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "cluster_id": "cid1",
            "shard_group_name": "test_group",
            "description": "Test shard group",
            "shard_names": ["shard1", "shard2"]
        }
        """
        Given "worker_task_id3" acquired and finished by worker
        Given all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
        When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "folder_id": "folder1",
            "name": "test_restored",
            "environment": "PRESTABLE",
            "config_spec": {
                "clickhouse": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                }
            },
            "host_specs": [{
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }],
            "backup_id": "cid1:0",
            "additional_backup_ids": ["cid1:1"]
        }
        """
        Then we get gRPC response with body
        # language=json
        """
        {
            "created_by": "user",
            "description": "Create new ClickHouse cluster from the backup",
            "done": false,
            "id": "worker_task_id4",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.RestoreClusterMetadata",
                "backup_id": "cid1:1,cid1:0",
                "cluster_id": "cid2"
            }
        }
        """
        Given "worker_task_id4" acquired and finished by worker
        When we "ListShardGroups" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "cluster_id": "cid2",
            "page_size": 1
        }
        """
        Then we get gRPC response with body
        # language=json
        """
        {
            "shard_groups": [
                {
                    "cluster_id": "cid2",
                    "name": "test_group",
                    "description": "Test shard group",
                    "shard_names": ["shard1", "shard2"]
                }
            ],
            "next_page_token": ""
        }
        """

    @restore @shard_groups
    Scenario: Attempt to restore from incompatible shard backups fails
      When we "DeleteShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "cluster_id": "cid1",
            "shard_name": "shard2"
        }
        """
        Then we get gRPC response OK
        Given "worker_task_id3" acquired and finished by worker
        When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "cluster_id": "cid1",
            "shard_name": "shard3",
            "host_specs": [
                {
                    "type": "CLICKHOUSE",
                    "zone_id": "sas"
                }
            ]
        }
        """
        Then we get gRPC response OK
        Given "worker_task_id4" acquired and finished by worker
        Given s3 response
        # language=json
        """
        {
            "Contents": [
                {
                    "Key": "ch_backup/cid1/shard1/0/backup_struct.json",
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
                },
                {
                    "Key": "ch_backup/cid1/shard2/1/backup_struct.json",
                    "LastModified": 1,
                    "Body": {
                        "meta": {
                            "date_fmt": "%Y-%m-%d %H:%M:%S",
                            "start_time": "1970-01-01 00:00:10",
                            "end_time": "1970-01-01 00:00:11",
                            "labels": {
                                "shard_name": "shard2"
                            },
                            "state": "created"
                        }
                    }
                },
                {
                    "Key": "ch_backup/cid1/shard3/2/backup_struct.json",
                    "LastModified": 1,
                    "Body": {
                        "meta": {
                            "date_fmt": "%Y-%m-%d %H:%M:%S",
                            "start_time": "1970-01-01 00:00:20",
                            "end_time": "1970-01-01 00:00:21",
                            "labels": {
                                "shard_name": "shard3"
                            },
                            "state": "created"
                        }
                    }
                }
            ]
        }
        """
        When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.BackupService" with data
        # language=json
        """
        {
            "folder_id": "folder1"
        }
        """
        Then we get gRPC response with body
        # language=json
        """
        {
            "backups": [
                {
                    "created_at": "1970-01-01T00:00:21Z",
                    "folder_id": "folder1",
                    "id": "cid1:2",
                    "source_cluster_id": "cid1",
                    "source_shard_names": ["shard3"],
                    "started_at": "1970-01-01T00:00:20Z"
                },
                {
                    "created_at": "1970-01-01T00:00:11Z",
                    "folder_id": "folder1",
                    "id": "cid1:1",
                    "source_cluster_id": "cid1",
                    "source_shard_names": ["shard2"],
                    "started_at": "1970-01-01T00:00:10Z"
                },
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
        Given all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
        When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "folder_id": "folder1",
            "name": "test_restored",
            "environment": "PRESTABLE",
            "config_spec": {
                "clickhouse": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                }
            },
            "host_specs": [{
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }],
            "backup_id": "cid1:1",
            "additional_backup_ids": ["cid1:2"]
        }
        """
        Then we get gRPC response error with code FAILED_PRECONDITION and message "backups are incompatible. backup "cid1:1" contains data for shard "shard2" but this shard didn't exist at the time of backup "cid1:2" creation"
