Feature: List Data Proc UI Links

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
                "services": ["HDFS", "YARN", "FLUME"],
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
        "description": "test cluster",
        "zoneId": "myt",
        "serviceAccountId": "service_account_1",
        "bucket": "user_s3_bucket",
        "labels": {
           "foo": "bar",
           "mycluster": "42"
        }
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
    And "worker_task_id1" acquired and finished by worker

  Scenario: Unable to list UI Links if feature flag is off
    When we GET "/mdb/hadoop/1.0/clusters/cid1/ui_links"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Requested feature is not available"
    }
    """

  Scenario: List of UI Links is empty if feature wasn't turned on for the cluster
    Given feature flags
    """
    ["MDB_DATAPROC_UI_PROXY"]
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/ui_links"
    Then we get response with status 200 and body contains
    """
    {
        "links": []
    }
    """

  Scenario: Can list UI Links if feature is turned on for the cluster
    Given feature flags
    """
    ["MDB_DATAPROC_UI_PROXY"]
    """
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "uiProxy": true
    }
    """
    Then we get response with status 200
    Then "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/hadoop/1.0/clusters/cid1/ui_links"
    Then we get response with status 200 and body contains
    """
    {
        "links": [
            {
                "name": "HDFS Namenode UI",
                "url": "https://cluster-cid1.dataproc-ui/hdfs/"
            },
            {
                "name": "YARN Resource Manager Web UI",
                "url": "https://cluster-cid1.dataproc-ui/yarn/"
            },
            {
                "name": "JobHistory Server Web UI",
                "url": "https://cluster-cid1.dataproc-ui/jobhistory/"
            }
        ]
    }
    """

  Scenario: Can list UI Links if "bypass Knox" mode is turned on for the cluster
    Given feature flags
    """
    ["MDB_DATAPROC_UI_PROXY"]
    """
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "uiProxy": true
    }
    """
    Then we get response with status 200
    Then "worker_task_id2" acquired and finished by worker
    And we run query
    """
        UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,unmanaged,bypass_knox}', 'true')
        WHERE cid = 'cid1'
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/ui_links"
    Then we get response with status 200 and body contains
    """
    {
        "links": [
            {
                "name": "HDFS Namenode UI",
                "url": "https://ui-cid1-myt-dataproc-m-1-1001.dataproc-ui/"
            },
            {
                "name": "YARN Resource Manager Web UI",
                "url": "https://ui-cid1-myt-dataproc-m-1-1002.dataproc-ui/"
            },
            {
                "name": "JobHistory Server Web UI",
                "url": "https://ui-cid1-myt-dataproc-m-1-1003.dataproc-ui/"
            }
        ]
    }
    """
