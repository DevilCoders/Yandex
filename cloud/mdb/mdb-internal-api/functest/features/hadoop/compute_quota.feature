Feature: Compute quota is being checked before cluster operations creation

  Background:
    Given default headers
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "test",
        "configSpec": {
            "versionId": "1.4",
            "hadoop": {
                "sshPublicKeys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"]
            },
            "subclustersSpec": [
                {
                    "name": "main",
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 21474836480
                    },
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 21474836480
                    },
                    "hostsCount": 5,
                    "subnetId": "network1-myt"
                }
            ]
        },
        "zoneId": "myt",
        "serviceAccountId": "service_account_1"
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
    When "worker_task_id1" acquired and finished by worker
    When we run query
    """
    UPDATE dbaas.subclusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """

  @quota_fail
  Scenario: Create dataproc cluster that does not fit to Compute quota fails
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "test2",
        "configSpec": {
            "versionId": "1.4",
            "hadoop": {
                "sshPublicKeys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"]
            },
            "subclustersSpec": [
                {
                    "name": "main",
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 161061273600
                    },
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 2199019061248
                    },
                    "hostsCount": 20,
                    "subnetId": "network1-myt"
                }
            ]
        },
        "zoneId": "myt",
        "serviceAccountId": "service_account_1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Insufficient Compute quota for: \n compute.instances.count. Requested: 21. Available: 10.0.\n compute.instanceMemory.size. Requested: 90194313216. Available: 68719476736.0.\n compute.ssdDisks.size. Requested: 44141442498560. Available: 2700460687360.0.\n compute.disks.count. Requested: 21. Available: 10.0.\nIncrease Compute quota or try to make a smaller cluster."
    }
    """

  @modify @quota_fail
  Scenario: Modify subcluster with resources that do not fit to quota fails
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "subclustersSpec": [
                {
                    "id": "subcid2",
                    "resources": {
                        "resourcePresetId": "s1.compute.2",
                        "diskTypeId": "network-ssd",
                        "diskSize": 2199019061248
                    },
                    "hostsCount": 999
                }
            ]
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Insufficient Compute quota for: \n compute.instances.count. Requested: 994. Available: 10.0.\n compute.instanceCores.count. Requested: 1993.0. Available: 1200.0.\n compute.instanceMemory.size. Requested: 8559869820928. Available: 68719476736.0.\n compute.ssdDisks.size. Requested: 2196712668004352. Available: 2700460687360.0.\n compute.disks.count. Requested: 994. Available: 10.0.\nIncrease Compute quota or try to make a smaller cluster."
    }
    """
