@sqlserver
@grpc_api
Feature: Mofidy Compute Microsoft SQLServer Cluster

  Background:
    Given default headers
    And health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
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
      "networkId": "network1",
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

  Scenario: Cluster modify works
     When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
        "cluster_id": "cid1",
        "description": "another test cluster",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {},
          "resources": {
            "resourcePresetId": "s1.porto.2",
            "diskTypeId": "local-ssd",
            "diskSize": 21474836480
          }
        },
        "labels": {
            "label1": "value1"
        },
        "update_mask": {
          "paths": [
            "description",
            "labels",
            "config_spec.resources.resource_preset_id",
            "config_spec.resources.disk_type_id",
            "config_spec.resources.disk_size"
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

  Scenario: Cluster modify works without update mask fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
       "cluster_id": "cid1",
       "description": "another test cluster",
       "config_spec": {
         "sqlserver_config_2016sp2ent": {},
         "resources": {
           "resourcePresetId": "s1.porto.2",
           "diskTypeId": "local-ssd",
           "diskSize": 21474836480
         }
       },
       "labels": {
           "label1": "value1"
       }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"


  Scenario: Cluster modify only disk size works
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
        "cluster_id": "cid1",
        "config_spec": {
          "resources": {
                "disk_size": "21474836480"
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.resources.disk_size"
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
                    "disk_size": "21474836480",
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
  Scenario: Cluster modify with no actual changes fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
        "cluster_id": "cid1"
     }
     """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"

  Scenario: Cluster modify only resources works
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
        "cluster_id": "cid1",
        "config_spec": {
          "resources": {
            "resourcePresetId": "s1.porto.2",
            "diskTypeId": "local-ssd",
            "diskSize": 21474836480
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.resources.resource_preset_id",
            "config_spec.resources.disk_type_id",
            "config_spec.resources.disk_size"
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
                    "disk_size": "21474836480",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.2"
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

  Scenario: Cluster modify only resource preset id works
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
        "cluster_id": "cid1",
        "config_spec": {
          "resources": {
            "resourcePresetId": "s1.porto.2"
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.resources.resource_preset_id"
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
                    "resource_preset_id": "s1.porto.2"
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

  Scenario: Cluster modify only resources with no actual changes fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
        "cluster_id": "cid1",
        "config_spec": {
          "resources": {
            "resourcePresetId": "s1.porto.1",
            "diskTypeId": "local-ssd",
            "diskSize": 10737418240
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.resources.resource_preset_id",
            "config_spec.resources.disk_type_id",
            "config_spec.resources.disk_size"
          ]
        }
     }
     """
     Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"

  Scenario: Cluster modify without cluster id fails
     When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
        "description": "another test cluster",
        "config_spec": {
          "sqlserver_config_2016sp2ent": {},
          "resources": {
            "resourcePresetId": "s1.porto.1",
            "diskTypeId": "local-ssd",
            "diskSize": 10737418240
          }
        },
        "labels": {
            "label1": "value1"
        },
        "update_mask": {
          "paths": [
            "description",
            "labels",
            "config_spec.resources.resource_preset_id",
            "config_spec.resources.disk_type_id",
            "config_spec.resources.disk_size"
          ]
        }
     }
     """
     Then we get gRPC response error with code INVALID_ARGUMENT and message "cluster id need to be set in cluster update request"

  Scenario: Cluster version upgrade rejected
     When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
        "cluster_id": "cid1",
        "description": "another test cluster",
        "config_spec": {
          "version": "2018sp2",
          "sqlserver_config_2016sp2ent": {},
          "resources": {
            "resourcePresetId": "s1.porto.1",
            "diskTypeId": "local-ssd",
            "diskSize": 10737418240
          }
        },
        "labels": {
            "label1": "value1"
        },
        "update_mask": {
          "paths": [
            "description",
            "labels",
            "config_spec.version",
            "config_spec.resources.resource_preset_id",
            "config_spec.resources.disk_type_id",
            "config_spec.resources.disk_size"
          ]
        }
     }
     """
    Then we get gRPC response error with code UNIMPLEMENTED and message "version upgrade is not implemented yet"

  Scenario: Cluster modify with only name works
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
        "cluster_id": "cid1",
        "name": "another_name",
        "update_mask": {
          "paths": [
            "name"
          ]
        }
     }
     """
    Then we get gRPC response with body
     """
     {
        "created_by": "user",
        "description": "Update Microsoft SQLServer cluster metadata",
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
       "cluster_id": "cid1"
     }
     """
    Then we get gRPC response with body
     """
     {
        "name": "another_name"
     }
     """

  Scenario: Cluster modify with invalid name fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
        "cluster_id": "cid1",
        "name": "another_name!@",
        "update_mask": {
          "paths": [
            "name"
          ]
        }
     }
     """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "cluster name "another_name!@" has invalid symbols"

  Scenario: Cluster modify with too long name fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
       """
       {
          "cluster_id": "cid1",
          "name": "another_name_long_name_so_logn_i_cant_even_imagine_it_without_copypasting_some_words",
          "update_mask": {
            "paths": [
              "name"
            ]
          }
       }
       """
      Then we get gRPC response error with code INVALID_ARGUMENT and message "cluster name "another_name_long_name_so_logn_i_cant_even_imagine_it_without_copypasting_some_words" is too long"

  Scenario: Cluster modify with only description works
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
        "cluster_id": "cid1",
        "description": "another description",
        "update_mask": {
          "paths": [
            "description"
          ]
        }
     }
     """
    Then we get gRPC response with body
     """
     {
        "created_by": "user",
        "description": "Modify Microsoft SQLServer cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateClusterMetadata",
          "cluster_id": "cid1"
        }
     }
     """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
     """
     {
       "cluster_id": "cid1"
     }
     """
    Then we get gRPC response with body
     """
     {
        "description": "another description"
     }
     """

  Scenario: Cluster modify with too long description fails
      When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
       """
       {
          "cluster_id": "cid1",
          "description": "another iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
          "update_mask": {
            "paths": [
              "description"
            ]
          }
       }
       """
       Then we get gRPC response error with code INVALID_ARGUMENT and message "cluster description "another iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii" is too long"

  Scenario: Cluster modify only labels works
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
       "cluster_id": "cid1",
       "labels":      {
           "label1": "value1"
       },
       "update_mask": {
         "paths": [
           "labels"
         ]
       }
    }
    """
    Then we get gRPC response with body
    """
    {
       "created_by": "user",
       "description": "Update Microsoft SQLServer cluster metadata",
       "done": false,
       "id": "worker_task_id2",
       "metadata": {
         "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateClusterMetadata",
         "cluster_id": "cid1"
       }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
       "cluster_id": "cid1",
       "labels": {},
       "update_mask": {
         "paths": [
           "labels"
         ]
       }
    }
    """
    Then we get gRPC response with body
    """
    {
       "created_by": "user",
       "description": "Update Microsoft SQLServer cluster metadata",
       "done": false,
       "id": "worker_task_id3",
       "metadata": {
         "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateClusterMetadata",
         "cluster_id": "cid1"
       }
    }
    """

  Scenario: Cluster modify backup window start works
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
       "cluster_id": "cid1",
       "config_spec": {
           "backup_window_start": {
               "hours": 13,
               "minutes": 13,
               "nanos": 13,
               "seconds": 13
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
          "data_lens": false,
          "data_transfer": false,
          "web_sql": false,
          "serverless": false
        },
        "backup_window_start": {
          "hours": 13,
          "minutes": 13,
          "seconds": 13,
          "nanos": 13
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
      "description": "test cluster",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1"
    }
    """

  Scenario: Cluster modify access DataLens works
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
       "cluster_id": "cid1",
       "config_spec": {
           "access": {
               "data_lens": true
           }
       },
       "update_mask": {
         "paths": [
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
          "data_lens": true,
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
      "description": "test cluster",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1"
    }
    """

  Scenario: Cluster modify access secondary connections works
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
       "cluster_id": "cid1",
       "config_spec": {
           "secondary_connections": "SECONDARY_CONNECTIONS_READ_ONLY"
       },
       "update_mask": {
         "paths": [
           "config_spec.secondary_connections"
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
          "hours": 22,
          "minutes": 15,
          "seconds": 30,
          "nanos": 100
        },
        "secondary_connections": "SECONDARY_CONNECTIONS_READ_ONLY",
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
      "description": "test cluster",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1"
    }
    """

  Scenario: Cluster modify service account works
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
       "cluster_id": "cid1",
       "service_account_id": "sa1",
       "update_mask": {
         "paths": [
           "service_account_id"
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
      "description": "test cluster",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1",
      "service_account_id": "sa1"
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
       "cluster_id": "cid1",
       "service_account_id": "",
       "update_mask": {
         "paths": [
           "service_account_id"
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
      "description": "test cluster",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1",
      "service_account_id": ""
    }
    """

  Scenario: Cluster modify with incorrect service account fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
       "cluster_id": "cid1",
       "service_account_id": "unknown_sa",
       "update_mask": {
         "paths": [
           "service_account_id"
         ]
       }
    }
    """
    Then we get gRPC response error with code PERMISSION_DENIED and message "you do not have permission to access the requested service account or service account does not exist"
