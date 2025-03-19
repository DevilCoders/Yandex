@elasticsearch
@grpc_api
Feature: ElasticSearch basic operations (compute)
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
                       "name": "elasticsearch",
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
                       "name": "elasticsearch",
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
                       "name": "elasticsearch",
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
                       "name": "elasticsearch",
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
                       "name": "elasticsearch",
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
                       "name": "elasticsearch",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           }
       ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "service_account_id": "service_account_1",
        "user_specs": [{
            "name": "user1",
            "password": "fencesmakegoodneighbours"
        }],
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "elasticsearch_spec": {
                "data_node": {
                    "elasticsearch_config_7": {
                        "fielddata_cache_size": "100mb",
                        "max_clause_count": 200,
                        "reindex_remote_whitelist": "*.mdb.yandexcloud.net",
                        "reindex_ssl_ca_path": "extensions/ssl/remote.crt"
                    },
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
                },
                "plugins": ["analysis-icu"]
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
        "description": "Create ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And "worker_task_id1" acquired and finished by worker

  @delete
  Scenario: Cluster creation and deletion works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "config": {
            "elasticsearch": {
                "data_node": {
                    "elasticsearch_config_set_7": {
                        "effective_config": {
                            "fielddata_cache_size": "100mb",
                            "max_clause_count": "200",
                            "reindex_remote_whitelist": "*.mdb.yandexcloud.net",
                            "reindex_ssl_ca_path": "extensions/ssl/remote.crt"
                        },
                        "user_config": {
                            "fielddata_cache_size": "100mb",
                            "max_clause_count": "200",
                            "reindex_remote_whitelist": "*.mdb.yandexcloud.net",
                            "reindex_ssl_ca_path": "extensions/ssl/remote.crt"
                        },
                        "default_config": {}
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    }
                },
                "master_node": {
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    }
                },
                "plugins": ["analysis-icu"]
            },
            "version": "7.14",
            "edition": "basic"
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
        "network_id": "network1",
        "service_account_id": "service_account_1",
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
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
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
    When we "ListOperations" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
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
                "created_by": "user",
                "created_at": "**IGNORE**",
                "modified_at": "**IGNORE**",
                "description": "Create ElasticSearch cluster",
                "done": true,
                "id": "worker_task_id1",
                "metadata": {
                    "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.CreateClusterMetadata",
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
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.DeleteClusterMetadata",
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
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "cluster id "cid1" not found"


  Scenario: Cluster stop and start works
    When we "Stop" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Stop ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.StopClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
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
    When we "Start" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Start ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.StartClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
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
