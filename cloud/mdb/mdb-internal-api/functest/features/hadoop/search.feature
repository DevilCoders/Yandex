@search
Feature: Hadoop search API

  Scenario: Verify search renders
    Given default headers
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "testHadoopCluster",
        "configSpec": {
            "hadoop": {
                "sshPublicKeys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"],
                "properties": {
                    "hdfs:dfs.replication": 1,
                    "yarn:yarn.timeline-service.generic-application-history.enabled": "true",
                    "yarn:yarn.nodemanager.aux-services.mapreduce_shuffle.class": "org.apache.hadoop.mapred.ShuffleHandler",
                    "hive:hive.server2.metrics.enabled": "true"
                }
            },
            "subclustersSpec": [
                {
                    "name": "main",
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "hostsCount": 5,
                    "subnetId": "network1-myt"
                }
            ]
        },
        "description": "test cluster",
        "zoneId": "myt",
        "serviceAccountId": "service_account_1"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "timestamp": "<TIMESTAMP>",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testHadoopCluster",
        "service": "data-proc",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testHadoopCluster",
            "description": "test cluster",
            "labels": {},
            "hosts": ["myt-dataproc-d-1.df.cloud.yandex.net",
                      "myt-dataproc-d-2.df.cloud.yandex.net",
                      "myt-dataproc-d-3.df.cloud.yandex.net",
                      "myt-dataproc-d-4.df.cloud.yandex.net",
                      "myt-dataproc-d-5.df.cloud.yandex.net",
                      "myt-dataproc-m-1.df.cloud.yandex.net"]
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And  we DELETE "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200
    Then last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "timestamp": "<TIMESTAMP>",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testHadoopCluster",
        "service": "data-proc",
        "deleted": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1"
    }
    """
