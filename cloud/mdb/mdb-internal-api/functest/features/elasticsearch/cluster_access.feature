@elasticsearch
@grpc_api
Feature: ElasticSearch cluster access settings
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

  Scenario: Cluster creation with access settings work
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "service_account_id": "service_account_1",
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "access": {
                "data_transfer": true
            },
            "elasticsearch_spec": {
                "data_node": {
                    "elasticsearch_config_7": {
                        "fielddata_cache_size": "100mb",
                        "max_clause_count": 200
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
            "access": {
                "data_transfer": true
            },
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
  Scenario: Update Elasticsearch access settings works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "service_account_id": "service_account_1",
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "update_mask": {
            "paths": [
                "config_spec.access"
            ]
        },
        "config_spec": {
            "access": {
                "data_transfer": true
            }
        }
    }
    """
    Then we get gRPC response OK
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response OK

    And gRPC response body at path "$.config.access" contains
    """
    {
      "data_transfer": true
    }
    """
