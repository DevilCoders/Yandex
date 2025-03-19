Feature: Validate console handlers

  Background:
    Given default headers

  Scenario: Validate estimated billing with datanodes
    When we POST "/mdb/hadoop/1.0/console/clusters:estimate?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "configSpec": {
            "subclustersSpec": [
                {
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 10737418240
                    },
                    "subnetId": "network1-myt"
                },
                {
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.2",
                        "diskTypeId": "network-ssd",
                        "diskSize": 536870912000
                    },
                    "hostsCount": 2,
                    "subnetId": "network1-myt"
                }
            ]
        },
        "zoneId": "myt"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 10737418240,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 1.0,
                    "gpus": 0,
                    "memory": 4294967296,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            },
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 536870912000,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2.0,
                    "gpus": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            },
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 536870912000,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2.0,
                    "gpus": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            }
        ]
    }
    """

  Scenario: Validate estimated billing without subnetId works
    When we POST "/mdb/hadoop/1.0/console/clusters:estimate?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "configSpec": {
            "subclustersSpec": [
                {
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 10737418240
                    }
                },
                {
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.2",
                        "diskTypeId": "network-ssd",
                        "diskSize": 536870912000
                    },
                    "hostsCount": 2
                }
            ]
        },
        "zoneId": "myt"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 10737418240,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 1.0,
                    "gpus": 0,
                    "memory": 4294967296,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            },
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 536870912000,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2.0,
                    "gpus": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            },
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 536870912000,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2.0,
                    "gpus": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            }
        ]
    }
    """

  Scenario: Validate estimated billing with all types of nodes
    When we POST "/mdb/hadoop/1.0/console/clusters:estimate?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "configSpec": {
            "subclustersSpec": [
                {
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 10737418240
                    },
                    "subnetId": "network1-myt"
                },
                {
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.2",
                        "diskTypeId": "network-ssd",
                        "diskSize": 536870912000
                    },
                    "hostsCount": 2,
                    "subnetId": "network1-myt"
                },
                {
                    "role": "COMPUTENODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.2",
                        "diskTypeId": "network-ssd",
                        "diskSize": 10737418240
                    },
                    "hostsCount": 3,
                    "subnetId": "network1-myt"
                }
             ]
        },
        "zoneId": "myt"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 10737418240,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 1.0,
                    "gpus": 0,
                    "memory": 4294967296,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            },
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 536870912000,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2.0,
                    "gpus": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            },
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 536870912000,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2.0,
                    "gpus": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            },
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 10737418240,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2.0,
                    "gpus": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            },
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 10737418240,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2.0,
                    "gpus": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            },
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 10737418240,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2.0,
                    "gpus": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            }
        ]
    }
    """

  Scenario: Validate estimated billing with gpu nodes
    When we POST "/mdb/hadoop/1.0/console/clusters:estimate?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "configSpec": {
            "subclustersSpec": [
                {
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 10737418240
                    },
                    "subnetId": "network1-myt"
                },
                {
                    "role": "COMPUTENODE",
                    "resources": {
                        "resourcePresetId": "g3.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 10737418240
                    },
                    "hostsCount": 1,
                    "subnetId": "network1-myt"
                }
             ]
        },
        "zoneId": "myt"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 10737418240,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 1.0,
                    "gpus": 0,
                    "memory": 4294967296,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            },
            {
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 10737418240,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 1.0,
                    "gpus": 1,
                    "memory": 4294967296,
                    "platform_id": "standard-v1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            }
        ]
    }
    """

  Scenario: Get console config
    Given feature flags
    """
    ["MDB_DATAPROC_IMAGE_99"]
    """
    When we GET "/mdb/hadoop/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusterName": {
            "blacklist": [],
            "max": 63,
            "min": 0,
            "regexp": "[a-zA-Z0-9_-]*"
        },
        "hostCountLimits": {
            "maxHostCount": 32,
            "minHostCount": 1
        },
        "versions": ["1.3.1", "1.3", "1.4.1", "1.4", "2.0.1", "2.0", "99.0.0"],
        "availableVersions": [
            { "id": "1.3.1", "name": "1.3.1", "deprecated": true, "updatableTo": [] },
            { "id": "1.3", "name": "1.3", "deprecated": true, "updatableTo": [] },
            { "id": "1.4.1", "name": "1.4.1", "deprecated": false, "updatableTo": [] },
            { "id": "1.4", "name": "1.4", "deprecated": false, "updatableTo": [] },
            { "id": "2.0.1", "name": "2.0.1", "deprecated": false, "updatableTo": [] },
            { "id": "2.0", "name": "2.0", "deprecated": false, "updatableTo": [] },
            { "id": "99.0.0", "name": "99.0.0", "deprecated": false, "updatableTo": [] }
        ],
        "defaultResources": {
            "resourcePresetId": "s2.micro",
            "diskSize": 21474836480,
            "diskTypeId": "network-ssd"
        },
        "defaultVersion": "2.0",
        "images": [
            {
                "version": "1.3.1",
                "services": [
                    {
                        "service": "HDFS",
                        "version": "2.8.5",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "YARN",
                        "version": "2.8.5",
                        "dependencies": ["HDFS"],
                        "default": true
                    },
                    {
                        "service": "MAPREDUCE",
                        "version": "2.8.5",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "TEZ",
                        "version": "0.9.1",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "ZOOKEEPER",
                        "version": "3.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "HBASE",
                        "version": "1.3.3",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"],
                        "default": true
                    },
                    {
                        "service": "HIVE",
                        "version": "2.3.4",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "SQOOP",
                        "version": "1.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "FLUME",
                        "version": "1.8.0",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "SPARK",
                        "version": "2.2.1",
                        "dependencies": ["YARN", "HDFS"],
                        "default": true
                    },
                    {
                        "service": "ZEPPELIN",
                        "version": "0.7.3",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "OOZIE",
                        "version": "4.3.1",
                        "dependencies": ["ZOOKEEPER"],
                        "default": false
                    },
                    {
                        "service": "LIVY",
                        "version": "0.7.0",
                        "dependencies": ["SPARK"],
                        "default": false
                    }
                ],
                "availableServices": [
                    "HDFS",
                    "YARN",
                    "MAPREDUCE",
                    "TEZ",
                    "ZOOKEEPER",
                    "HBASE",
                    "HIVE",
                    "SQOOP",
                    "FLUME",
                    "SPARK",
                    "ZEPPELIN",
                    "OOZIE",
                    "LIVY"
                ],
                "serviceDependencies": [
                    {
                        "service": "YARN",
                        "dependencies": ["HDFS"]
                    },
                    {
                        "service": "MAPREDUCE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "TEZ",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "HBASE",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"]
                    },
                    {
                        "service": "HIVE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "SPARK",
                        "dependencies": ["YARN", "HDFS"]
                    },
                    {
                        "service": "OOZIE",
                        "dependencies": ["ZOOKEEPER"]
                    },
                    {
                        "service": "LIVY",
                        "dependencies": ["SPARK"]
                    }
                ],
                "minSize": 16106127360
            },
            {
                "version": "1.3",
                "services": [
                    {
                        "service": "HDFS",
                        "version": "2.8.5",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "YARN",
                        "version": "2.8.5",
                        "dependencies": ["HDFS"],
                        "default": true
                    },
                    {
                        "service": "MAPREDUCE",
                        "version": "2.8.5",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "TEZ",
                        "version": "0.9.1",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "ZOOKEEPER",
                        "version": "3.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "HBASE",
                        "version": "1.3.3",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"],
                        "default": true
                    },
                    {
                        "service": "HIVE",
                        "version": "2.3.4",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "SQOOP",
                        "version": "1.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "FLUME",
                        "version": "1.8.0",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "SPARK",
                        "version": "2.2.1",
                        "dependencies": ["YARN", "HDFS"],
                        "default": true
                    },
                    {
                        "service": "ZEPPELIN",
                        "version": "0.7.3",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "OOZIE",
                        "version": "4.3.1",
                        "dependencies": ["ZOOKEEPER"],
                        "default": false
                    },
                    {
                        "service": "LIVY",
                        "version": "0.7.0",
                        "dependencies": ["SPARK"],
                        "default": false
                    }
                ],
                "availableServices": [
                    "HDFS",
                    "YARN",
                    "MAPREDUCE",
                    "TEZ",
                    "ZOOKEEPER",
                    "HBASE",
                    "HIVE",
                    "SQOOP",
                    "FLUME",
                    "SPARK",
                    "ZEPPELIN",
                    "OOZIE",
                    "LIVY"
                ],
                "serviceDependencies": [
                    {
                        "service": "YARN",
                        "dependencies": ["HDFS"]
                    },
                    {
                        "service": "MAPREDUCE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "TEZ",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "HBASE",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"]
                    },
                    {
                        "service": "HIVE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "SPARK",
                        "dependencies": ["YARN", "HDFS"]
                    },
                    {
                        "service": "OOZIE",
                        "dependencies": ["ZOOKEEPER"]
                    },
                    {
                        "service": "LIVY",
                        "dependencies": ["SPARK"]
                    }
                ],
                "minSize": 16106127360
            },
            {
                "version": "1.4.1",
                "services": [
                    {
                        "service": "HDFS",
                        "version": "2.8.5",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "YARN",
                        "version": "2.8.5",
                        "dependencies": ["HDFS"],
                        "default": true
                    },
                    {
                        "service": "MAPREDUCE",
                        "version": "2.8.5",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "TEZ",
                        "version": "0.9.1",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "ZOOKEEPER",
                        "version": "3.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "HBASE",
                        "version": "1.3.3",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"],
                        "default": true
                    },
                    {
                        "service": "HIVE",
                        "version": "2.3.4",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "SQOOP",
                        "version": "1.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "FLUME",
                        "version": "1.8.0",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "SPARK",
                        "version": "2.2.1",
                        "dependencies": ["YARN", "HDFS"],
                        "default": true
                    },
                    {
                        "service": "ZEPPELIN",
                        "version": "0.7.3",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "OOZIE",
                        "version": "4.3.1",
                        "dependencies": ["ZOOKEEPER"],
                        "default": false
                    },
                    {
                        "service": "LIVY",
                        "version": "0.7.0",
                        "dependencies": ["SPARK"],
                        "default": false
                    }
                ],
                "availableServices": [
                    "HDFS",
                    "YARN",
                    "MAPREDUCE",
                    "TEZ",
                    "ZOOKEEPER",
                    "HBASE",
                    "HIVE",
                    "SQOOP",
                    "FLUME",
                    "SPARK",
                    "ZEPPELIN",
                    "OOZIE",
                    "LIVY"
                ],
                "serviceDependencies": [
                    {
                        "service": "YARN",
                        "dependencies": ["HDFS"]
                    },
                    {
                        "service": "MAPREDUCE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "TEZ",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "HBASE",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"]
                    },
                    {
                        "service": "HIVE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "SPARK",
                        "dependencies": ["YARN", "HDFS"]
                    },
                    {
                        "service": "OOZIE",
                        "dependencies": ["ZOOKEEPER"]
                    },
                    {
                        "service": "LIVY",
                        "dependencies": ["SPARK"]
                    }
                ],
                "minSize": 16106127360
            },
            {
                "version": "1.4",
                "services": [
                    {
                        "service": "HDFS",
                        "version": "2.8.5",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "YARN",
                        "version": "2.8.5",
                        "dependencies": ["HDFS"],
                        "default": true
                    },
                    {
                        "service": "MAPREDUCE",
                        "version": "2.8.5",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "TEZ",
                        "version": "0.9.1",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "ZOOKEEPER",
                        "version": "3.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "HBASE",
                        "version": "1.3.3",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"],
                        "default": true
                    },
                    {
                        "service": "HIVE",
                        "version": "2.3.4",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "SQOOP",
                        "version": "1.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "FLUME",
                        "version": "1.8.0",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "SPARK",
                        "version": "2.2.1",
                        "dependencies": ["YARN", "HDFS"],
                        "default": true
                    },
                    {
                        "service": "ZEPPELIN",
                        "version": "0.7.3",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "OOZIE",
                        "version": "4.3.1",
                        "dependencies": ["ZOOKEEPER"],
                        "default": false
                    },
                    {
                        "service": "LIVY",
                        "version": "0.7.0",
                        "dependencies": ["SPARK"],
                        "default": false
                    }
                ],
                "availableServices": [
                    "HDFS",
                    "YARN",
                    "MAPREDUCE",
                    "TEZ",
                    "ZOOKEEPER",
                    "HBASE",
                    "HIVE",
                    "SQOOP",
                    "FLUME",
                    "SPARK",
                    "ZEPPELIN",
                    "OOZIE",
                    "LIVY"
                ],
                "serviceDependencies": [
                    {
                        "service": "YARN",
                        "dependencies": ["HDFS"]
                    },
                    {
                        "service": "MAPREDUCE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "TEZ",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "HBASE",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"]
                    },
                    {
                        "service": "HIVE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "SPARK",
                        "dependencies": ["YARN", "HDFS"]
                    },
                    {
                        "service": "OOZIE",
                        "dependencies": ["ZOOKEEPER"]
                    },
                    {
                        "service": "LIVY",
                        "dependencies": ["SPARK"]
                    }
                ],
                "minSize": 16106127360
            },
            {
                "version": "2.0.1",
                "services": [
                    {
                        "service": "HDFS",
                        "version": "2.8.5",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "YARN",
                        "version": "2.8.5",
                        "dependencies": ["HDFS"],
                        "default": true
                    },
                    {
                        "service": "MAPREDUCE",
                        "version": "2.8.5",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "TEZ",
                        "version": "0.9.1",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "ZOOKEEPER",
                        "version": "3.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "HBASE",
                        "version": "1.3.3",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"],
                        "default": true
                    },
                    {
                        "service": "HIVE",
                        "version": "2.3.4",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "SQOOP",
                        "version": "1.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "FLUME",
                        "version": "1.8.0",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "SPARK",
                        "version": "2.2.1",
                        "dependencies": ["YARN", "HDFS"],
                        "default": true
                    },
                    {
                        "service": "ZEPPELIN",
                        "version": "0.7.3",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "OOZIE",
                        "version": "4.3.1",
                        "dependencies": ["ZOOKEEPER"],
                        "default": false
                    },
                    {
                        "service": "LIVY",
                        "version": "0.7.0",
                        "dependencies": ["SPARK"],
                        "default": false
                    }
                ],
                "availableServices": [
                    "HDFS",
                    "YARN",
                    "MAPREDUCE",
                    "TEZ",
                    "ZOOKEEPER",
                    "HBASE",
                    "HIVE",
                    "SQOOP",
                    "FLUME",
                    "SPARK",
                    "ZEPPELIN",
                    "OOZIE",
                    "LIVY"
                ],
                "serviceDependencies": [
                    {
                        "service": "YARN",
                        "dependencies": ["HDFS"]
                    },
                    {
                        "service": "MAPREDUCE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "TEZ",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "HBASE",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"]
                    },
                    {
                        "service": "HIVE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "SPARK",
                        "dependencies": ["YARN", "HDFS"]
                    },
                    {
                        "service": "OOZIE",
                        "dependencies": ["ZOOKEEPER"]
                    },
                    {
                        "service": "LIVY",
                        "dependencies": ["SPARK"]
                    }
                ],
                "minSize": 16106127360
            },
            {
                "version": "2.0",
                "services": [
                    {
                        "service": "HDFS",
                        "version": "2.8.5",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "YARN",
                        "version": "2.8.5",
                        "dependencies": ["HDFS"],
                        "default": true
                    },
                    {
                        "service": "MAPREDUCE",
                        "version": "2.8.5",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "TEZ",
                        "version": "0.9.1",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "ZOOKEEPER",
                        "version": "3.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "HBASE",
                        "version": "1.3.3",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"],
                        "default": true
                    },
                    {
                        "service": "HIVE",
                        "version": "2.3.4",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "SQOOP",
                        "version": "1.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "FLUME",
                        "version": "1.8.0",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "SPARK",
                        "version": "2.2.1",
                        "dependencies": ["YARN", "HDFS"],
                        "default": true
                    },
                    {
                        "service": "ZEPPELIN",
                        "version": "0.7.3",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "OOZIE",
                        "version": "4.3.1",
                        "dependencies": ["ZOOKEEPER"],
                        "default": false
                    },
                    {
                        "service": "LIVY",
                        "version": "0.7.0",
                        "dependencies": ["SPARK"],
                        "default": false
                    }
                ],
                "availableServices": [
                    "HDFS",
                    "YARN",
                    "MAPREDUCE",
                    "TEZ",
                    "ZOOKEEPER",
                    "HBASE",
                    "HIVE",
                    "SQOOP",
                    "FLUME",
                    "SPARK",
                    "ZEPPELIN",
                    "OOZIE",
                    "LIVY"
                ],
                "serviceDependencies": [
                    {
                        "service": "YARN",
                        "dependencies": ["HDFS"]
                    },
                    {
                        "service": "MAPREDUCE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "TEZ",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "HBASE",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"]
                    },
                    {
                        "service": "HIVE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "SPARK",
                        "dependencies": ["YARN", "HDFS"]
                    },
                    {
                        "service": "OOZIE",
                        "dependencies": ["ZOOKEEPER"]
                    },
                    {
                        "service": "LIVY",
                        "dependencies": ["SPARK"]
                    }
                ],
                "minSize": 16106127360
            },
            {
                "version": "99.0.0",
                "services": [
                    {
                        "service": "HDFS",
                        "version": "2.8.5",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "YARN",
                        "version": "2.8.5",
                        "dependencies": ["HDFS"],
                        "default": true
                    },
                    {
                        "service": "MAPREDUCE",
                        "version": "2.8.5",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "TEZ",
                        "version": "0.9.1",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "ZOOKEEPER",
                        "version": "3.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "HBASE",
                        "version": "1.3.3",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"],
                        "default": true
                    },
                    {
                        "service": "HIVE",
                        "version": "2.3.4",
                        "dependencies": ["YARN"],
                        "default": true
                    },
                    {
                        "service": "SQOOP",
                        "version": "1.4.6",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "FLUME",
                        "version": "1.8.0",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "SPARK",
                        "version": "2.2.1",
                        "dependencies": ["YARN", "HDFS"],
                        "default": true
                    },
                    {
                        "service": "ZEPPELIN",
                        "version": "0.7.3",
                        "dependencies": [],
                        "default": true
                    },
                    {
                        "service": "OOZIE",
                        "version": "4.3.1",
                        "dependencies": ["ZOOKEEPER"],
                        "default": false
                    },
                    {
                        "service": "LIVY",
                        "version": "0.7.0",
                        "dependencies": ["SPARK"],
                        "default": false
                    }
                ],
                "availableServices": [
                    "HDFS",
                    "YARN",
                    "MAPREDUCE",
                    "TEZ",
                    "ZOOKEEPER",
                    "HBASE",
                    "HIVE",
                    "SQOOP",
                    "FLUME",
                    "SPARK",
                    "ZEPPELIN",
                    "OOZIE",
                    "LIVY"
                ],
                "serviceDependencies": [
                    {
                        "service": "YARN",
                        "dependencies": ["HDFS"]
                    },
                    {
                        "service": "MAPREDUCE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "TEZ",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "HBASE",
                        "dependencies": ["ZOOKEEPER", "HDFS", "YARN"]
                    },
                    {
                        "service": "HIVE",
                        "dependencies": ["YARN"]
                    },
                    {
                        "service": "SPARK",
                        "dependencies": ["YARN", "HDFS"]
                    },
                    {
                        "service": "OOZIE",
                        "dependencies": ["ZOOKEEPER"]
                    },
                    {
                        "service": "LIVY",
                        "dependencies": ["SPARK"]
                    }
                ],
                "minSize": 16106127360
            }
        ]
    }
    """

  Scenario: Get console config and check resourcePresets against schema
    When we GET "/mdb/hadoop/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body matches "hadoop/clusters_config_schema.json" schema

  Scenario: Get console config zones in resourcePresets
    When we GET "/mdb/hadoop/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body at path "$.hostTypes[*].resourcePresets[*].zones[*].zoneId" contains only
    """
    ["myt", "vla", "sas", "iva", "man"]
    """

  Scenario: Get Hadoop sorted resource presets
    When we GET "/mdb/hadoop/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body at path "$.hostTypes[0].resourcePresets[*]" contains fields with order: type;-generation;cpuLimit;cpuFraction;memoryLimit;presetId

  Scenario: Console config contains gpu flavors
    When we GET "/mdb/hadoop/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body at path "$.hostTypes[*].resourcePresets[*].presetId" includes
    """
    ["g3.compute.1"]
    """
