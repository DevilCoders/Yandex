Feature: Stop Cluster when Job is running

  Background:
    Given default headers
    When we POST "/mdb/hadoop/1.1/clusters?folderId=folder1" with data
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
      "bucket": "bucket-for-job-output",
      "s3Bucket": "user_s3_bucket"
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

@stop @events
  Scenario: Stop cluster works when the job is running
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
    When we POST "/mdb/hadoop/1.0/clusters/cid1:stop"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Stop Data Proc cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.StopClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STOPPED"
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/jobs/hadoop_job_id1"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "createdBy": "user",
        "id": "hadoop_job_id1",
        "mapreduceJob":  {
            "archiveUris": ["hdfs://jobs/word_count.zip"],
            "args": ["arg1", "arg2"],
            "fileUris": ["hdfs://jobs/word_count", "hdfs://jobs/word_count_1"],
            "jarFileUris": ["hdfs://jobs/word_count.jar"],
            "mainJarFileUri": "hdfs://jobs/word_count.jar",
            "properties": {
                "prop1": "prop_value1",
                "prop2": 2
            }
        },
        "name": "test job",
        "status": "ERROR"
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
      },
      "error": {"code": 1, "details": [{"code": 1, "message": "The job was terminated", "type": "Cancelled"}], "message": "The job was terminated"}
    }
    """

