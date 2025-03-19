@dataproc
@security_groups
Feature: Security Groups in Data Proc cluster

  Background:
    Given default headers
    When we add default feature flag "MDB_DATAPROC_AUTOSCALING"

  Scenario: Create dataproc cluster with SG
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "test",
        "configSpec": {
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
                        "diskSize": 16106127360
                    },
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.2",
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
        "securityGroupIds": ["sg_id7"]
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
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" set to "[sg_id7]"
    When "worker_task_id1" acquired and finished by worker
    And worker set "sg_id7" security groups on "cid1"
    When we run query
    """
    UPDATE dbaas.subclusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "cid1",
        "securityGroupIds": ["sg_id7"]
    }
    """
    And worker clear security groups for "cid1"

  Scenario: Create dataproc cluster with SG without rules (without internal connectivity) fails
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "test",
        "configSpec": {
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
                        "diskSize": 16106127360
                    },
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.2",
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
        "securityGroupIds": ["sg_id6"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Specified set of security_groups rules doesn't have enough permissions for working dataproc correctly."
    }
    """

  Scenario: Modify SG on dataproc works
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "test",
        "configSpec": {
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
                        "diskSize": 16106127360
                    },
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.2",
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
        "securityGroupIds": ["sg_id7"]
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
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" set to "[sg_id7]"
    When "worker_task_id1" acquired and finished by worker
    And worker set "sg_id7" security groups on "cid1"
    When we run query
    """
    UPDATE dbaas.subclusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "cid1",
        "securityGroupIds": ["sg_id7"]
    }
    """
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "securityGroupIds": ["sg_id7", "sg_id6"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify Data Proc cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" set to "[sg_id7, sg_id6]"
    When "worker_task_id2" acquired and finished by worker
    And worker set "sg_id6" security groups on "cid1"
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "cid1",
        "securityGroupIds": ["sg_id6", "sg_id7"]
    }
    """
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "securityGroupIds": ["sg_id6"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Specified set of security_groups rules doesn't have enough permissions for working dataproc correctly."
    }
    """
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "securityGroupIds": []
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify Data Proc cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And in worker_queue exists "worker_task_id3" id with args "security_group_ids" set to "[]"
    When "worker_task_id3" acquired and finished by worker
    And worker clear security groups for "cid1"
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "securityGroupIds": ["sg_id5"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Specified set of security_groups rules doesn't have enough permissions for working dataproc correctly."
    }
    """
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "securityGroupIds": ["sg_id8"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify Data Proc cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And in worker_queue exists "worker_task_id4" id with args "security_group_ids" set to "[sg_id8]"
    When "worker_task_id4" acquired and finished by worker
