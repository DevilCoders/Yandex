@opensearch
@grpc_api
Feature: OpenSearch basic operations (porto)

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
                       "name": "opensearch",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "myt-2.db.yandex.net",
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
               "fqdn": "sas-1.db.yandex.net",
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
               "fqdn": "sas-2.db.yandex.net",
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
               "fqdn": "man-1.db.yandex.net",
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
               "fqdn": "man-2.db.yandex.net",
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
               "fqdn": "man-3.db.yandex.net",
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
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
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
            "zone_id": "man",
            "type": "DATA_NODE"
        }, {
            "zone_id": "myt",
            "type": "MASTER_NODE"
        }, {
            "zone_id": "sas",
            "type": "MASTER_NODE"
        }, {
            "zone_id": "man",
            "type": "MASTER_NODE"
        }],
        "description": "test cluster",
        "labels": {
            "foo": "bar"
        },
        "networkId": "IN-PORTO-NO-NETWORK-API"
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

  Scenario: Cluster creation and deletion works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "config": {
            "opensearch": {
                "data_node": {
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    }
                },
                "master_node": {
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    }
                }
            },
            "version": "7.14"
        },
        "description": "test cluster",
        "labels": {
            "foo": "bar"
        },
        "environment": "PRESTABLE",
        "folder_id": "folder1",
        "health": "ALIVE",
        "id": "cid1",
        "monitoring":[],
        "name": "test",
        "status": "RUNNING",
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
                "name": "man-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "man"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "man"
            },
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
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "sas"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "sas"
            }
        ]
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete OpenSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.DeleteClusterMetadata",
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

  Scenario: Cluster listing works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "another_test_cluster",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.10",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "description": "another test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "List" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "page_size": 100
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "clusters": [
            {
                "config": {
                    "version": "7.10",
                    "opensearch": {
                        "data_node": {
                            "resources": {
                                "disk_size": "10737418240",
                                "disk_type_id": "local-ssd",
                                "resource_preset_id": "s1.porto.1"
                            }
                        }
                   }
                },
                "created_at":  "**IGNORE**",
                "description": "another test cluster",
                "environment": "PRESTABLE",
                "folder_id":   "folder1",
                "id":          "cid2",
                "maintenance_window": { "anytime": {} },
                "name":        "another_test_cluster",
                "status":      "RUNNING"
            },
            {
                "config": {
                    "version": "7.14",
                    "opensearch": {
                        "data_node": {
                            "resources": {
                                "disk_size": "10737418240",
                                "disk_type_id": "local-ssd",
                                "resource_preset_id": "s1.porto.1"
                            }
                        },
                        "master_node": {
                            "resources": {
                                "resource_preset_id": "s1.porto.1",
                                "disk_type_id": "local-ssd",
                                "disk_size": "10737418240"
                           }
                       }
                   }
                },
                "created_at":  "**IGNORE**",
                "description": "test cluster",
                "environment": "PRESTABLE",
                "folder_id":   "folder1",
                "health":      "ALIVE",
                "id":          "cid1",
                "labels": {
                    "foo": "bar"
                },
                "maintenance_window": { "anytime": {} },
                "name":        "test",
                "status":      "RUNNING"
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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

  @hosts @list
  Scenario: Host listing works
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 1
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [{
            "assign_public_ip": false,
            "cluster_id": "cid1",
            "health": "ALIVE",
            "name": "man-1.db.yandex.net",
            "resources": {
                "disk_size": "10737418240",
                "disk_type_id": "local-ssd",
                "resource_preset_id": "s1.porto.1"
            },
            "services":  [],
            "system": null,
            "subnet_id": "",
            "type": "DATA_NODE",
            "zone_id": "man"
        }],
        "next_page_token": "eyJPZmZzZXQiOjF9"
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_token": "eyJPZmZzZXQiOjF9"
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
                "name": "man-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "man"
            },
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
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "sas"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "sas"
            }
        ],
        "next_page_token": ""
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 5,
        "page_token": "eyJPZmZzZXQiOjF9"
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
                "name": "man-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "man"
            },
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
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "sas"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "sas"
            }
        ],
        "next_page_token": "eyJPZmZzZXQiOjZ9"
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_token": "eyJPZmZzZXQiOjZ9"
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [],
        "next_page_token": ""
    }
    """

  @hosts @create
  Scenario: Adding host works
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "host_specs": [{
            "type": "DATA_NODE",
            "zone_id": "man"
        }],
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add datanode host to OpenSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.AddClusterHostsMetadata",
            "cluster_id": "cid1",
            "host_names": ["man-3.db.yandex.net"]
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
                "name": "man-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "man"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "man"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "man-3.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "man"
            },
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
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "sas"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "sas"
            }
        ]
    }
    """

  @hosts @create
  Scenario Outline: Adding host with invalid params fails
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": <hosts>
    }
    """
    Then we get gRPC response error with code <code> and message "<message>"
    Examples:
      | hosts                                                                              | code                | message                                                                                 |
      | []                                                                                 | FAILED_PRECONDITION | no hosts to add are specified                                                           |
      | [{"zone_id": "man", "type": "DATA_NODE"}, {"zone_id": "man", "type": "DATA_NODE"}] | UNIMPLEMENTED       | adding multiple hosts at once is not supported yet                                      |
      | [{"zone_id": "man", "type": "MASTER_NODE"}]                                        | UNIMPLEMENTED       | only data nodes can be added                                                            |
      | [{"zone_id": "nodc", "type": "DATA_NODE"}]                                         | INVALID_ARGUMENT    | invalid zone nodc, valid are: iva, man, myt, sas, vla |

  @hosts @delete
  Scenario: Deleting host works
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "host_names": ["man-1.db.yandex.net"],
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete datanode host from OpenSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.DeleteClusterHostsMetadata",
            "cluster_id": "cid1",
            "host_names": ["man-1.db.yandex.net"]
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
                "name": "man-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "man"
            },
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
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "myt"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "DATA_NODE",
                "zone_id": "sas"
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "services":  [],
                "system": null,
                "subnet_id": "",
                "type": "MASTER_NODE",
                "zone_id": "sas"
            }
        ]
    }
    """

  @hosts @delete
  Scenario Outline: Deleting host with invalid params fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test2",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.10",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "man",
            "type": "DATA_NODE"
        }, {
            "zone_id": "sas",
            "type": "DATA_NODE"
        }, {
            "zone_id": "vla",
            "type": "DATA_NODE"
        }],
        "description": "test cluster 2",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2",
        "host_names": <hosts>
    }
    """
    Then we get gRPC response error with code <code> and message "<message>"
    Examples:
      | hosts                                          | code                | message                                              |
      | []                                             | FAILED_PRECONDITION | no hosts to delete are specified                     |
      | ["man-1.db.yandex.net", "sas-1.db.yandex.net"] | UNIMPLEMENTED       | deleting multiple hosts at once is not supported yet |
      | ["nohost"]                                     | NOT_FOUND           | host "nohost" does not exist                         |

  Scenario: Cluster with service account not allowed
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "testbad",
        "environment": "PRESTABLE",
        "service_account_id": "service_account_1",
        "config_spec": {
            "version": "7.10",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "service accounts not supported in porto clusters"
  Scenario: Return error on unknown field paths
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "service_account_id": "service_account_1",
        "update_mask": {
            "paths": [
                "service_account_id"
           ]
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "service accounts not supported in porto clusters"
