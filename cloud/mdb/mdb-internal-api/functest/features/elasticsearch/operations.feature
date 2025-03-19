@elasticsearch
@grpc_api
Feature: Operation for Managed Elasticsearch
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
                       "name": "elasticsearch",
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
                       "name": "elasticsearch",
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
                       "name": "elasticsearch",
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
                       "name": "elasticsearch",
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
                       "name": "elasticsearch",
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
                        "max_clause_count": 200
                    },
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

  Scenario: Get operation returns cluster data
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_at": "**IGNORE**",
        "created_by": "user",
        "description": "Create ElasticSearch cluster",
        "done": true,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        },
        "modified_at": "**IGNORE**",
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.Cluster",
            "config": {
                "elasticsearch": {
                    "data_node": {
                        "elasticsearch_config_set_7": {
                            "effective_config": {
                                "fielddata_cache_size": "100mb",
                                "max_clause_count": "200"
                            },
                            "user_config": {
                                "fielddata_cache_size": "100mb",
                                "max_clause_count": "200"
                            },
                            "default_config": {}
                        },
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
                "version": "7.14",
                "edition": "basic"
            },
            "created_at": "**IGNORE**",
            "description": "test cluster",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "health": "ALIVE",
            "id": "cid1",
            "labels": {
                "foo": "bar"
            },
            "maintenance_window": { "anytime": {} },
            "monitoring": [
                {
                    "description": "YaSM (Golovan) charts",
                    "link": "https://yasm/cid=cid1",
                    "name": "YASM"
                },
                {
                    "description": "Solomon charts",
                    "link": "https://solomon/cid=cid1\u0026fExtID=folder1",
                    "name": "Solomon"
                },
                {
                    "description": "Console charts",
                    "link": "https://console/cid=cid1\u0026fExtID=folder1",
                    "name": "Console"
                }
            ],
            "name": "test",
            "status": "RUNNING"
        }
    }
    """

  Scenario: Create User operation returns user data
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
          "name": "test",
          "password": "TestPassword"
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create user in ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.CreateUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create user in ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.CreateUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test"
        }
    }
    """
