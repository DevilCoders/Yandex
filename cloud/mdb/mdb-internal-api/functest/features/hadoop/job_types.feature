Feature: Create Data Proc jobs of all types

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


  Scenario: Pyspark job
    Given health report is received from dataproc agent
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "pyspark job",
      "pysparkJob": {
        "args": ["arg1", "arg2"],
        "jarFileUris": ["file1.jar", "file2.jar"],
        "fileUris": ["file1.txt", "file2.txt"],
        "archiveUris": ["archive1.zip", "archive2.zip"],
        "properties": {"key1": "val1", "key2": "val2"},
        "mainPythonFileUri": "main.py",
        "pythonFileUris": ["file1.py", "file2.py"],
        "packages": ["org.apache.spark:spark-streaming-kafka_2.10:1.6.0", "org.elasticsearch:elasticsearch-spark_2.10:2.2.0"],
        "repositories": ["https://oss.sonatype.org/content/groups/public/", "https://oss.sonatype.org/content/groups/public/"],
        "excludePackages": ["org.apache.spark:spark-streaming-kafka_2.10"]
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
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1"
    Then we get response with status 200 and body contains
    """
      {
        "clusterId": "cid1",
        "id": "hadoop_job_id1",
        "name": "pyspark job",
        "pysparkJob": {
            "archiveUris": ["archive1.zip", "archive2.zip"],
            "args": ["arg1", "arg2"],
            "fileUris": ["file1.txt", "file2.txt"],
            "jarFileUris": ["file1.jar", "file2.jar"],
            "mainPythonFileUri": "main.py",
            "properties": {
                "key1": "val1",
                "key2": "val2"
            },
            "pythonFileUris": ["file1.py", "file2.py"],
            "packages": ["org.apache.spark:spark-streaming-kafka_2.10:1.6.0", "org.elasticsearch:elasticsearch-spark_2.10:2.2.0"],
            "repositories": ["https://oss.sonatype.org/content/groups/public/", "https://oss.sonatype.org/content/groups/public/"],
            "excludePackages": ["org.apache.spark:spark-streaming-kafka_2.10"]
        },
        "status": "PROVISIONING",
        "applicationInfo": null
      }
    """


  Scenario: Hive job
    Given health report is received from dataproc agent
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "hive job",
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
      "id": "worker_task_id2",
      "metadata": {
        "@type": "yandex.cloud.dataproc.v1.CreateJobMetadata",
        "clusterId": "cid1",
        "jobId": "hadoop_job_id1"
      }
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "createdBy": "user",
        "finishedAt": null,
        "hiveJob": {
            "continueOnFailure": true,
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "properties": {
                "prop1": "prop_value1",
                "prop2": 2
            },
            "queryList": {
                "queries": ["SELECT 1;", "SELECT 2;"]
            },
            "scriptVariables": {
                "var1": "val1",
                "var2": "val2"
            }
        },
        "id": "hadoop_job_id1",
        "name": "hive job",
        "startedAt": null,
        "status": "PROVISIONING",
        "applicationInfo": null
    }
    """


  Scenario: Mapreduce job
    Given health report is received from dataproc agent
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "mapreduce job",
      "mapreduceJob": {
        "args": ["-mapper", "mapper.py", "-reducer", "reducer.py"],
        "archiveUris": ["archive1.zip", "archive2.zip"],
        "fileUris": ["s3a://bucket/mapper.py", "s3a://bucket/reducer.py"],
        "jarFileUris": ["file1.jar", "file2.jar"],
        "properties": {"key1": "val1", "key2": "val2"},
        "mainJarFileUri": "hadoop-streaming.jar"
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
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1"
    Then we get response with status 200 and body contains
    """
    {
      "clusterId": "cid1",
      "finishedAt": null,
      "id": "hadoop_job_id1",
      "mapreduceJob": {
        "archiveUris": ["archive1.zip", "archive2.zip"],
        "args": ["-mapper", "mapper.py", "-reducer", "reducer.py"],
        "fileUris": ["s3a://bucket/mapper.py", "s3a://bucket/reducer.py"],
        "jarFileUris": ["file1.jar", "file2.jar"],
        "mainJarFileUri": "hadoop-streaming.jar",
        "properties": {
          "key1": "val1",
          "key2": "val2"
        }
      },
      "name": "mapreduce job",
      "startedAt": null,
      "applicationInfo": null,
      "status": "PROVISIONING"
    }
    """
