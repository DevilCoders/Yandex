Feature: Create dataproc cluster with custom image

  Background:
    Given default headers
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "test",
        "configSpec": {
            "versionId": "1.4",
            "imageId": "customImageId",
            "hadoop": {
                "services": ["HDFS", "YARN", "MAPREDUCE", "TEZ", "HIVE", "SPARK"],
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
        "bucket": "user_s3_bucket"
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

  Scenario: Unmanaged config handler works properly
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
            "ssh_public_keys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"],
            "image": "customImageId",
            "subcluster_main_id": "subcid1",
            "service_account_id": "service_account_1",
            "services": ["hdfs", "hive", "mapreduce", "spark", "tez", "yarn"],
            "properties": {},
            "s3_bucket": "user_s3_bucket",
            "ui_proxy": false,
            "topology": {
                "revision": 3,
                "network_id": "network1",
                "zone_id": "myt",
                "subclusters": {
                    "subcid1": {
                        "cid": "cid1",
                        "name": "main",
                        "resources": {
                            "resource_preset_id": "s1.compute.1",
                            "disk_type_id": "network-ssd",
                            "disk_size": 16106127360,
                            "cores": 1.0,
                            "core_fraction": 100,
                            "memory": 4294967296
                        },
                        "hosts": [
                            "myt-dataproc-m-1.df.cloud.yandex.net"
                        ],
                        "role": "hadoop_cluster.masternode",
                        "services": ["hdfs", "yarn", "mapreduce", "hive", "spark"],
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
                            "disk_size": 16106127360,
                            "cores": 1.0,
                            "core_fraction": 100,
                            "memory": 4294967296
                        },
                        "hosts": [
                            "myt-dataproc-d-1.df.cloud.yandex.net",
                            "myt-dataproc-d-2.df.cloud.yandex.net",
                            "myt-dataproc-d-3.df.cloud.yandex.net",
                            "myt-dataproc-d-4.df.cloud.yandex.net",
                            "myt-dataproc-d-5.df.cloud.yandex.net"
                        ],
                        "role": "hadoop_cluster.datanode",
                        "services": ["hdfs", "yarn", "mapreduce", "tez", "spark"],
                        "hosts_count": 5,
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
                    "subcluster_role": "masternode"
                },
                "subcid2": {
                    "cloud_id": "cloud1",
                    "cluster_id": "cid1",
                    "folder_id": "folder1",
                    "subcluster_id": "subcid2",
                    "subcluster_role": "datanode"
                }
            }
        }
    }
    """

  @events
  Scenario: Unmanaged config handler works properly after added subcluster
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
    When we GET "/api/v1.0/config_unmanaged/myt-dataproc-c-1.df.cloud.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "ssh_authorized_keys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"],
        "data": {
            "agent": {
                "cid": "cid1",
                "manager_url": "test-dataproc-manager-public-url"
            },
            "ssh_public_keys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"],
            "image": "customImageId",
            "subcluster_main_id": "subcid1",
            "service_account_id": "service_account_1",
            "services": ["hdfs", "hive", "mapreduce", "spark", "tez", "yarn"],
            "properties": {},
            "s3_bucket": "user_s3_bucket",
            "ui_proxy": false,
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
                            "cores": 1.0,
                            "core_fraction": 100,
                            "memory": 4294967296
                        },
                        "hosts": [
                            "myt-dataproc-m-1.df.cloud.yandex.net"
                        ],
                        "role": "hadoop_cluster.masternode",
                        "services": ["hdfs", "yarn", "mapreduce", "hive", "spark"],
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
                            "disk_size": 16106127360,
                            "cores": 1.0,
                            "core_fraction": 100,
                            "memory": 4294967296
                        },
                        "hosts": [
                            "myt-dataproc-d-1.df.cloud.yandex.net",
                            "myt-dataproc-d-2.df.cloud.yandex.net",
                            "myt-dataproc-d-3.df.cloud.yandex.net",
                            "myt-dataproc-d-4.df.cloud.yandex.net",
                            "myt-dataproc-d-5.df.cloud.yandex.net"
                        ],
                        "role": "hadoop_cluster.datanode",
                        "services": ["hdfs", "yarn", "mapreduce", "tez", "spark"],
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
                            "cores": 1.0,
                            "core_fraction": 100,
                            "memory": 4294967296
                        },
                        "hosts": [
                            "myt-dataproc-c-1.df.cloud.yandex.net",
                            "myt-dataproc-c-2.df.cloud.yandex.net",
                            "myt-dataproc-c-3.df.cloud.yandex.net"
                        ],
                        "role": "hadoop_cluster.computenode",
                        "services": ["yarn", "mapreduce", "tez"],
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
                    "subcluster_id": "subcid1",
                    "subcluster_role": "masternode"
                },
                "subcid2": {
                    "cloud_id": "cloud1",
                    "cluster_id": "cid1",
                    "folder_id": "folder1",
                    "subcluster_id": "subcid2",
                    "subcluster_role": "datanode"
                },
                "subcid3": {
                    "cloud_id": "cloud1",
                    "cluster_id": "cid1",
                    "folder_id": "folder1",
                    "subcluster_id": "subcid3",
                    "subcluster_role": "computenode"
                }
             }
        }
    }
    """
