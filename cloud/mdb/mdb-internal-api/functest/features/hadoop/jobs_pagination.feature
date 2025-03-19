Feature: Pagination for Compute Hadoop jobs

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
          "services": ["HDFS", "YARN", "MAPREDUCE", "TEZ", "ZOOKEEPER", "HBASE", "HIVE", "SQOOP", "FLUME", "SPARK"],
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
            "subnetId": "network1-myt-with-nat"
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
      "bucket": "bucket-for-job-output"
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
    Given health report is received from dataproc agent
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "mapreduceJob": {
        "args": ["arg1", "arg2"],
        "jarFileUris": ["hdfs://jobs/word_count.jar"],
        "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
        "archiveUris": ["hdfs://jobs/word_count.zip"],
        "properties": {
          "prop1": "prop_value1",
          "prop2": 2
        },
        "mainJarFileUri" : "hdfs://jobs/word_count.jar"
      }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "createdBy": "user",
      "description": "Create Data Proc job",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "yandex.cloud.dataproc.v1.CreateJobMetadata",
        "clusterId": "cid1",
        "jobId": "hadoop_job_id1"
      }
    }
    """
    Given health report is received from dataproc agent
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "mapreduceJob": {
        "args": ["arg1", "arg2"],
        "jarFileUris": ["hdfs://jobs/word_count.jar"],
        "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
        "archiveUris": ["hdfs://jobs/word_count.zip"],
        "properties": {
          "prop1": "prop_value1",
          "prop2": 2
        },
        "mainJarFileUri" : "hdfs://jobs/word_count.jar"
      }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "createdBy": "user",
      "description": "Create Data Proc job",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "yandex.cloud.dataproc.v1.CreateJobMetadata",
        "clusterId": "cid1",
        "jobId": "hadoop_job_id2"
      }
    }
    """
    Given health report is received from dataproc agent
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "mapreduceJob": {
        "args": ["arg1", "arg2"],
        "jarFileUris": ["hdfs://jobs/word_count.jar"],
        "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
        "archiveUris": ["hdfs://jobs/word_count.zip"],
        "properties": {
          "prop1": "prop_value1",
          "prop2": 2
        },
        "mainJarFileUri" : "hdfs://jobs/word_count.jar"
      }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "createdBy": "user",
      "description": "Create Data Proc job",
      "done": false,
      "id": "worker_task_id4",
      "metadata": {
        "@type": "yandex.cloud.dataproc.v1.CreateJobMetadata",
        "clusterId": "cid1",
        "jobId": "hadoop_job_id3"
      }
    }
    """
    Given health report is received from dataproc agent
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "mapreduceJob": {
        "args": ["arg1", "arg2"],
        "jarFileUris": ["hdfs://jobs/word_count.jar"],
        "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
        "archiveUris": ["hdfs://jobs/word_count.zip"],
        "properties": {
          "prop1": "prop_value1",
          "prop2": 2
        },
        "mainJarFileUri" : "hdfs://jobs/word_count.jar"
      }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "createdBy": "user",
      "description": "Create Data Proc job",
      "done": false,
      "id": "worker_task_id5",
      "metadata": {
        "@type": "yandex.cloud.dataproc.v1.CreateJobMetadata",
        "clusterId": "cid1",
        "jobId": "hadoop_job_id4"
      }
    }
    """
    Given health report is received from dataproc agent
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "mapreduceJob": {
        "args": ["arg1", "arg2"],
        "jarFileUris": ["hdfs://jobs/word_count.jar"],
        "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
        "archiveUris": ["hdfs://jobs/word_count.zip"],
        "properties": {
          "prop1": "prop_value1",
          "prop2": 2
        },
        "mainJarFileUri" : "hdfs://jobs/word_count.jar"
      }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "createdBy": "user",
      "description": "Create Data Proc job",
      "done": false,
      "id": "worker_task_id6",
      "metadata": {
        "@type": "yandex.cloud.dataproc.v1.CreateJobMetadata",
        "clusterId": "cid1",
        "jobId": "hadoop_job_id5"
      }
    }
    """
    When we run query
    """
    UPDATE dbaas.hadoop_jobs
    SET create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """

  Scenario: Page limit for cluster jobs list works
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs?pageSize=3"
    Then we get response with status 200 and body contains
    """
    {
      "nextPageToken": "MjAwMC0wMS0wMSAwMDowMDowMCswMDowMD9oYWRvb3Bfam9iX2lkMw==",
      "jobs": [
        {
          "id": "hadoop_job_id5",
          "clusterId": "cid1",
          "name": "test job",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": null,
          "finishedAt": null,
          "status": "PROVISIONING",
          "applicationInfo": null,
          "mapreduceJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri" : "hdfs://jobs/word_count.jar"
          }
        },
        {
          "id": "hadoop_job_id4",
          "clusterId": "cid1",
          "name": "test job",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": null,
          "finishedAt": null,
          "status": "PROVISIONING",
          "applicationInfo": null,
          "mapreduceJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri" : "hdfs://jobs/word_count.jar"
          }
        },
        {
          "id": "hadoop_job_id3",
          "clusterId": "cid1",
          "name": "test job",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": null,
          "finishedAt": null,
          "status": "PROVISIONING",
          "applicationInfo": null,
          "mapreduceJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri" : "hdfs://jobs/word_count.jar"
          }
        }
      ]
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs?pageSize=3&pageToken=MjAwMC0wMS0wMSAwMDowMDowMCswMDowMD9oYWRvb3Bfam9iX2lkMw=="
    Then we get response with status 200 and body contains
    """
    {
      "jobs": [
        {
          "id": "hadoop_job_id2",
          "clusterId": "cid1",
          "name": "test job",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": null,
          "finishedAt": null,
          "status": "PROVISIONING",
          "applicationInfo": null,
          "mapreduceJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri" : "hdfs://jobs/word_count.jar"
          }
        },
        {
          "id": "hadoop_job_id1",
          "clusterId": "cid1",
          "name": "test job",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": null,
          "finishedAt": null,
          "status": "PROVISIONING",
          "applicationInfo": null,
          "mapreduceJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri" : "hdfs://jobs/word_count.jar"
          }
        }
      ]
    }
    """

  Scenario: Page limit for service jobs list works
    When we GET "/mdb/hadoop/1.0/jobs?pageSize=3"
    Then we get response with status 200 and body contains
    """
    {
      "nextPageToken": "MjAwMC0wMS0wMSAwMDowMDowMCswMDowMD9oYWRvb3Bfam9iX2lkMw==",
      "jobs": [
        {
          "id": "hadoop_job_id1",
          "clusterId": "cid1",
          "name": "test job",
          "status": "PROVISIONING",
          "applicationInfo": null,
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": null,
          "finishedAt": null,
          "mapreduceJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri" : "hdfs://jobs/word_count.jar"
          }
        },
        {
          "id": "hadoop_job_id2",
          "clusterId": "cid1",
          "name": "test job",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": null,
          "finishedAt": null,
          "status": "PROVISIONING",
          "applicationInfo": null,
          "mapreduceJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri" : "hdfs://jobs/word_count.jar"
          }
        },
        {
          "id": "hadoop_job_id3",
          "clusterId": "cid1",
          "name": "test job",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": null,
          "finishedAt": null,
          "status": "PROVISIONING",
          "applicationInfo": null,
          "mapreduceJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri" : "hdfs://jobs/word_count.jar"
          }
        }
      ]
    }
    """
    When we GET "/mdb/hadoop/1.0/jobs?pageSize=3&pageToken=MjAwMC0wMS0wMSAwMDowMDowMCswMDowMD9oYWRvb3Bfam9iX2lkMw=="
    Then we get response with status 200 and body contains
    """
    {
      "jobs": [
        {
          "id": "hadoop_job_id4",
          "clusterId": "cid1",
          "name": "test job",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": null,
          "finishedAt": null,
          "status": "PROVISIONING",
          "applicationInfo": null,
          "mapreduceJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri" : "hdfs://jobs/word_count.jar"
          }
        },
        {
          "id": "hadoop_job_id5",
          "clusterId": "cid1",
          "name": "test job",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": null,
          "finishedAt": null,
          "status": "PROVISIONING",
          "applicationInfo": null,
          "mapreduceJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri" : "hdfs://jobs/word_count.jar"
          }
        }
      ]
    }
    """
