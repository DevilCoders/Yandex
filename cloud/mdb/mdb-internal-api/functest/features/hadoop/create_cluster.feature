Feature: Create Compute Hadoop Cluster

  Background:
    Given feature flags
    """
    ["MDB_DATAPROC_UI_PROXY"]
    """
    Given default headers
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
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
        },
        "uiProxy": true
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

  @events
  Scenario: Cluster creation works
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "hadoop": {
                "services": ["HDFS", "YARN", "MAPREDUCE", "TEZ", "ZOOKEEPER", "HBASE", "HIVE", "SQOOP", "FLUME", "SPARK", "ZEPPELIN"],
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
            "versionId": "1.4"
        },
        "description": "test cluster",
        "environment": "PRODUCTION",
        "folderId": "folder1",
        "id": "cid1",
        "labels": {
           "foo": "bar",
           "mycluster": "42"
        },
        "name": "test",
        "networkId": "network1",
        "status": "RUNNING",
        "zoneId": "myt",
        "serviceAccountId": "service_account_1",
        "bucket": "user_s3_bucket",
        "uiProxy": true
    }
    """
    And for "worker_task_id1" exists "yandex.cloud.events.dataproc.CreateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters"
    Then we get response with status 200 and body contains
    """
    {
        "subclusters": [
            {
                "id": "subcid1",
                "clusterId": "cid1",
                "createdAt": "2000-01-01T00:00:00+00:00",
                "name": "main",
                "hostsCount": 1,
                "resources": {
                    "diskSize": 16106127360,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.1"
                },
                "role": "MASTERNODE",
                "assignPublicIp": false,
                "subnetId": "network1-myt"
            },
            {
                "id": "subcid2",
                "clusterId": "cid1",
                "createdAt": "2000-01-01T00:00:00+00:00",
                "name": "data",
                "hostsCount": 5,
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.1"
                },
                "role": "DATANODE",
                "assignPublicIp": false,
                "subnetId": "network1-myt"
            }
        ]
    }
    """

  Scenario Outline: Cluster health is taken from dataproc-manager
    Given dataproc manager cluster health response
    """
    {
        "health": "<dm-health>"
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "health": "<api-health>"
    }
    """
    Examples:
      | dm-health | api-health |
      | ALIVE     | ALIVE      |
      | DEAD      | DEAD       |
      | DEGRADED  | DEGRADED   |
      | UNKNOWN   | UNKNOWN    |


  @events
  Scenario: Resources modify on network-ssd disk works
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "subclustersSpec": [
                {
                    "id": "subcid1",
                    "resources": {
                        "diskSize": 21474836480
                    }
                },
                {
                    "id": "subcid2",
                    "resources": {
                        "diskSize": 21474836480
                    }
                }
            ]
        }
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
    And for "worker_task_id2" exists "yandex.cloud.events.dataproc.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we run query
    """
    UPDATE dbaas.subclusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters"
    Then we get response with status 200 and body contains
    """
    {
        "subclusters": [
            {
                "id": "subcid1",
                "clusterId": "cid1",
                "createdAt": "2000-01-01T00:00:00+00:00",
                "name": "main",
                "hostsCount": 1,
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.1"
                },
                "role": "MASTERNODE",
                "assignPublicIp": false,
                "subnetId": "network1-myt"
            },
            {
                "id": "subcid2",
                "clusterId": "cid1",
                "createdAt": "2000-01-01T00:00:00+00:00",
                "name": "data",
                "hostsCount": 5,
                "resources": {
                    "diskSize": 21474836480,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.1"
                },
                "role": "DATANODE",
                "assignPublicIp": false,
                "subnetId": "network1-myt"
            }
        ]
    }
    """

  @stop @events
  Scenario: Stop cluster works
    When we POST "/mdb/hadoop/1.0/clusters/cid1:stop"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Stop Data Proc cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.StopClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.dataproc.StopCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/1.0/operations/worker_task_id2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Stop Data Proc cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.StopClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STOPPING"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STOPPED"
    }
    """

  @stop
  Scenario: Modifying stopped cluster fails
    When we POST "/mdb/hadoop/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "labels": {
            "labels-are-not-metadata-only-for-hadoop-clusters": "yes"
         }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @stop
  Scenario: Stop stopped cluster fails
    When we POST "/mdb/hadoop/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/hadoop/1.0/clusters/cid1:stop"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @start @events
  Scenario: Start cluster works
    When we POST "/mdb/hadoop/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/hadoop/1.0/clusters/cid1:start"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start Data Proc cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.StartClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.dataproc.StartCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STARTING"
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """

  @start
  Scenario: Start running cluster fails
    When we POST "/mdb/hadoop/1.0/clusters/cid1:start"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  Scenario: Cluster list works
    When we run query
    """
    UPDATE dbaas.clusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/hadoop/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
      "clusters": [
        {
          "config": {
              "versionId": "1.4",
              "hadoop": {
                  "services": ["HDFS", "YARN", "MAPREDUCE", "TEZ", "ZOOKEEPER", "HBASE", "HIVE", "SQOOP", "FLUME", "SPARK", "ZEPPELIN"],
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
              }
          },
          "createdAt": "2000-01-01T00:00:00+00:00",
          "deletionProtection": false,
          "description": "test cluster",
          "environment": "PRODUCTION",
          "folderId": "folder1",
          "health": "UNKNOWN",
          "id": "cid1",
          "labels": {
            "foo": "bar",
            "mycluster": "42"
          },
          "monitoring": [],
          "name": "test",
          "networkId": "network1",
          "status": "RUNNING",
          "zoneId": "myt",
          "securityGroupIds": [],
          "serviceAccountId": "service_account_1",
          "bucket": "user_s3_bucket",
          "uiProxy": true,
          "logGroupId": null,
          "hostGroupIds": []
        }
      ]
    }
    """

  Scenario Outline: Cluster list with invalid filter fails
    When we GET "/mdb/hadoop/1.0/clusters" with params
    """
    {
        "folderId": "folder1",
        "filter": "<filter>"
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | filter          | status | error code | message                                                                                            |
      | name = 1        | 422    | 3          | Filter 'name = 1' has wrong 'name' attribute type. Expected a string.                              |
      | my = 1          | 422    | 3          | Filter by 'my' ('my = 1') is not supported.                                                        |
      | name =          | 422    | 3          | The request is invalid.\nfilter: Filter syntax error (missing value) at or near 6.\nname =\n     ^ |
      | name < \"test\" | 501    | 12         | Operator '<' not implemented.                                                                      |

  Scenario: Host list works
    When we GET "/mdb/hadoop/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
              "name": "myt-dataproc-d-1.df.cloud.yandex.net",
              "subclusterId": "subcid2",
              "health": "ALIVE",
              "role": "DATANODE"
            },
            {
              "name": "myt-dataproc-d-2.df.cloud.yandex.net",
              "subclusterId": "subcid2",
              "health": "ALIVE",
              "role": "DATANODE"
            },
            {
              "name": "myt-dataproc-d-3.df.cloud.yandex.net",
              "subclusterId": "subcid2",
              "health": "ALIVE",
              "role": "DATANODE"
            },
            {
              "name": "myt-dataproc-d-4.df.cloud.yandex.net",
              "subclusterId": "subcid2",
              "health": "ALIVE",
              "role": "DATANODE"
            },
            {
              "name": "myt-dataproc-d-5.df.cloud.yandex.net",
              "subclusterId": "subcid2",
              "health": "ALIVE",
              "role": "DATANODE"
            },
            {
              "name": "myt-dataproc-m-1.df.cloud.yandex.net",
              "subclusterId": "subcid1",
              "health": "ALIVE",
              "role": "MASTERNODE"
            }
        ]
    }
    """


  Scenario: Host list filter works
    When we GET "/mdb/hadoop/1.0/clusters/cid1/hosts" with params
    """
    {
        "filter": "subcid='subcid1'"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
              "name": "myt-dataproc-m-1.df.cloud.yandex.net",
              "subclusterId": "subcid1",
              "health": "ALIVE",
              "role": "MASTERNODE"
            }
        ]
    }
    """


  Scenario: Host list with created compute instances
    When we run query
    """
    UPDATE dbaas.hosts
    SET vtype_id = 'compute-instance-id-1'
    WHERE fqdn = 'myt-dataproc-m-1.df.cloud.yandex.net'
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/hosts" with params
    """
    {
        "filter": "subcid='subcid1'"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
              "name": "myt-dataproc-m-1.df.cloud.yandex.net",
              "subclusterId": "subcid1",
              "computeInstanceId": "compute-instance-id-1",
              "health": "ALIVE",
              "role": "MASTERNODE"
            }
        ]
    }
    """


  Scenario Outline: Host list with invalid filter fails
    When we run query
    """
    UPDATE dbaas.hosts
    SET vtype_id = 'compute-instance-id-1'
    WHERE fqdn = 'myt-dataproc-m-1.df.cloud.yandex.net'
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/hosts" with params
    """
    {
        "filter": "<filter>"
    }
    """
    Then we get response with status 501 and body contains
    """
    {
        "code": 12,
        "message": "<message>"
    }
    """
    Examples:
      | filter                                    | message                                           |
      | name = 'main'                             | Unsupported filter field \"name\"                 |
      | subcid = 1                                | Unsupported type of filter. Type must be a string |
      | subcid = 2018-05-24T00:00+03              | Unsupported type of filter. Type must be a string |
      | subcid != 'subcid1'                       | Unsupported filter operator. Must be equals       |
      | subcid > 'subcid1'                        | Unsupported filter operator. Must be equals       |
      | subcid = 'subcid1' AND subcid = 'subcid2' | Unsupported filter. Only simple filter            |


  @events
  Scenario: Cluster name change works
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "name": "changed"
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
    And for "worker_task_id2" exists "yandex.cloud.events.dataproc.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "name": "changed"
    }
    """


  Scenario: Subcluster list works
    When we run query
    """
    UPDATE dbaas.subclusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters"
    Then we get response with status 200 and body contains
    """
    {
      "subclusters": [
          {
              "id": "subcid1",
              "clusterId": "cid1",
              "createdAt": "2000-01-01T00:00:00+00:00",
              "name": "main",
              "hostsCount": 1,
              "resources": {
                  "diskSize": 16106127360,
                  "diskTypeId": "network-ssd",
                  "resourcePresetId": "s1.compute.1"
              },
              "role": "MASTERNODE",
              "assignPublicIp": false,
              "subnetId": "network1-myt"
          },
          {
              "id": "subcid2",
              "clusterId": "cid1",
              "createdAt": "2000-01-01T00:00:00+00:00",
              "name": "data",
              "hostsCount": 5,
              "resources": {
                  "diskSize": 21474836480,
                  "diskTypeId": "network-ssd",
                  "resourcePresetId": "s1.compute.1"
              },
              "role": "DATANODE",
              "assignPublicIp": false,
              "subnetId": "network1-myt"
          }
      ]
    }
    """


  Scenario: Subcluster info works
    When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid2"
    Then we get response with status 200 and body contains
    """
    {
        "id": "subcid2",
        "clusterId": "cid1",
        "name": "data",
        "hostsCount": 5,
        "resources": {
            "diskSize": 21474836480,
            "diskTypeId": "network-ssd",
            "resourcePresetId": "s1.compute.1"
        },
        "role": "DATANODE"
    }
    """


  @events
  Scenario: Subcluster add works
  When we POST "/mdb/hadoop/1.0/clusters/cid1/subclusters" with data
  """
    {
      "name": "compute",
      "hostsCount": 3,
      "resources": {
          "diskSize": 16106127360,
          "diskTypeId": "network-ssd",
          "resourcePresetId": "s1.compute.1"
      },
      "role": "COMPUTENODE",
      "subnetId": "network1-myt"
    }
  """
  Then we get response with status 200 and body contains
  """
    {
        "createdBy": "user",
        "description": "Create Data Proc subcluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.CreateSubclusterMetadata",
            "clusterId": "cid1",
            "subclusterId": "subcid3"
        }
    }
  """
  When "worker_task_id2" acquired and finished by worker
  And we run query
    """
    UPDATE dbaas.subclusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
  When we GET "/mdb/1.0/operations/worker_task_id2"
  Then we get response with status 200 and body contains
  """
    {
        "createdBy": "user",
        "description": "Create Data Proc subcluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.CreateSubclusterMetadata",
            "clusterId": "cid1",
            "subclusterId": "subcid3"
        },
        "response": {
            "@type": "yandex.cloud.dataproc.v1.Subcluster",
            "clusterId": "cid1",
            "id": "subcid3",
            "name": "compute",
            "hostsCount": 3,
            "resources": {
                "diskSize": 16106127360,
                "diskTypeId": "network-ssd",
                "resourcePresetId": "s1.compute.1"
            },
            "role": "COMPUTENODE",
            "subnetId": "network1-myt",
            "createdAt": "2000-01-01T00:00:00+00:00"
        }
    }
  """
  And for "worker_task_id2" exists "yandex.cloud.events.dataproc.CreateSubcluster" event with
  """
    {
        "details": {
            "cluster_id": "cid1",
            "subcluster_id": "subcid3"
        }
    }
  """
  When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid3"
  Then we get response with status 200 and body contains
  """
    {
      "id": "subcid3",
      "clusterId": "cid1",
      "name": "compute",
      "hostsCount": 3,
      "resources": {
          "diskSize": 16106127360,
          "diskTypeId": "network-ssd",
          "resourcePresetId": "s1.compute.1"
      },
      "role": "COMPUTENODE"
    }
  """


  @events
  Scenario: Subcluster name modify works
  When we PATCH "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid2" with data
  """
  {
    "name": "data1"
  }
  """
  Then we get response with status 200 and body contains
  """
    {
        "createdBy": "user",
        "description": "Modify Data Proc subcluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.ModifySubclusterMetadata",
            "clusterId": "cid1",
            "subclusterId": "subcid2"
        }
    }
  """
  And for "worker_task_id2" exists "yandex.cloud.events.dataproc.UpdateSubcluster" event with
  """
    {
        "details": {
            "cluster_id": "cid1",
            "subcluster_id": "subcid2"
        }
    }
  """
  When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid2"
  Then we get response with status 200 and body contains
  """
    {
      "id": "subcid2",
      "clusterId": "cid1",
      "name": "data1",
      "hostsCount": 5,
      "resources": {
          "diskSize": 21474836480,
          "diskTypeId": "network-ssd",
          "resourcePresetId": "s1.compute.1"
      },
      "role": "DATANODE"
    }
  """

  @events
  Scenario: Subcluster delete works
  When we POST "/mdb/hadoop/1.0/clusters/cid1/subclusters" with data
  """
    {
      "name": "compute",
      "hostsCount": 3,
      "resources": {
          "diskSize": 16106127360,
          "diskTypeId": "network-ssd",
          "resourcePresetId": "s1.compute.1"
      },
      "role": "COMPUTENODE",
      "subnetId": "network1-myt"
    }
  """
  Then we get response with status 200 and body contains
  """
    {
        "createdBy": "user",
        "description": "Create Data Proc subcluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.CreateSubclusterMetadata",
            "clusterId": "cid1",
            "subclusterId": "subcid3"
        }
    }
  """
  When "worker_task_id2" acquired and finished by worker
  And we DELETE "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid3"
  Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete Data Proc subcluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.DeleteSubclusterMetadata",
            "clusterId": "cid1",
            "subclusterId": "subcid3"
        }
    }
    """
  When "worker_task_id3" acquired and finished by worker
  When we GET "/mdb/1.0/operations/worker_task_id3"
  Then we get response with status 200 and body contains
  """
    {
        "createdBy": "user",
        "description": "Delete Data Proc subcluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.DeleteSubclusterMetadata",
            "clusterId": "cid1",
            "subclusterId": "subcid3"
        },
        "response": {}
    }
  """
  And for "worker_task_id3" exists "yandex.cloud.events.dataproc.DeleteSubcluster" event with
  """
    {
        "details": {
            "cluster_id": "cid1",
            "subcluster_id": "subcid3"
        }
    }
  """
  When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid3"
  Then we get response with status 404 and body contains
  """
    {
        "code": 5,
        "message": "Subcluster 'subcid3' does not exist"
    }
  """
  When we run query
  """
  UPDATE dbaas.subclusters
  SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
  """
  When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters"
  Then we get response with status 200 and body contains
  """
    {
      "subclusters": [
          {
              "id": "subcid1",
              "clusterId": "cid1",
              "createdAt": "2000-01-01T00:00:00+00:00",
              "name": "main",
              "hostsCount": 1,
              "resources": {
                  "diskSize": 16106127360,
                  "diskTypeId": "network-ssd",
                  "resourcePresetId": "s1.compute.1"
              },
              "role": "MASTERNODE",
              "assignPublicIp": false,
              "subnetId": "network1-myt"
          },
          {
              "id": "subcid2",
              "clusterId": "cid1",
              "createdAt": "2000-01-01T00:00:00+00:00",
              "name": "data",
              "hostsCount": 5,
              "resources": {
                  "diskSize": 21474836480,
                  "diskTypeId": "network-ssd",
                  "resourcePresetId": "s1.compute.1"
              },
              "role": "DATANODE",
              "assignPublicIp": false,
              "subnetId": "network1-myt"
          }
      ]
    }
    """

  Scenario: Managed config fails
    When we GET "/api/v1.0/config/myt-dataproc-m-1.df.cloud.yandex.net"
    Then we get response with status 200 and body contains
    """
    {}
    """

  Scenario: Unmanaged config handler works properly
    When we POST "/mdb/hadoop/1.0/clusters/cid1/subclusters" with data
    """
        {
        "name": "compute",
        "hostsCount": 3,
        "resources": {
            "diskSize": 16106127360,
            "diskTypeId": "network-ssd",
            "resourcePresetId": "s1.compute.1"
        },
        "role": "COMPUTENODE",
        "subnetId": "network1-myt"
        }
    """
    Then we get response with status 200
    When we GET "/api/v1.0/config_unmanaged/myt-dataproc-m-1.df.cloud.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "ssh_authorized_keys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"],
        "data": {
            "agent": {
                "cid": "cid1",
                "manager_url": "test-dataproc-manager-public-url"
            },
            "services": ["hdfs", "yarn", "mapreduce", "tez", "zookeeper", "hbase", "hive", "sqoop", "flume", "spark", "zeppelin"],
            "ssh_public_keys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"],
            "properties": {
                "hdfs": {
                    "dfs.replication": 1
                },
                "yarn": {
                    "yarn.timeline-service.generic-application-history.enabled": "true",
                    "yarn.nodemanager.aux-services.mapreduce_shuffle.class": "org.apache.hadoop.mapred.ShuffleHandler"
                },
                "hive": {
                    "hive.server2.metrics.enabled": "true"
                },
                "capacity-scheduler": {
                    "yarn.scheduler.capacity.maximum-am-resource-percent": "1.0"
                },
                "hivemetastore": {
                    "hive.metastore.schema.verification": "true"
                },
                "hiveserver2": {
                    "hive.server2.metrics.enabled": "true"
                },
                "dataproc": {
                    "max-concurrent-jobs": 13
                },
                "spark": {
                    "spark.history.fs.cleaner.maxAge": "3d"
                }
            },
            "image": "image_id_1.4.1",
            "subcluster_main_id": "subcid1",
            "service_account_id": "service_account_1",
            "s3_bucket": "user_s3_bucket",
            "ui_proxy": true,
            "topology": {
                "network_id": "network1",
                "revision": 4,
                "zone_id": "myt",
                "subclusters": {
                    "subcid1": {
                        "cid": "cid1",
                        "name": "main",
                        "resources": {
                            "resource_preset_id": "s1.compute.1",
                            "disk_type_id": "network-ssd",
                            "disk_size": 16106127360,
                            "memory": 4294967296,
                            "cores": 1,
                            "core_fraction": 100
                        },
                        "hosts": [
                            "myt-dataproc-m-1.df.cloud.yandex.net"
                        ],
                        "role": "hadoop_cluster.masternode",
                        "services": ["hdfs", "yarn", "mapreduce", "zookeeper", "hbase", "hive","sqoop", "spark", "zeppelin"],
                        "hosts_count": 1,
                        "subcid": "subcid1",
                        "assign_public_ip": false,
                        "subnet_id": "network1-myt"
                    },
                    "subcid2": {
                        "cid": "cid1",
                        "name": "data",
                        "resources": {
                            "resource_preset_id": "s1.compute.1",
                            "disk_type_id": "network-ssd",
                            "disk_size": 21474836480,
                            "memory": 4294967296,
                            "cores": 1,
                            "core_fraction": 100
                        },
                        "hosts": [
                            "myt-dataproc-d-1.df.cloud.yandex.net",
                            "myt-dataproc-d-2.df.cloud.yandex.net",
                            "myt-dataproc-d-3.df.cloud.yandex.net",
                            "myt-dataproc-d-4.df.cloud.yandex.net",
                            "myt-dataproc-d-5.df.cloud.yandex.net"
                        ],
                        "role": "hadoop_cluster.datanode",
                        "services": ["hdfs", "yarn", "mapreduce", "tez", "hbase", "spark", "flume"],
                        "hosts_count": 5,
                        "subcid": "subcid2",
                        "assign_public_ip": false,
                        "subnet_id": "network1-myt"
                    },
                    "subcid3": {
                        "cid": "cid1",
                        "name": "compute",
                        "resources": {
                            "resource_preset_id": "s1.compute.1",
                            "disk_type_id": "network-ssd",
                            "disk_size": 16106127360,
                            "memory": 4294967296,
                            "cores": 1,
                            "core_fraction": 100
                        },
                        "hosts": [
                            "myt-dataproc-c-1.df.cloud.yandex.net",
                            "myt-dataproc-c-2.df.cloud.yandex.net",
                            "myt-dataproc-c-3.df.cloud.yandex.net"
                        ],
                        "role": "hadoop_cluster.computenode",
                        "services": ["yarn", "mapreduce", "tez", "flume"],
                        "hosts_count": 3,
                        "subcid": "subcid3",
                        "subnet_id": "network1-myt"
                    }
                }
            },
            "version": "1.4.1",
            "version_prefix": "1.4",
            "labels": {
                "subcid1": {
                    "cloud_id": "cloud1",
                    "cluster_id": "cid1",
                    "folder_id": "folder1",
                    "foo": "bar",
                    "mycluster": "42",
                    "subcluster_id": "subcid1",
                    "subcluster_role": "masternode"
                },
                "subcid2": {
                    "cloud_id": "cloud1",
                    "cluster_id": "cid1",
                    "folder_id": "folder1",
                    "foo": "bar",
                    "mycluster": "42",
                    "subcluster_id": "subcid2",
                    "subcluster_role": "datanode"
                },
                "subcid3": {
                    "cloud_id": "cloud1",
                    "cluster_id": "cid1",
                    "folder_id": "folder1",
                    "foo": "bar",
                    "mycluster": "42",
                    "subcluster_id": "subcid3",
                    "subcluster_role": "computenode"
                }
            }
        }
    }
    """

  Scenario: Topology handler works properly
  When we GET "/mdb/hadoop/1.0/clusters/cid1/topology"
  Then we get response with status 200 and body contains
  """
  {
    "revision": 3,
    "folder_id": "folder1",
    "services": ["hdfs", "yarn", "mapreduce", "tez", "zookeeper", "hbase", "hive", "sqoop", "flume", "spark", "zeppelin"],
    "subcluster_main": {
      "cid": "cid1",
      "subcid": "subcid1",
      "name": "main",
      "services": ["hdfs", "yarn", "mapreduce", "zookeeper", "hbase", "hive", "sqoop", "spark", "zeppelin"],
      "hosts_count": 1,
      "resources": {
        "disk_size": 16106127360,
        "disk_type_id": "network-ssd",
        "resource_preset_id": "s1.compute.1",
        "cores": 1.0,
        "core_fraction": 100,
        "memory": 4294967296
      },
      "hosts": [
        "myt-dataproc-m-1.df.cloud.yandex.net"
      ],
      "role": "hadoop_cluster.masternode",
      "subnet_id": "network1-myt",
      "assign_public_ip": false
    },
    "subclusters": [
      {
        "cid": "cid1",
        "subcid": "subcid2",
        "name": "data",
        "services": ["hdfs", "yarn", "mapreduce", "tez", "hbase", "spark", "flume"],
        "hosts_count": 5,
        "hosts": [
          "myt-dataproc-d-1.df.cloud.yandex.net",
          "myt-dataproc-d-2.df.cloud.yandex.net",
          "myt-dataproc-d-3.df.cloud.yandex.net",
          "myt-dataproc-d-4.df.cloud.yandex.net",
          "myt-dataproc-d-5.df.cloud.yandex.net"
        ],
        "resources": {
          "disk_size": 21474836480,
          "disk_type_id": "network-ssd",
          "resource_preset_id": "s1.compute.1",
          "cores": 1.0,
          "core_fraction": 100,
          "memory": 4294967296
        },
        "role": "hadoop_cluster.datanode",
        "assign_public_ip": false,
        "subnet_id": "network1-myt"
      }
    ],
    "ui_proxy": true
  }
  """

  @delete @events
  Scenario: Cluster removal works
    When we DELETE "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete Data Proc cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.DeleteClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.dataproc.DeleteCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": []
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 403


  @delete
  Scenario: Broken cluster removal works
    When we DELETE "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete Data Proc cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.DeleteClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": []
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 403
