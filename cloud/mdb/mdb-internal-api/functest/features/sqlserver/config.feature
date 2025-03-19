@sqlserver
@grpc_api
Feature: Config Microsoft SQLServer Cluster

    Background:
      Given default headers
      When we add default feature flag "MDB_SQLSERVER_CLUSTER"
      When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "folderId": "folder1",
        "environment": "PRESTABLE",
        "name": "test",
        "description": "test cluster",
        "networkId": "network1",
        "config_spec": {
         "access": {
            "data_lens": true,
            "data_transfer": true,
            "web_sql": false,
            "serverless": false
          },
          "version": "2016sp2ent",
          "backup_window_start": {
             "hours": 11,
             "minutes": 11,
             "seconds": 9
          },
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

    Scenario: Cluster get works
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
            "data_lens": true,
            "data_transfer": true,
            "web_sql": false,
            "serverless": false
          },
          "backup_window_start": {
                "hours": 11,
                "minutes": 11,
                "seconds": 9,
                "nanos": 0
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
        "environment": "PRESTABLE",
        "folder_id": "folder1",
        "id": "cid1",
        "name": "test",
        "network_id": "network1"
      }
      """

    Scenario: All settings in config could be modified and be reset to default values
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {
            "maxDegreeOfParallelism": 10,
            "costThresholdForParallelism": 666,
            "auditLevel": 2,
            "fillFactorPercent": 51,
            "optimizeForAdHocWorkloads": true
           },
          "access": {
            "data_lens": false,
            "data_transfer": false
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.sqlserver_config_2016sp2ent.max_degree_of_parallelism",
            "config_spec.sqlserver_config_2016sp2ent.cost_threshold_for_parallelism",
            "config_spec.sqlserver_config_2016sp2ent.audit_level",
            "config_spec.sqlserver_config_2016sp2ent.fill_factor_percent",
            "config_spec.sqlserver_config_2016sp2ent.optimize_for_ad_hoc_workloads",
            "config_spec.access.data_transfer",
            "config_spec.access.data_lens"
          ]
        }
      }
      """
      Then we get gRPC response with body
      """
      {
         "created_by": "user",
         "description": "Modify Microsoft SQLServer cluster",
         "done": false,
         "id": "worker_task_id2",
         "metadata": {
           "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateClusterMetadata",
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
      Then we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
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
                "hours": 11,
                "minutes": 11,
                "seconds": 9,
                "nanos": 0
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
              "audit_level":                    "2",
              "cost_threshold_for_parallelism": "666",
              "fill_factor_percent":            "51",
              "max_degree_of_parallelism":      "10",
              "optimize_for_ad_hoc_workloads":  true
            },
            "user_config": {
              "cost_threshold_for_parallelism": "666",
              "fill_factor_percent":            "51",
              "optimize_for_ad_hoc_workloads":  true,
              "max_degree_of_parallelism":      "10",
              "audit_level": "2"
            }
          },
          "resources": {
            "resource_preset_id": "s1.porto.1",
            "disk_type_id": "local-ssd",
            "disk_size": "10737418240"
          }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folder_id": "folder1",
        "id": "cid1",
        "name": "test",
        "network_id": "network1"
      }
      """
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {}
        },
        "update_mask": {
          "paths": [
            "config_spec.sqlserver_config_2016sp2ent.max_degree_of_parallelism",
            "config_spec.sqlserver_config_2016sp2ent.cost_threshold_for_parallelism",
            "config_spec.sqlserver_config_2016sp2ent.audit_level",
            "config_spec.sqlserver_config_2016sp2ent.fill_factor_percent",
            "config_spec.sqlserver_config_2016sp2ent.optimize_for_ad_hoc_workloads"
          ]
        }
      }
      """
      Then we get gRPC response with body
      """
      {
         "created_by": "user",
         "description": "Modify Microsoft SQLServer cluster",
         "done": false,
         "id": "worker_task_id3",
         "metadata": {
           "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateClusterMetadata",
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
      Then we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
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
            "hours": 11,
            "minutes": 11,
            "seconds": 9,
            "nanos": 0
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
              "optimize_for_ad_hoc_workloads":  false
            },
            "user_config": {
              "audit_level":                    null,
              "cost_threshold_for_parallelism": null,
              "fill_factor_percent":            null,
              "max_degree_of_parallelism":      null,
              "optimize_for_ad_hoc_workloads":  null
            }
          },
          "resources": {
            "resource_preset_id": "s1.porto.1",
            "disk_type_id": "local-ssd",
            "disk_size": "10737418240"
          }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folder_id": "folder1",
        "id": "cid1",
        "name": "test",
        "network_id": "network1"
      }
      """

    Scenario: Modify with negative max_degree_of_parallelism fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {
           "maxDegreeOfParallelism": -1
         }
        },
        "update_mask": {
          "paths": [
            "config_spec.sqlserver_config_2016sp2ent.max_degree_of_parallelism"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "MaxDegreeOfParallelism must be in range from 0 to 32767 instead of: -1"

    Scenario: Modify with too big max_degree_of_parallelism fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {
           "maxDegreeOfParallelism": 32768
         }
        },
        "update_mask": {
          "paths": [
            "config_spec.sqlserver_config_2016sp2ent.max_degree_of_parallelism"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "MaxDegreeOfParallelism must be in range from 0 to 32767 instead of: 32768"

    Scenario: Modify with small cost_threshold_for_parallelism fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {
           "costThresholdForParallelism": 4
         }
        },
        "update_mask": {
          "paths": [
            "config_spec.sqlserver_config_2016sp2ent.cost_threshold_for_parallelism"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "CostThresholdForParallelism must be in range from 5 to 32767 instead of: 4"

    Scenario: Modify with too big cost_threshold_for_parallelism fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {
           "costThresholdForParallelism": 32768
         }
        },
        "update_mask": {
          "paths": [
            "config_spec.sqlserver_config_2016sp2ent.cost_threshold_for_parallelism"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "CostThresholdForParallelism must be in range from 5 to 32767 instead of: 32768"

    Scenario: Modify with negative audit_level fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {
           "auditLevel": -1
         }
        },
        "update_mask": {
          "paths": [
            "config_spec.sqlserver_config_2016sp2ent.audit_level"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "AuditLevel must be in range from 0 to 3 instead of: -1"


    Scenario: Modify with too big audit_level fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {
           "auditLevel": 4
         }
        },
        "update_mask": {
          "paths": [
            "config_spec.sqlserver_config_2016sp2ent.audit_level"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "AuditLevel must be in range from 0 to 3 instead of: 4"

    Scenario: Modify with negative fill_factor_percent fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {
           "fillFactorPercent": -1
         }
        },
        "update_mask": {
          "paths": [
            "config_spec.sqlserver_config_2016sp2ent.fill_factor_percent"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "FillFactorPercent must be in range from 0 to 100 instead of: -1"


    Scenario: Modify with too big fill_factor_percent fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {
           "fillFactorPercent": 146
         }
        },
        "update_mask": {
          "paths": [
            "config_spec.sqlserver_config_2016sp2ent.fill_factor_percent"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "FillFactorPercent must be in range from 0 to 100 instead of: 146"


    Scenario: Modify backup window works
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "backup_window_start": {
            "hours": 23,
            "minutes": 10
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.backup_window_start"
          ]
        }
      }
      """
      Then we get gRPC response with body
      """
      {
         "created_by": "user",
         "description": "Modify Microsoft SQLServer cluster",
         "done": false,
         "id": "worker_task_id2",
         "metadata": {
           "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateClusterMetadata",
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
      Then we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
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
            "data_lens": true,
            "data_transfer": true,
            "web_sql": false,
            "serverless": false
          },
          "backup_window_start": {
            "hours": 23,
            "minutes": 10,
            "seconds": 0,
            "nanos": 0
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
        "environment": "PRESTABLE",
        "folder_id": "folder1",
        "id": "cid1",
        "name": "test",
        "network_id": "network1"
      }
      """

    Scenario: Modify with negative hours for backup_window_start fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "backup_window_start": {
            "hours": -1
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.backup_window_start"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "Backup window start hours should be in range from 0 to 23 instead of -1"

    Scenario: Modify with too big hours for backup_window_start fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "backup_window_start": {
            "hours": 24
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.backup_window_start"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "Backup window start hours should be in range from 0 to 23 instead of 24"

    Scenario: Modify with negative minutes for backup_window_start fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "backup_window_start": {
            "minutes": -1
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.backup_window_start"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "Backup window start minutes should be in range from 0 to 59 instead of -1"

    Scenario: Modify with too big minutes for backup_window_start fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "backup_window_start": {
            "minutes": 60
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.backup_window_start"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "Backup window start minutes should be in range from 0 to 59 instead of 60"

    Scenario: Modify with negative seconds for backup_window_start fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "backup_window_start": {
            "seconds": -1
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.backup_window_start"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "Backup window start seconds should be in range from 0 to 59 instead of -1"

    Scenario: Modify with too big seconds for backup_window_start fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "backup_window_start": {
            "seconds": 60
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.backup_window_start"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "Backup window start seconds should be in range from 0 to 59 instead of 60"

    Scenario: Modify with negative nanos for backup_window_start fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "backup_window_start": {
            "nanos": -1
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.backup_window_start"
          ]
        }
      }
      """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "Backup window start nanos should be in range from 0 to 10^9-1 instead of -1"

    Scenario: Modify with too big nanos for backup_window_start fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "backup_window_start": {
            "nanos": 1000000000
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.backup_window_start"
          ]
        }
      }
      """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "Backup window start nanos should be in range from 0 to 10^9-1 instead of 1000000000"

  Scenario Outline: Create cluster of different versions works
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
              "hours": 11,
              "minutes": 11,
              "seconds": 9,
              "nanos": 0
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
            "audit_level":                    "2",
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
            "audit_level": "2"
          }
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": "10737418240"
        }
      },
      "description": "test cluster <version>",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid2",
      "name": "test_<version>",
      "network_id": "network1"
    }
    """
  Examples:
    | version     |
    | 2016sp2ent  |
    | 2016sp2std  |
    | 2017ent     |
    | 2017std     |
    | 2019ent     |
    | 2019std     |


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