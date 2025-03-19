Feature: Create/Modify Compute Hadoop jobs

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
    When we run query
    """
    UPDATE dbaas.hadoop_jobs
    SET create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """

  Scenario: Job info works
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1"
    Then we get response with status 200 and body contains
    """
    {
      "id": "hadoop_job_id1",
      "clusterId": "cid1",
      "name": "test job",
      "status": "PROVISIONING",
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

  Scenario: Get job with wrong job id
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs/wrong_job_id"
    Then we get response with status 404 and body contains
    """
    {
        "message": "Data Proc job 'wrong_job_id' does not exist"
    }
    """

  Scenario: Job change status works
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1"
    Then we get response with status 200 and body contains
    """
    {
      "id": "hadoop_job_id1",
      "clusterId": "cid1",
      "name": "test job",
      "createdBy": "user",
      "createdAt": "2000-01-01T00:00:00+00:00",
      "startedAt": null,
      "finishedAt": null,
      "status": "PROVISIONING",
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
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1:updateStatus" with data
    """
    {
      "status": "RUNNING",
      "application_info": {
        "application_attempts": [
          {
            "id": "appattempt_1633344885833_0006_000001",
            "am_container_id": "container_1633344885833_0006_01_000001"
          }
        ],
        "id": "application_1633344885833_0006"
      }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "clusterId": "cid1",
      "jobId": "hadoop_job_id1",
      "status": "RUNNING"
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1"
    Then we get response with status 200 and body contains
    """
    {
      "id": "hadoop_job_id1",
      "clusterId": "cid1",
      "name": "test job",
      "status": "RUNNING",
      "applicationInfo": {
        "applicationAttempts": [
          {
            "id": "appattempt_1633344885833_0006_000001",
            "amContainerId": "container_1633344885833_0006_01_000001"
          }
        ],
        "id": "application_1633344885833_0006"
      },
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
    When we GET "/mdb/hadoop/1.0/operations/worker_task_id2"
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
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1:updateStatus" with data
    """
    {
      "status": "DONE"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "clusterId": "cid1",
      "jobId": "hadoop_job_id1",
      "status": "DONE"
    }
    """
    When we GET "/mdb/hadoop/1.0/operations/worker_task_id2"
    Then we get response with status 200 and body contains
    """
    {
      "createdBy": "user",
      "description": "Create Data Proc job",
      "done": true,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "yandex.cloud.dataproc.v1.CreateJobMetadata",
        "clusterId": "cid1",
        "jobId": "hadoop_job_id1"
      }
    }
    """

  Scenario: Jobs list for cluster works
    Given health report is received from dataproc agent
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "sparkJob": {
        "args": ["arg1", "arg2"],
        "jarFileUris": ["hdfs://jobs/word_count.jar"],
        "fileUris": ["hdfs://jobs/word_count"],
        "archiveUris": ["hdfs://jobs/word_count.zip"],
        "properties": {
          "prop1": "prop_value1",
          "prop2": 2
        },
        "mainJarFileUri": "hdfs://jobs/word_count",
        "mainClass": "WordCount"
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
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id2:updateStatus" with data
    """
    {
      "status": "RUNNING",
      "application_info": {
        "application_attempts": [
          {
            "id": "appattempt_1633344885833_0006_000001",
            "am_container_id": "container_1633344885833_0006_01_000001"
          }
        ],
        "id": "application_1633344885833_0006"
      }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "clusterId": "cid1",
      "jobId": "hadoop_job_id2",
      "status": "RUNNING"
    }
    """
    When we run query
    """
    UPDATE dbaas.hadoop_jobs
    SET create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we run query
    """
    UPDATE dbaas.hadoop_jobs
    SET start_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    WHERE status='RUNNING'
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs"
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
          "startedAt": "2000-01-01T00:00:00+00:00",
          "finishedAt": null,
          "status": "RUNNING",
          "applicationInfo": {
            "applicationAttempts": [
              {
                "id": "appattempt_1633344885833_0006_000001",
                "amContainerId": "container_1633344885833_0006_01_000001"
              }
            ],
            "id": "application_1633344885833_0006"
          },
          "sparkJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri": "hdfs://jobs/word_count",
            "mainClass": "WordCount"
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
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1:updateStatus" with data
    """
    {
      "status": "ERROR"
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs?filters=status='active'"
    Then we get response with status 200 and body contains
    """
    {
      "jobs": [
        {
          "id": "hadoop_job_id2",
          "clusterId": "cid1",
          "name": "test job",
          "applicationInfo": {
            "applicationAttempts": [
              {
                "id": "appattempt_1633344885833_0006_000001",
                "amContainerId": "container_1633344885833_0006_01_000001"
              }
            ],
            "id": "application_1633344885833_0006"
          },
          "status": "RUNNING",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": "2000-01-01T00:00:00+00:00",
          "finishedAt": null,
          "sparkJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri": "hdfs://jobs/word_count",
            "mainClass": "WordCount"
          }
        }
      ]
    }
    """
  Scenario: Jobs list for service works
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
      "folderId": "folder1",
      "name": "test2",
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
      "serviceAccountId": "service_account_1"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "createdBy": "user",
      "description": "Create Data Proc cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "yandex.cloud.dataproc.v1.CreateClusterMetadata",
        "clusterId": "cid2"
      }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    Given health report is received from dataproc agent
    When we POST "/mdb/hadoop/1.0/clusters/cid2/jobs" with data
    """
    {
      "name": "test job",
      "hiveJob": {
        "properties": {
          "prop1": "prop_value1",
          "prop2": 2
        },
        "continueOnFailure": true,
        "scriptVariables": {
          "var1": "val1",
          "var2": "val2"
        },
        "jarFileUris": ["hdfs://jobs/word_count.jar"],
        "queryList": {
          "queries": [
            "SELECT 1;",
            "SELECT 2;"
          ]
        }
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
        "clusterId": "cid2",
        "jobId": "hadoop_job_id2"
      }
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1"
    Then we get response with status 200 and body contains
    """
    {
      "id": "hadoop_job_id1",
      "clusterId": "cid1",
      "name": "test job",
      "applicationInfo": null,
      "status": "PROVISIONING",
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
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id2"
    Then we get response with status 200 and body contains
    """
    {
      "id": "hadoop_job_id2",
      "clusterId": "cid2",
      "name": "test job",
      "applicationInfo": null,
      "status": "PROVISIONING",
      "hiveJob": {
        "properties": {
          "prop1": "prop_value1",
          "prop2": 2
        },
        "continueOnFailure": true,
        "scriptVariables": {
          "var1": "val1",
          "var2": "val2"
        },
        "jarFileUris": ["hdfs://jobs/word_count.jar"],
        "queryList": {
          "queries": [
            "SELECT 1;",
            "SELECT 2;"
          ]
        }
      }
    }
    """
    When we GET "/mdb/hadoop/1.0/jobs?cluster_id=cid1"
    Then we get response with status 200 and body contains
    """
    {
      "jobs": [
        {
          "id": "hadoop_job_id1",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": null,
          "finishedAt": null,
          "clusterId": "cid1",
          "name": "test job",
          "applicationInfo": null,
          "status": "PROVISIONING",
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
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1:updateStatus" with data
    """
    {
      "status": "RUNNING"
    }
    """
    And we run query
    """
    UPDATE dbaas.hadoop_jobs
    SET start_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    WHERE job_id = 'hadoop_job_id1'
    """
    When we GET "/mdb/hadoop/1.0/jobs?filters=status%20NOT%20IN%20('provisioning')"
    Then we get response with status 200 and body contains
    """
    {
      "jobs": [
        {
          "id": "hadoop_job_id1",
          "clusterId": "cid1",
          "name": "test job",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": "2000-01-01T00:00:00+00:00",
          "finishedAt": null,
          "applicationInfo": null,
          "status": "RUNNING",
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

  Scenario: Job cancel returns operation
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1:cancel"
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
  
  Scenario: Cancel job with wrong job id
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs/wrong_job_id:cancel"
    Then we get response with status 404 and body contains
    """
    {
        "message": "Data Proc job 'wrong_job_id' does not exist"
    }
    """

  Scenario: Job cancel with different job statuses
    Given health report is received from dataproc agent
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "sparkJob": {
        "args": ["arg1", "arg2"],
        "jarFileUris": ["hdfs://jobs/word_count.jar"],
        "fileUris": ["hdfs://jobs/word_count"],
        "archiveUris": ["hdfs://jobs/word_count.zip"],
        "properties": {
          "prop1": "prop_value1",
          "prop2": 2
        },
        "mainJarFileUri": "hdfs://jobs/word_count",
        "mainClass": "WordCount"
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
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id2:updateStatus" with data
    """
    {
      "status": "RUNNING",
      "application_info": {
        "application_attempts": [
          {
            "id": "appattempt_1633344885833_0006_000001",
            "am_container_id": "container_1633344885833_0006_01_000001"
          }
        ],
        "id": "application_1633344885833_0006"
      }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "clusterId": "cid1",
      "jobId": "hadoop_job_id2",
      "status": "RUNNING"
    }
    """
    When we run query
    """
    UPDATE dbaas.hadoop_jobs
    SET create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we run query
    """
    UPDATE dbaas.hadoop_jobs
    SET start_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    WHERE status='RUNNING'
    """
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id2:cancel"
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
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs?filters=status='active'"
    Then we get response with status 200 and body contains
    """
    {
      "jobs": [
        {
          "id": "hadoop_job_id2",
          "clusterId": "cid1",
          "name": "test job",
          "applicationInfo": {
            "applicationAttempts": [
              {
                "id": "appattempt_1633344885833_0006_000001",
                "amContainerId": "container_1633344885833_0006_01_000001"
              }
            ],
            "id": "application_1633344885833_0006"
          },
          "status": "CANCELLING",
          "createdBy": "user",
          "createdAt": "2000-01-01T00:00:00+00:00",
          "startedAt": "2000-01-01T00:00:00+00:00",
          "finishedAt": null,
          "sparkJob": {
            "args": ["arg1", "arg2"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "fileUris": ["hdfs://jobs/word_count"],
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "properties": {
              "prop1": "prop_value1",
              "prop2": 2
            },
            "mainJarFileUri": "hdfs://jobs/word_count",
            "mainClass": "WordCount"
          }
        },
        {
          "id": "hadoop_job_id1",
          "clusterId": "cid1",
          "name": "test job",
          "status": "PROVISIONING",
          "startedAt": null,
          "createdAt": "2000-01-01T00:00:00+00:00",
          "finishedAt": null,
          "createdBy": "user",
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
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id2:cancel"
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
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id2:updateStatus" with data
    """
    {
      "status": "CANCELLED",
      "application_info": {
        "application_attempts": [
          {
            "id": "appattempt_1633344885833_0006_000001",
            "am_container_id": "container_1633344885833_0006_01_000001"
          }
        ],
        "id": "application_1633344885833_0006"
      }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
      "clusterId": "cid1",
      "jobId": "hadoop_job_id2",
      "status": "CANCELLED"
    }
    """
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id2:cancel"
    Then we get response with status 422 and body contains
    """
    {
    "code": 3,
    "message": "Cannot cancel terminated job with status `CANCELLED`"
    }
    """
    