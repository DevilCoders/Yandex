@sqlserver
@grpc_api
Feature: Create Compute Microsoft SQLServer Cluster

  Background:
    Given default headers
    And health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "Alive"
           },
           {
               "cid": "cid2",
               "status": "Alive"
           }
       ],
       "hosts": [
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                    {
                        "name": "sqlserver",
                        "role": "Master",
                        "status": "Alive",
                        "timestamp": 1600860350
                    }
                ]
           },
           {
               "fqdn": "myt-2.db.yandex.net",
               "cid": "cid2",
               "status": "Alive",
               "services": [
                    {
                        "name": "sqlserver",
                        "role": "Master",
                        "status": "Alive",
                        "timestamp": 1600860350
                    }
                ]
           }
       ]
    }
    """
    When we add default feature flag "MDB_SQLSERVER_CLUSTER"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "service_account_id": "sa1",
      "labels": {
        "foo": "bar"
      },
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
          "maxDegreeOfParallelism": 30,
          "auditLevel": 3
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateClusterMetadata",
        "cluster_id": "cid1"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: Cluster creation and delete works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "config": {
        "access": {
          "data_lens": false,
          "data_transfer": false,
          "web_sql": false,
          "serverless": false
        },
        "backup_window_start": {
          "hours": 22,
          "minutes": 15,
          "seconds": 30,
          "nanos": 100
        },
        "secondary_connections": "SECONDARY_CONNECTIONS_OFF",
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
          "default_config": {
            "audit_level":                    "0",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "0",
            "optimize_for_ad_hoc_workloads":  false
          },
          "effective_config": {
            "audit_level":                    "3",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "30",
            "optimize_for_ad_hoc_workloads":  false
          },
          "user_config": {
            "cost_threshold_for_parallelism": null,
            "fill_factor_percent":            null,
            "optimize_for_ad_hoc_workloads":  null,
            "max_degree_of_parallelism": "30",
            "audit_level": "3"
          }
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": "10737418240"
        }
      },
      "description": "test cluster",
      "labels": {
        "foo": "bar"
      },
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "health": "ALIVE",
      "host_group_ids": [],
      "network_id": "network1",
      "security_group_ids": [],
      "service_account_id": "sa1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "monitoring": [
        {
          "name": "YASM",
          "description": "YaSM (Golovan) charts",
          "link": "https://yasm/cid=cid1"
        },
        {
          "name": "Solomon",
          "description": "Solomon charts",
          "link": "https://solomon/cid=cid1&fExtID=folder1"
        },
        {
          "name": "Console",
          "description": "Console charts",
          "link": "https://console/cid=cid1&fExtID=folder1"
        }
      ]
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.DeleteClusterMetadata",
        "cluster_id": "cid1"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """

  Scenario: Cluster create with default config works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "2016sp2std",
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateClusterMetadata",
        "cluster_id": "cid2"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid2"
    }
    """
    Then we get gRPC response with body
    """
    {
      "config": {
        "access": {
          "data_lens": false,
          "data_transfer": false,
          "web_sql": false,
          "serverless": false
        },
        "backup_window_start": {
          "hours": 22,
          "minutes": 15,
          "seconds": 30,
          "nanos": 100
        },
        "secondary_connections": "SECONDARY_CONNECTIONS_OFF",
        "version": "2016sp2std",
        "sqlserver_config_2016sp2std": {
          "default_config": {
            "audit_level":                    "0",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "0",
            "optimize_for_ad_hoc_workloads":  false
          },
          "effective_config": {
            "audit_level":                    "0",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "0",
            "optimize_for_ad_hoc_workloads":  false
          },
          "user_config": {
            "cost_threshold_for_parallelism": null,
            "fill_factor_percent":            null,
            "optimize_for_ad_hoc_workloads":  null,
            "max_degree_of_parallelism":      null,
            "audit_level":                    null
          }
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": "10737418240"
        }
      },
      "description": "test cluster 2",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid2",
      "name": "test2",
      "health": "ALIVE",
      "host_group_ids": [],
      "network_id": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "monitoring": [
        {
          "name": "YASM",
          "description": "YaSM (Golovan) charts",
          "link": "https://yasm/cid=cid2"
        },
        {
          "name": "Solomon",
          "description": "Solomon charts",
          "link": "https://solomon/cid=cid2&fExtID=folder1"
        },
        {
          "name": "Console",
          "description": "Console charts",
          "link": "https://console/cid=cid2&fExtID=folder1"
        }
      ]
    }
    """

  Scenario: Cluster create with invalid name fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "тушкан-1",
      "description": "test cluster",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "cluster name "тушкан-1" has invalid symbols"

  Scenario: Cluster create without resources preset fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "resource preset must be specified"

  Scenario: Cluster create without disk type fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "disk type must be specified"

  Scenario: Cluster create without disk size fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd"
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "disk size must be specified"

  Scenario: Cluster create without version fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "version must be specified"

  Scenario: Cluster create with unknown version fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "2037sp2",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "unknown version: 2037sp2"

  Scenario: Cluster listing works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "another_test",
      "description": "another test cluster",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
          "optimize_for_ad_hoc_workloads": true
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "page_size": 100
    }
    """
    Then we get gRPC response with body
    """
    {
        "clusters": [
            {
                "config": {
                  "access": {
                    "data_lens": false,
                    "data_transfer": false,
                    "web_sql": false,
                    "serverless": false
                  },
                  "backup_window_start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                  },
                  "secondary_connections": "SECONDARY_CONNECTIONS_OFF",
                  "version": "2016sp2ent",
                  "sqlserver_config_2016sp2ent": {
                    "default_config": {
                      "audit_level":                    "0",
                      "cost_threshold_for_parallelism": "5",
                      "fill_factor_percent":            "0",
                      "max_degree_of_parallelism":      "0",
                      "optimize_for_ad_hoc_workloads":  false
                    },
                    "effective_config": {
                      "audit_level":                    "0",
                      "cost_threshold_for_parallelism": "5",
                      "fill_factor_percent":            "0",
                      "max_degree_of_parallelism":      "0",
                      "optimize_for_ad_hoc_workloads":  true
                    },
                    "user_config": {
                      "cost_threshold_for_parallelism": null,
                      "fill_factor_percent":            null,
                      "optimize_for_ad_hoc_workloads":  true,
                      "max_degree_of_parallelism": null,
                      "audit_level": null
                    }
                  },
                  "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": "10737418240"
                  }
                },
                "created_at":  "**IGNORE**",
                "description": "another test cluster",
                "deletion_protection": false,
                "environment": "PRESTABLE",
                "folder_id":   "folder1",
                "health":      "ALIVE",
                "host_group_ids": [],
                "id":          "cid2",
                "labels":      {},
                "monitoring":  [],
                "name":        "another_test",
                "network_id":  "network1",
                "security_group_ids": [],
                "service_account_id": "",
                "sqlcollation": "Cyrillic_General_CI_AI",
                "status":      "RUNNING"
            },
            {
                "config": {
                  "access": {
                    "data_lens": false,
                    "data_transfer": false,
                    "web_sql": false,
                    "serverless": false
                  },
                  "backup_window_start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                  },
                  "secondary_connections": "SECONDARY_CONNECTIONS_OFF",
                  "version": "2016sp2ent",
                  "sqlserver_config_2016sp2ent": {
                    "default_config": {
                      "audit_level":                    "0",
                      "cost_threshold_for_parallelism": "5",
                      "fill_factor_percent":            "0",
                      "max_degree_of_parallelism":      "0",
                      "optimize_for_ad_hoc_workloads":  false
                    },
                    "effective_config": {
                      "audit_level":                    "3",
                      "cost_threshold_for_parallelism": "5",
                      "fill_factor_percent":            "0",
                      "max_degree_of_parallelism":      "30",
                      "optimize_for_ad_hoc_workloads":  false
                    },
                    "user_config": {
                      "cost_threshold_for_parallelism": null,
                      "fill_factor_percent":            null,
                      "optimize_for_ad_hoc_workloads":  null,
                      "max_degree_of_parallelism": "30",
                      "audit_level": "3"
                    }
                  },
                  "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": "10737418240"
                  }
                },
                "created_at":  "**IGNORE**",
                "description": "test cluster",
                "deletion_protection": false,
                "environment": "PRESTABLE",
                "folder_id":   "folder1",
                "health":      "ALIVE",
                "host_group_ids": [],
                "id":          "cid1",
                "labels": {
                  "foo": "bar"
                },
                "monitoring":  [],
                "name":        "test",
                "network_id":  "network1",
                "security_group_ids": [],
                "service_account_id": "sa1",
                "sqlcollation": "Cyrillic_General_CI_AI",
                "status":      "RUNNING"
            }
        ],
        "next_page_token": ""
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
        "folder_id": "folder2",
        "page_size": 100
    }
    """
    Then we get gRPC response with body
    """
    {
        "clusters": []
    }
    """

  Scenario: Host list works
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "SQLSERVER"
                    }
                ],
                "role": "MASTER",
                "subnet_id": "network1-myt",
                "zone_id": "myt",
                "system": null
            }
        ]
    }
    """

  Scenario: Operation list works
    When we run query
    """
    UPDATE dbaas.worker_queue
    SET create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00',
        start_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00',
        end_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    WHERE task_id = 'worker_task_id1'
    """
    And we "ListOperations" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "operations": [
        {
          "id": "worker_task_id1",
          "description": "Create Microsoft SQLServer cluster",
          "created_by": "user",
          "created_at": "2000-01-01T00:00:00Z",
          "modified_at": "2000-01-01T00:00:00Z",
          "done": true,
          "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
          },
          "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
          }
        }
      ]
    }
    """

  Scenario Outline: Cluster health deduction works
    Given health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "<status>"
           }
       ],
       "hosts": [
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "<status>",
               "services": [
                   {
                       "name": "sqlserver",
                       "role": "Unknown",
                       "status": "<sqlserver-1>"
                   }
               ]
           }
       ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "health": "<health>"
    }
    """
    Examples:
        | sqlserver-1  | status   | health   |
        | Alive        | Alive    | ALIVE    |
        | Alive        | Degraded | DEGRADED |
        | Alive        | Degraded | DEGRADED |
        | Alive        | Dead     | DEAD     |
        | Dead         | Degraded | DEGRADED |
        | Dead         | Dead     | DEAD     |



  Scenario: Cluster stop and start works
    When we "Stop" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Stop Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.StopClusterMetadata",
        "cluster_id": "cid1"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "description": "test cluster",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1",
      "status": "STOPPED"
    }
    """
    When we "Start" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Start Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.StartClusterMetadata",
        "cluster_id": "cid1"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "description": "test cluster",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1",
      "status": "RUNNING"
    }
    """

  Scenario: Standard edition cluster create with many hosts fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "cluster1",
      "description": "test cluster",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "2016sp2std",
        "sqlserver_config_2016sp2std": {
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }, {
        "zoneId": "sas"
      }, {
        "zoneId": "vla"
      }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "only 1-node clusters are supported for Standard edition now"

  Scenario: Cluster create with invalid service account fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "testwrongcollation",
      "description": "testwrongcollation",
      "networkId": "network1",
      "service_account_id": "unknown_sa",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "testwrongcollation",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response error with code PERMISSION_DENIED and message "you do not have permission to access the requested service account or service account does not exist"

  Scenario: Cluster create with invalid sqlcollation fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "testwrongcollation",
      "description": "testwrongcollation",
      "networkId": "network1",
      "sqlcollation": "UTF-16",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "testwrongcollation",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "SQLCollation UTF-16 is not valid"

  Scenario: Cluster create with no sqlcollation specified works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test_emptycollation",
      "description": "test cluster emptycollation",
      "networkId": "network1",
      "config_spec": {
        "version": "2016sp2std",
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateClusterMetadata",
        "cluster_id": "cid2"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid2"
    }
    """
    Then we get gRPC response with body
    """
    {
      "config": {
        "access": {
          "data_lens": false,
          "data_transfer": false,
          "web_sql": false,
          "serverless": false
        },
        "backup_window_start": {
          "hours": 22,
          "minutes": 15,
          "seconds": 30,
          "nanos": 100
        },
        "secondary_connections": "SECONDARY_CONNECTIONS_OFF",
        "version": "2016sp2std",
        "sqlserver_config_2016sp2std": {
          "default_config": {
            "audit_level":                    "0",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "0",
            "optimize_for_ad_hoc_workloads":  false
          },
          "effective_config": {
            "audit_level":                    "0",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "0",
            "optimize_for_ad_hoc_workloads":  false
          },
          "user_config": {
            "cost_threshold_for_parallelism": null,
            "fill_factor_percent":            null,
            "optimize_for_ad_hoc_workloads":  null,
            "max_degree_of_parallelism":      null,
            "audit_level":                    null
          }
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": "10737418240"
        }
      },
      "description": "test cluster emptycollation",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid2",
      "name": "test_emptycollation",
      "health": "ALIVE",
      "host_group_ids": [],
      "network_id": "network1",
      "sqlcollation": "Cyrillic_General_CI_AS",
      "monitoring": [
        {
          "name": "YASM",
          "description": "YaSM (Golovan) charts",
          "link": "https://yasm/cid=cid2"
        },
        {
          "name": "Solomon",
          "description": "Solomon charts",
          "link": "https://solomon/cid=cid2&fExtID=folder1"
        },
        {
          "name": "Console",
          "description": "Console charts",
          "link": "https://console/cid=cid2&fExtID=folder1"
        }
      ]
    }
    """

  Scenario: Cluster create with readable replicas works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test_unreadable_replicas",
      "description": "test cluster unreadable_replicas",
      "networkId": "network1",
      "config_spec": {
        "version": "2016sp2std",
        "secondary_connections": "SECONDARY_CONNECTIONS_READ_ONLY",
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateClusterMetadata",
        "cluster_id": "cid2"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid2"
    }
    """
    Then we get gRPC response with body
    """
    {
      "config": {
        "access": {
          "data_lens": false,
          "data_transfer": false,
          "web_sql": false,
          "serverless": false
        },
        "backup_window_start": {
          "hours": 22,
          "minutes": 15,
          "seconds": 30,
          "nanos": 100
        },
        "secondary_connections": "SECONDARY_CONNECTIONS_READ_ONLY",
        "version": "2016sp2std",
        "sqlserver_config_2016sp2std": {
          "default_config": {
            "audit_level":                    "0",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "0",
            "optimize_for_ad_hoc_workloads":  false
          },
          "effective_config": {
            "audit_level":                    "0",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "0",
            "optimize_for_ad_hoc_workloads":  false
          },
          "user_config": {
            "cost_threshold_for_parallelism": null,
            "fill_factor_percent":            null,
            "optimize_for_ad_hoc_workloads":  null,
            "max_degree_of_parallelism":      null,
            "audit_level":                    null
          }
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": "10737418240"
        }
      },
      "description": "test cluster unreadable_replicas",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid2",
      "name": "test_unreadable_replicas",
      "health": "ALIVE",
      "host_group_ids": [],
      "network_id": "network1",
      "sqlcollation": "Cyrillic_General_CI_AS",
      "monitoring": [
        {
          "name": "YASM",
          "description": "YaSM (Golovan) charts",
          "link": "https://yasm/cid=cid2"
        },
        {
          "name": "Solomon",
          "description": "Solomon charts",
          "link": "https://solomon/cid=cid2&fExtID=folder1"
        },
        {
          "name": "Console",
          "description": "Console charts",
          "link": "https://console/cid=cid2&fExtID=folder1"
        }
      ]
    }
    """


  Scenario Outline: Cluster create with different versions works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "<version>",
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateClusterMetadata",
        "cluster_id": "cid2"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid2"
    }
    """
    Then we get gRPC response with body
    """
    {
      "config": {
        "access": {
          "data_lens": false,
          "data_transfer": false,
          "web_sql": false,
          "serverless": false
        },
        "backup_window_start": {
          "hours": 22,
          "minutes": 15,
          "seconds": 30,
          "nanos": 100
        },
        "secondary_connections": "SECONDARY_CONNECTIONS_OFF",
        "version": "<version>",
        "sqlserver_config_<version>": {
          "default_config": {
            "audit_level":                    "0",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "0",
            "optimize_for_ad_hoc_workloads":  false
          },
          "effective_config": {
            "audit_level":                    "0",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "0",
            "optimize_for_ad_hoc_workloads":  false
          },
          "user_config": {
            "cost_threshold_for_parallelism": null,
            "fill_factor_percent":            null,
            "optimize_for_ad_hoc_workloads":  null,
            "max_degree_of_parallelism":      null,
            "audit_level":                    null
          }
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": "10737418240"
        }
      },
      "description": "test cluster 2",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid2",
      "name": "test2",
      "health": "ALIVE",
      "host_group_ids": [],
      "network_id": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "monitoring": [
        {
          "name": "YASM",
          "description": "YaSM (Golovan) charts",
          "link": "https://yasm/cid=cid2"
        },
        {
          "name": "Solomon",
          "description": "Solomon charts",
          "link": "https://solomon/cid=cid2&fExtID=folder1"
        },
        {
          "name": "Console",
          "description": "Console charts",
          "link": "https://console/cid=cid2&fExtID=folder1"
        }
      ]
    }
    """
    Examples:
        | version    |
        | 2016sp2std |
        | 2016sp2ent |
        | 2017std    |
        | 2017ent    |
        | 2019std    |
        | 2019ent    |


  Scenario Outline: Create cluster of dev versions does not work because of product id
    Given default headers
    When we add default feature flag "MDB_SQLSERVER_ALLOW_17_19"
    When we add default feature flag "MDB_SQLSERVER_ALLOW_DEV"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test_<version>",
      "description": "test cluster <version>",
      "networkId": "network1",
      "config_spec": {
        "version": "<version>",
        "backup_window_start": {
           "hours": 11,
           "minutes": 11,
           "seconds": 9
        },
        "sqlserver_config_<version>": {
          "maxDegreeOfParallelism": 30,
          "auditLevel": 2
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response error with code UNKNOWN and message "unknown ProductID for version <version>"
    Examples:
    | version     |
    | 2016sp2dev  |
    | 2017dev     |
    | 2019dev     |


  Scenario Outline: Create two node cluster works
    Given default headers
    And health response
    """
    {
       "clusters": [
           {
               "cid": "cid2",
               "status": "Alive"
           }
       ],
       "hosts": [
           {
               "fqdn": "myt-2.db.yandex.net",
               "cid": "cid2",
               "status": "Alive",
               "services": [
                    {
                        "name": "sqlserver",
                        "role": "Master",
                        "status": "Alive",
                        "timestamp": 1600860350
                    }
                ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid2",
               "status": "Alive",
               "services": [
                    {
                        "name": "sqlserver",
                        "role": "Replica",
                        "status": "Alive",
                        "timestamp": 1600860350
                    }
                ]
           },
           {
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid2",
               "status": "Alive",
               "services": [
                    {
                        "name": "witness",
                        "role": "Unknown",
                        "status": "Alive",
                        "timestamp": 1600860350
                    }
                ]
           }
       ]
    }
    """
    When we add default feature flag "MDB_SQLSERVER_TWO_NODE_CLUSTER"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test_two_<version>",
      "description": "test_two_<version>",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "<version>",
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }, {
        "zoneId": "sas"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateClusterMetadata",
        "cluster_id": "cid2"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid2"
    }
    """
    Then we get gRPC response with body
    """
    {
      "config": {
        "access": {
          "data_lens": false,
          "data_transfer": false,
          "web_sql": false,
          "serverless": false
        },
        "backup_window_start": {
          "hours": 22,
          "minutes": 15,
          "seconds": 30,
          "nanos": 100
        },
        "secondary_connections": "SECONDARY_CONNECTIONS_OFF",
        "version": "<version>",
        "sqlserver_config_<version>": {
          "default_config": {
            "audit_level":                    "0",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "0",
            "optimize_for_ad_hoc_workloads":  false
          },
          "effective_config": {
            "audit_level":                    "0",
            "cost_threshold_for_parallelism": "5",
            "fill_factor_percent":            "0",
            "max_degree_of_parallelism":      "0",
            "optimize_for_ad_hoc_workloads":  false
          },
          "user_config": {
            "cost_threshold_for_parallelism": null,
            "fill_factor_percent":            null,
            "optimize_for_ad_hoc_workloads":  null,
            "max_degree_of_parallelism":      null,
            "audit_level":                    null
          }
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": "10737418240"
        }
      },
      "description": "test_two_<version>",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid2",
      "name": "test_two_<version>",
      "health": "ALIVE",
      "host_group_ids": [],
      "network_id": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "monitoring": [
        {
          "name": "YASM",
          "description": "YaSM (Golovan) charts",
          "link": "https://yasm/cid=cid2"
        },
        {
          "name": "Solomon",
          "description": "Solomon charts",
          "link": "https://solomon/cid=cid2&fExtID=folder1"
        },
        {
          "name": "Console",
          "description": "Console charts",
          "link": "https://console/cid=cid2&fExtID=folder1"
        }
      ]
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [
            {
                "assign_public_ip": false,
                "cluster_id": "cid2",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "WITNESS"
                    }
                ],
                "role": "ROLE_UNKNOWN",
                "subnet_id": "",
                "zone_id": "iva",
                "system": null
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid2",
                "health": "ALIVE",
                "name": "myt-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "SQLSERVER"
                    }
                ],
                "role": "MASTER",
                "subnet_id": "network1-myt",
                "zone_id": "myt",
                "system": null
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid2",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "SQLSERVER"
                    }
                ],
                "role": "REPLICA",
                "subnet_id": "network1-sas",
                "zone_id": "sas",
                "system": null
            }
        ]
    }
    """
    Examples:
    | version |
    | 2019std |
    | 2019ent |

  Scenario: Cluster create fails if license check returns error
    And we disallow create sqlserver clusters
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test_ent",
      "description": "test_ent",
      "networkId": "network1",
      "sqlcollation": "Cyrillic_General_CI_AI",
      "config_spec": {
        "version": "2016sp2ent",
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }, {
        "zoneId": "sas"
      }]
    }
    """
    Then we get gRPC response error with code PERMISSION_DENIED and message "some license check error"
    And we allow create sqlserver clusters


