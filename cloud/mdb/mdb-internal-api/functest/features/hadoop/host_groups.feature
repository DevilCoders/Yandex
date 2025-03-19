Feature: Dataproc cluster on host groups

  Background:
    Given default headers

  Scenario: Successful creation of dataproc cluster on host groups
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "test",
        "configSpec": {
            "versionId": "1.4",
            "hadoop": {
                "services": ["HDFS", "YARN"],
                "sshPublicKeys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"]
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
                },
                {
                    "name": "compute",
                    "role": "COMPUTENODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.2",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "hostsCount": 1,
                    "autoscalingConfig": {
                        "maxHostsCount": 5
                    },
                    "subnetId": "network1-myt"
                }
            ]
        },
        "zoneId": "myt",
        "serviceAccountId": "service_account_editor",
        "hostGroupIds": ["hg_id1", "hg_id2"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create Data Proc cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    Then "worker_task_id1" acquired and finished by worker
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "hadoop": {
                "services": ["HDFS", "YARN"],
                "sshPublicKeys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"],
                "properties": {}
            },
            "versionId": "1.4"
        },
        "environment": "PRODUCTION",
        "folderId": "folder1",
        "id": "cid1",
        "name": "test",
        "networkId": "network1",
        "status": "RUNNING",
        "zoneId": "myt",
        "serviceAccountId": "service_account_editor",
        "hostGroupIds": ["hg_id1", "hg_id2"]
    }
    """

  Scenario: Unable to use host group from another cloud
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "test",
        "configSpec": {
            "versionId": "1.4",
            "hadoop": {
                "services": ["HDFS"],
                "sshPublicKeys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"]
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
        "zoneId": "myt",
        "serviceAccountId": "service_account_1",
        "hostGroupIds": ["hg_id3"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Host group hg_id3 belongs to the cloud distinct from cluster's cloud"
    }
    """
