Feature: Cloud logging group manipulation works

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
        "logGroupId": "log_group_id_1",
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

  @fails
  Scenario: Creating dataproc cluster with non existing log group fails
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
                    "hostsCount": 2,
                    "subnetId": "network1-myt"
                }
            ]
        },
        "zoneId": "myt",
        "logGroupId": "not_existing_log_group_id",
        "serviceAccountId": "service_account_1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Can not find log group not_existing_log_group_id. Or service account service_account_1 does not have permission to get log group not_existing_log_group_id. Grant role \"logging.writer\" to service account service_account_1 for the folder of this log group."
    }
    """

  @fail
  Scenario: Creating dataproc cluster without proper permissions for log group fails
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
                    "hostsCount": 2,
                    "subnetId": "network1-myt"
                }
            ]
        },
        "zoneId": "myt",
        "logGroupId": "log_group_from_folder_2_id",
        "serviceAccountId": "service_account_1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Service account service_account_1 does not have permission to write to log group log_group_from_folder_2_id. Grant role \"logging.writer\" to service account service_account_1 for folder folder2."
    }
    """
