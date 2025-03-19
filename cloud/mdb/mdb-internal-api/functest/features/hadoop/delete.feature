Feature: Specific to Hadoop delete actions

  Scenario: Verify that broken cluster can be delete
    . Hadoop cluster deleted in specific way,
    . so better check it
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
    When "worker_task_id1" acquired and failed by worker
    And we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "ERROR"
    }
    """
    When we DELETE "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """
