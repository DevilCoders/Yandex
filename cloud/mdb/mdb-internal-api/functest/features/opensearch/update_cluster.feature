@opensearch
@grpc_api
Feature: OpenSearch cluster update
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
               "fqdn": "myt-1.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "opensearch",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "myt-2.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "opensearch",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "sas-1.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "opensearch",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "sas-2.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "opensearch",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "vla-1.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "opensearch",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "vla-2.df.cloud.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "opensearch",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           }
       ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }, {
            "zone_id": "sas",
            "type": "DATA_NODE"
        }, {
            "zone_id": "vla",
            "type": "DATA_NODE"
        }, {
            "zone_id": "myt",
            "type": "MASTER_NODE"
        }, {
            "zone_id": "sas",
            "type": "MASTER_NODE"
        }, {
            "zone_id": "vla",
            "type": "MASTER_NODE"
        }],
        "description": "test cluster",
        "labels": {
            "foo": "bar"
        },
        "networkId": "network1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create OpenSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: Update cluster name, description and labels
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "update_mask": {
            "paths": ["name", "description", "labels"]
        },
        "name" : "new cluster name",
        "description": "new description",
        "labels": {
            "key1": "val1",
            "key2": "val2"
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Update OpenSearch cluster metadata",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.UpdateClusterMetadata",
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "description": "new description",
        "name": "new cluster name",
        "labels": {
            "key1": "val1",
            "key2": "val2"
        }
    }
    """

  Scenario: Change Opensearch datanode resources
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "update_mask": {
            "paths": [
                "config_spec.opensearch_spec.data_node.resources.resource_preset_id",
                "config_spec.opensearch_spec.data_node.resources.disk_size"
           ]
        },
        "config_spec": {
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.2",
                        "disk_size": 21474836480
                    }
                }
            }
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Modify OpenSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.UpdateClusterMetadata",
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
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
                "name": "myt-1.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "21474836480",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-myt",
                "type": "DATA_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-2.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-myt",
                "type": "MASTER_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "21474836480",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-sas",
                "type": "DATA_NODE",
                "zone_id": "sas"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-2.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-sas",
                "type": "MASTER_NODE",
                "zone_id": "sas"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "vla-1.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "21474836480",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-vla",
                "type": "DATA_NODE",
                "zone_id": "vla"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "vla-2.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-vla",
                "type": "MASTER_NODE",
                "zone_id": "vla"
            }
        ]
    }
    """

  Scenario: Change Opensearch masternode resources
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "update_mask": {
            "paths": [
                "config_spec.opensearch_spec.master_node.resources.resource_preset_id",
                "config_spec.opensearch_spec.master_node.resources.disk_size"
           ]
        },
        "config_spec": {
            "opensearch_spec": {
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.2",
                        "disk_size": 21474836480
                    }
                }
            }
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Modify OpenSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.UpdateClusterMetadata",
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
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
                "name": "myt-1.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-myt",
                "type": "DATA_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-2.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "21474836480",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-myt",
                "type": "MASTER_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-sas",
                "type": "DATA_NODE",
                "zone_id": "sas"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-2.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "21474836480",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-sas",
                "type": "MASTER_NODE",
                "zone_id": "sas"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "vla-1.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-vla",
                "type": "DATA_NODE",
                "zone_id": "vla"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "vla-2.df.cloud.yandex.net",
                "resources": {
                    "disk_size": "21474836480",
                    "disk_type_id": "network-ssd",
                    "resource_preset_id": "s1.compute.2"
                },
                "services":  [],
                "system": null,
                "subnet_id": "network1-vla",
                "type": "MASTER_NODE",
                "zone_id": "vla"
            }
        ]
    }
    """

# TODO config update
#  Scenario: Update Opensearch datanode config
#    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
#    """
#    {
#        "cluster_id": "cid1",
#        "update_mask": {
#            "paths": [
#                "config_spec.opensearch_spec.data_node.opensearch_config_7.fielddata_cache_size",
#                "config_spec.opensearch_spec.data_node.opensearch_config_7.max_clause_count"
#           ]
#        },
#        "config_spec": {
#            "opensearch_spec": {
#                "data_node": {
#                    "opensearch_config_7": {
#                        "fielddata_cache_size": "900mb",
#                        "max_clause_count": 900
#                    }
#                }
#            }
#        }
#    }
#    """
#    Then we get gRPC response with body
#    """
#    {
#        "created_by": "user",
#        "description": "Modify OpenSearch cluster",
#        "done": false,
#        "id": "worker_task_id2",
#        "metadata": {
#            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.UpdateClusterMetadata",
#            "cluster_id": "cid1"
#        },
#        "response": {
#            "@type": "type.googleapis.com/google.rpc.Status",
#            "code": 0,
#            "details": [],
#            "message": "OK"
#        }
#    }
#    """
#    And "worker_task_id2" acquired and finished by worker
#    When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
#    """
#    {
#        "cluster_id": "cid1"
#    }
#    """
#    Then we get gRPC response with body ignoring empty
#    """
#    {
#        "config": {
#            "opensearch": {
#                "data_node": {
#                    "opensearch_config_set_7": {
#                        "effective_config": {
#                            "fielddata_cache_size": "900mb",
#                            "max_clause_count": "900"
#                        },
#                        "user_config": {
#                            "fielddata_cache_size": "900mb",
#                            "max_clause_count": "900"
#                        },
#                        "default_config": {}
#                    },
#                    "resources": {
#                        "disk_size": "10737418240",
#                        "disk_type_id": "network-ssd",
#                        "resource_preset_id": "s1.compute.1"
#                    }
#                },
#                "master_node": {
#                    "resources": {
#                        "disk_size": "10737418240",
#                        "disk_type_id": "network-ssd",
#                        "resource_preset_id": "s1.compute.1"
#                    }
#                }
#            },
#            "version": "7.14",
#            "edition": "basic"
#        },
#        "description": "test cluster",
#        "labels": {
#            "foo": "bar"
#        },
#        "environment": "PRESTABLE",
#        "folder_id": "folder1",
#        "health": "ALIVE",
#        "id": "cid1",
#        "monitoring":[],
#        "name": "test",
#        "network_id": "network1",
#        "status": "RUNNING",
#        "monitoring": [
#            {
#              "name": "YASM",
#              "description": "YaSM (Golovan) charts",
#              "link": "https://yasm/cid=cid1"
#            },
#            {
#              "name": "Solomon",
#              "description": "Solomon charts",
#              "link": "https://solomon/cid=cid1&fExtID=folder1"
#            },
#            {
#              "name": "Console",
#              "description": "Console charts",
#              "link": "https://console/cid=cid1&fExtID=folder1"
#            }
#        ]
#    }
#    """

  Scenario: Return error when no changes detected within update request
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "update_mask": {
            "paths": [
                "description",
                "config_spec.opensearch_spec.master_node.resources.disk_size"
           ]
        },
        "description": "test cluster",
        "config_spec": {
            "opensearch_spec": {
                "master_node": {
                    "resources": {
                        "disk_size": 10737418240
                    }
                }
            }
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"

  Scenario: Return error on unknown field paths
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "update_mask": {
            "paths": [
                "config_spec.opensearch_spec.master_node.resources.disk_size"
           ]
        }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "unknown field paths: config_spec.opensearch_spec.master_node.resources.disk_size"

# TODO config update
#  Scenario: Return error on invalid setting values
#    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
#    """
#    {
#        "cluster_id": "cid1",
#        "update_mask": {
#            "paths": [
#                "config_spec.opensearch_spec.data_node.opensearch_config_7.fielddata_cache_size",
#                "config_spec.opensearch_spec.data_node.opensearch_config_7.max_clause_count"
#           ]
#        },
#        "config_spec": {
#            "opensearch_spec": {
#                "data_node": {
#                    "opensearch_config_7": {
#                        "fielddata_cache_size": "loh",
#                        "max_clause_count": -2
#                    }
#                }
#            }
#        }
#    }
#    """
#    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid data node config: incorrect value for indices.fielddata.cache.size;specified value for indices.query.bool.max_clause_count is too small (min: 0, max: 10240)"
