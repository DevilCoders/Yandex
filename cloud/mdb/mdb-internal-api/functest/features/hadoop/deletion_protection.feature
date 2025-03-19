Feature: Deletion Protection

Background:
    Given default headers
    And we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "test",
        "configSpec": {
            "versionId": "1.4",
            "hadoop": {
                "sshPublicKeys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"],
                "properties": {
                    "hdfs:dfs.replication": 1,
                    "yarn:yarn.timeline-service.generic-application-history.enabled": "true",
                    "yarn:yarn.nodemanager.aux-services.mapreduce_shuffle.class": "org.apache.hadoop.mapred.ShuffleHandler",
                    "capacity-scheduler:yarn.scheduler.capacity.maximum-am-resource-percent": "1.0",
                    "hive:hive.server2.metrics.enabled": "true",
                    "hivemetastore:hive.metastore.schema.verification": "true",
                    "hiveserver2:hive.server2.metrics.enabled": "true",
                    "dataproc:max-concurrent-jobs": 13,
                    "spark:spark.history.fs.cleaner.maxAge": "3d"
                }
            },
            "subclustersSpec": [
                {
                    "name": "main",
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskSize": 16106127360
                    },
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd"
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
        "id": "worker_task_id1"
    }
    """
    When "worker_task_id1" acquired and finished by worker

    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "deletionProtection": false
    }
    """


Scenario: delete_protection works
    #
    # ENABLE deletion_protection
    #
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "deletionProtection": true
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify Data Proc cluster"
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "deletionProtection": true
    }
    """
    #
    # TEST deletion_protection
    #
    When we DELETE "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 422
    #
    # DISABLE deletion_protection
    #
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "deletionProtection": false
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify Data Proc cluster"
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "deletionProtection": false
    }
    """
    #
    # TEST deletion_protection
    #
    When we DELETE "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200

Scenario: Resource Reaper can overcome delete_protection
    #
    # ENABLE deletion_protection
    #
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "deletionProtection": true
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify Data Proc cluster"
    }
    """

    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "deletionProtection": true
    }
    """
    #
    # TEST deletion_protection
    #
    When default headers with "resource-reaper-service-token" token
    And we DELETE "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200

