Feature: Modify Compute Hadoop Cluster

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
                }
            ]
        },
        "description": "test cluster",
        "zoneId": "myt",
        "serviceAccountId": "service_account_1",
        "labels" : {"foo": "bar"}
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

  @labels
  # Test for https://st.yandex-team.ru/MDB-6893
  Scenario: Create subcluster after modifying labels on cluster
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "labels": {
            "foo2": "bar2"
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
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "labels": {"foo2":"bar2"}
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/hadoop/1.0/clusters/cid1/subclusters" with data
    """
    {
        "name": "compute",
        "role": "COMPUTENODE",
        "resources": {
            "resourcePresetId": "s1.compute.2",
            "diskTypeId": "network-ssd",
            "diskSize": 16106127360
        },
        "hostsCount": 5,
        "subnetId": "network1-myt"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create Data Proc subcluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.CreateSubclusterMetadata",
            "clusterId": "cid1",
            "subclusterId": "subcid3"
        }
    }
    """

  @labels
  # Test for https://st.yandex-team.ru/MDB-6916
  Scenario: Labels are not cleared up on cluster update
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "description": "updated description"
    }
    """
    Then we get response with status 200
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "updated description",
        "labels": {"foo":"bar"}
    }
    """


  Scenario: Modify to burstable on multi-host cluster
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "subclustersSpec": [
                {
                    "id": "subcid2",
                    "resources": {
                        "resourcePresetId": "b2.compute.3",
                        "diskTypeId": "network-ssd"
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
                    "resourcePresetId": "s1.compute.2"
                },
                "role": "DATANODE",
                "assignPublicIp": false,
                "subnetId": "network1-myt"
            }
        ]
    }
    """

  Scenario: Resource presets modify on cluster subclusters works
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "subclustersSpec": [
                {
                    "id": "subcid1",
                    "resources": {
                        "resourcePresetId": "s1.compute.2"
                    }
                },
                {
                    "id": "subcid2",
                    "resources": {
                        "resourcePresetId": "s1.compute.3"
                    },
                    "hostsCount": 6
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
                    "diskSize": 16106127360,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.2"
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
                "hostsCount": 6,
                "resources": {
                    "diskSize": 16106127360,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.3"
                },
                "role": "DATANODE",
                "assignPublicIp": false,
                "subnetId": "network1-myt"
            }
        ]
    }
    """
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
            "properties": {},
            "image": "image_id_1.4.1",
            "subcluster_main_id": "subcid1",
            "service_account_id": "service_account_1",
            "ui_proxy": false,
            "topology": {
                "revision": 5,
                "network_id": "network1",
                "zone_id": "myt",
                "subclusters": {
                    "subcid1": {
                        "cid": "cid1",
                        "name": "main",
                        "resources": {
                            "resource_preset_id": "s1.compute.2",
                            "disk_type_id": "network-ssd",
                            "disk_size": 16106127360,
                            "memory": 8589934592,
                            "cores": 2,
                            "core_fraction": 100
                        },
                        "hosts": [
                            "myt-dataproc-m-1.df.cloud.yandex.net"
                        ],
                        "role": "hadoop_cluster.masternode",
                        "services": ["hdfs", "yarn", "mapreduce", "zookeeper", "hbase", "hive", "sqoop", "spark", "zeppelin"],
                        "hosts_count": 1,
                        "subcid": "subcid1",
                        "assign_public_ip": false,
                        "subnet_id": "network1-myt"
                    },
                    "subcid2": {
                        "cid": "cid1",
                        "name": "data",
                        "resources": {
                            "resource_preset_id": "s1.compute.3",
                            "disk_type_id": "network-ssd",
                            "disk_size": 16106127360,
                            "memory": 17179869184,
                            "cores": 4,
                            "core_fraction": 100
                         },
                        "hosts": [
                            "myt-dataproc-d-1.df.cloud.yandex.net",
                            "myt-dataproc-d-2.df.cloud.yandex.net",
                            "myt-dataproc-d-3.df.cloud.yandex.net",
                            "myt-dataproc-d-4.df.cloud.yandex.net",
                            "myt-dataproc-d-5.df.cloud.yandex.net",
                            "myt-dataproc-d-6.df.cloud.yandex.net"
                        ],
                        "role": "hadoop_cluster.datanode",
                        "services": ["hdfs", "yarn", "mapreduce", "tez", "hbase", "spark", "flume"],
                        "hosts_count": 6,
                        "subcid": "subcid2",
                        "assign_public_ip": false,
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
                    "subcluster_id": "subcid1",
                    "subcluster_role": "masternode",
                    "foo": "bar"
                },
                "subcid2": {
                    "cloud_id": "cloud1",
                    "cluster_id": "cid1",
                    "folder_id": "folder1",
                    "subcluster_id": "subcid2",
                    "subcluster_role": "datanode",
                    "foo": "bar"
                }
            }
        }
    }
    """

  Scenario: Resource preset change on subcluster works
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid1" with data
    """
    {
        "resources": {
            "resourcePresetId": "s1.compute.3"
        }
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
            "subclusterId": "subcid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.dataproc.UpdateSubcluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "subcluster_id": "subcid1"
        }
    }
    """
    When we run query
    """
    UPDATE dbaas.subclusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "subcid1",
        "clusterId": "cid1",
        "createdAt": "2000-01-01T00:00:00+00:00",
        "name": "main",
        "hostsCount": 1,
        "resources": {
            "diskSize": 16106127360,
            "diskTypeId": "network-ssd",
            "resourcePresetId": "s1.compute.3"
        },
        "role": "MASTERNODE",
        "assignPublicIp": false,
        "subnetId": "network1-myt"
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
                    "resourcePresetId": "s1.compute.3"
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
                    "diskSize": 16106127360,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.2"
                },
                "role": "DATANODE",
                "assignPublicIp": false,
                "subnetId": "network1-myt"
            }
        ]
    }
    """


  Scenario: Increasing subcluster host count works
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid2" with data
    """
    {
        "hostsCount": 8
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
    When we run query
    """
    UPDATE dbaas.subclusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid2"
    Then we get response with status 200 and body contains
    """
    {
        "id": "subcid2",
        "clusterId": "cid1",
        "createdAt": "2000-01-01T00:00:00+00:00",
        "name": "data",
        "hostsCount": 8,
        "resources": {
            "diskSize": 16106127360,
            "diskTypeId": "network-ssd",
            "resourcePresetId": "s1.compute.2"
        },
        "role": "DATANODE",
        "assignPublicIp": false,
        "subnetId": "network1-myt"
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
                "hostsCount": 8,
                "resources": {
                    "diskSize": 16106127360,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.2"
                },
                "role": "DATANODE",
                "assignPublicIp": false,
                "subnetId": "network1-myt"
            }
        ]
    }
    """


  Scenario: Changing many fields for subcluster works
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid2" with data
    """
    {
        "hostsCount": 7,
        "name": "data2",
        "resources": {
            "resourcePresetId": "s1.compute.2"
        }
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
    When we run query
    """
    UPDATE dbaas.subclusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid2"
    Then we get response with status 200 and body contains
    """
    {
        "id": "subcid2",
        "clusterId": "cid1",
        "createdAt": "2000-01-01T00:00:00+00:00",
        "name": "data2",
        "hostsCount": 7,
        "resources": {
            "diskSize": 16106127360,
            "diskTypeId": "network-ssd",
            "resourcePresetId": "s1.compute.2"
        },
        "role": "DATANODE",
        "assignPublicIp": false,
        "subnetId": "network1-myt"
    }
    """


  Scenario: Multiple fields modify on cluster subclusters works
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "subclustersSpec": [
                {
                    "id": "subcid1",
                    "name": "master2"
                },
                {
                    "id": "subcid2",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskSize": 32212254720
                    },
                    "hostsCount": 9
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
                "name": "master2",
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
                "hostsCount": 9,
                "resources": {
                    "diskSize": 32212254720,
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


  Scenario: Service account is changed for the cluster
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "serviceAccountId": "service_account_with_permission"
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
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "serviceAccountId": "service_account_with_permission"
    }
    """

  Scenario: S3 bucket is changed for the cluster
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "bucket": "user_s3_bucket-new"
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
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "bucket": "user_s3_bucket-new"
    }
    """

  Scenario: Proxy UI flag is changed for the cluster
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
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "uiProxy": true
    }
    """

  Scenario: Unable to modify unhealthy cluster
    Given feature flags
    """
    ["MDB_DATAPROC_MANAGER"]
    """
    Given dataproc manager cluster health response
    """
    {
        "health": "DEGRADED",
        "explanation": "this is explanation"
    }
    """
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "serviceAccountId": "service_account_with_permission"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Unable to modify cluster while its health status is Degraded: this is explanation"
    }
    """
