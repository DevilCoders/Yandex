@opensearch
@grpc_api
Feature: OpenSearch single node cluster
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
            "version": "7.10",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
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

  Scenario: Check cluster config for empty master section
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
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    }
                }
            },
            "version": "7.10"
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
