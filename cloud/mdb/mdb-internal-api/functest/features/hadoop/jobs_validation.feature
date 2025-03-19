Feature: Create/Get Compute Hadoop jobs

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
          "services": ["HDFS", "YARN", "MAPREDUCE", "TEZ", "HIVE"],
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

  Scenario: Job creation with no spec fails
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
      "code": 3,
      "message": "You need to specify exactly one job spec"
    }
    """

  Scenario: Job creation with two specs fails
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
        "driver": {
          "mainJarFileUri" : "hdfs://jobs/word_count.jar"
        }
      },
      "sparkJob": {
        "args": ["arg1", "arg2"],
        "jarFileUris": ["hdfs://jobs/word_count.jar"],
        "mainClass": "WordCount"
      }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
      "code": 3,
      "message": "You need to specify exactly one job spec"
    }
    """

  Scenario: Creation of hive job with both queryFileUri and queries fails
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "hiveJob": {
        "queryFileUri": "hdfs:///query.sql",
        "queryList": {
          "queries": [
            "SELECT 1;",
            "SELECT 2;"
          ]
        }
      }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
      "code": 3,
      "message": "Exactly one of attributes \"query_file_uri\" or \"query_list\" must be specified for hive job"
    }
    """


  Scenario: Creation of hive job without queryFileUri and queries fails
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "hiveJob": {
      }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
      "code": 3,
      "message": "Exactly one of attributes \"query_file_uri\" or \"query_list\" must be specified for hive job"
    }
    """


  Scenario: Creation of mapreduce job with both mainJarFileUri and mainClass fails
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "mapreduceJob": {
        "mainJarFileUri" : "hadoop-streaming.jar",
        "mainClass" : "WordCount"
      }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
      "code": 3,
      "message": "Exactly one of attributes \"main_jar_file_uri\" or \"main_class\" must be specified for mapreduce job"
    }
    """


  Scenario: Creation of mapreduce job without mainJarFileUri and mainClass fails
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "mapreduceJob": {
      }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
      "code": 3,
      "message": "Exactly one of attributes \"main_jar_file_uri\" or \"main_class\" must be specified for mapreduce job"
    }
    """


  Scenario: Jobs list for cluster works
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
    Then we get response with status 422 and body contains
    """
    {
      "code": 3,
      "message": "To create job of this type you need cluster with services: spark"
    }
    """


  Scenario: Unable to run job if NAT is not enabled on the subnet of the main subcluster
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
            "subnetId": "network1-myt-without-nat"
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
      "id": "worker_task_id2",
      "metadata": {
        "@type": "yandex.cloud.dataproc.v1.CreateClusterMetadata",
        "clusterId": "cid2"
      }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/hadoop/1.0/clusters/cid2/jobs" with data
    """
    {
      "name": "test job",
      "hiveJob": {
        "queryList": {
          "queries": [
            "SELECT 1;",
            "SELECT 2;"
          ]
        }
      }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
      "code": 3,
      "message": "In order to run jobs subnet of the main subcluster should have NAT feature turned on."
    }
    """


  Scenario: Unable to run job if HDFS is in safemode
    Given dataproc manager cluster health response
    """
    {
        "health": "ALIVE",
        "explanation": "",
        "hdfs_in_safemode": true
    }
    """
    When we POST "/mdb/hadoop/1.0/clusters/cid1/jobs" with data
    """
    {
      "name": "test job",
      "hiveJob": {
        "queryList": {
          "queries": [
            "SELECT 1;",
            "SELECT 2;"
          ]
        }
      }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
      "code": 3,
      "message": "Unable to run jobs while HDFS is in Safemode"
    }
    """
