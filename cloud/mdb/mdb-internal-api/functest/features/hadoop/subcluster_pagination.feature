Feature: Pagination for Hadoop Subcluster

  Background:
    Given default headers
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "test",
        "configSpec": {
            "hadoop": {
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
                    "name": "data2",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "hostsCount": 1,
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data3",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "hostsCount": 1,
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data4",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "hostsCount": 1,
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data5",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "hostsCount": 1,
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data6",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "hostsCount": 1,
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data7",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "hostsCount": 1,
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data8",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "hostsCount": 1,
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data9",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "hostsCount": 1,
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
    When we run query
    """
    UPDATE dbaas.subclusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """

  Scenario: Page limit works
    When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters?pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
      "nextPageToken": "c3ViY2lkNQ==",
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
              "name": "data2",
              "hostsCount": 1,
              "resources": {
                  "diskSize": 16106127360,
                  "diskTypeId": "network-ssd",
                  "resourcePresetId": "s1.compute.1"
              },
              "role": "DATANODE",
              "assignPublicIp": false,
              "subnetId": "network1-myt"
          },
          {
              "id": "subcid3",
              "clusterId": "cid1",
              "createdAt": "2000-01-01T00:00:00+00:00",
              "name": "data3",
              "hostsCount": 1,
              "resources": {
                  "diskSize": 16106127360,
                  "diskTypeId": "network-ssd",
                  "resourcePresetId": "s1.compute.1"
              },
              "role": "DATANODE",
              "assignPublicIp": false,
              "subnetId": "network1-myt"
          },
          {
              "id": "subcid4",
              "clusterId": "cid1",
              "createdAt": "2000-01-01T00:00:00+00:00",
              "name": "data4",
              "hostsCount": 1,
              "resources": {
                  "diskSize": 16106127360,
                  "diskTypeId": "network-ssd",
                  "resourcePresetId": "s1.compute.1"
              },
              "role": "DATANODE",
              "assignPublicIp": false,
              "subnetId": "network1-myt"
          },
          {
              "id": "subcid5",
              "clusterId": "cid1",
              "createdAt": "2000-01-01T00:00:00+00:00",
              "name": "data5",
              "hostsCount": 1,
              "resources": {
                  "diskSize": 16106127360,
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

  Scenario: Next page token works
    When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters?pageSize=3&pageToken=c3ViY2lkNQ=="
    Then we get response with status 200 and body contains
    """
    {
      "nextPageToken": "c3ViY2lkOA==",
      "subclusters": [
          {
              "id": "subcid6",
              "clusterId": "cid1",
              "createdAt": "2000-01-01T00:00:00+00:00",
              "name": "data6",
              "hostsCount": 1,
              "resources": {
                  "diskSize": 16106127360,
                  "diskTypeId": "network-ssd",
                  "resourcePresetId": "s1.compute.1"
              },
              "role": "DATANODE",
              "assignPublicIp": false,
              "subnetId": "network1-myt"
          },
          {
              "id": "subcid7",
              "clusterId": "cid1",
              "createdAt": "2000-01-01T00:00:00+00:00",
              "name": "data7",
              "hostsCount": 1,
              "resources": {
                  "diskSize": 16106127360,
                  "diskTypeId": "network-ssd",
                  "resourcePresetId": "s1.compute.1"
              },
              "role": "DATANODE",
              "assignPublicIp": false,
              "subnetId": "network1-myt"
          },
          {
              "id": "subcid8",
              "clusterId": "cid1",
              "createdAt": "2000-01-01T00:00:00+00:00",
              "name": "data8",
              "hostsCount": 1,
              "resources": {
                  "diskSize": 16106127360,
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
