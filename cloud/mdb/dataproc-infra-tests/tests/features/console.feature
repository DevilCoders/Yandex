Feature: Validate console handlers

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with standard Hadoop cluster


  Scenario: Validate estimated billing with datanodes
    When we POST "/mdb/hadoop/1.0/console/clusters:estimate?folderId={{ template_params['folder_ext_id'] }}" with data
    """
    {
      "folderId": "{{ template_params['folder_ext_id'] }}",
      "configSpec": {
        "subclustersSpec": [
          {
            "role": "MASTERNODE",
            "resources": {
              "resourcePresetId": "s2.micro",
              "diskTypeId": "network-nvme",
              "diskSize": 21474836480
            },
            "subnetId": "{{ template_params['subnet_id'] }}"
          },
          {
            "role": "DATANODE",
            "resources": {
              "resourcePresetId": "s2.micro",
              "diskTypeId": "network-ssd",
              "diskSize": 16106127360
            },
            "hostsCount": 2,
            "subnetId": "{{ template_params['subnet_id'] }}"
          }
        ]
      },
      "zoneId": "myt"
    }
    """
    Then response should have status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 21474836480,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 16106127360,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 16106127360,
                    "type": "network-ssd"
                }
            }
        ]
    }
    """

  Scenario: Validate estimated billing without subnetId works
    When we POST "/mdb/hadoop/1.0/console/clusters:estimate?folderId={{ template_params['folder_ext_id'] }}" with data
    """
    {
        "folderId": "{{ folder['cloud_ext_id'] }}",
        "configSpec": {
            "subclustersSpec": [
                {
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s2.micro",
                        "diskTypeId": "network-nvme",
                        "diskSize": 21474836480
                    }
                },
                {
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s2.small",
                        "diskTypeId": "network-nvme",
                        "diskSize": 536870912000
                    },
                    "hostsCount": 2
                }
            ]
        },
        "zoneId": "myt"
    }
    """
    Then response should have status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 21474836480,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 4,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 17179869184,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 536870912000,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 4,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 17179869184,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 536870912000,
                    "type": "network-ssd"
                }
            }
        ]
    }
    """

  Scenario: Validate estimated billing with all types of nodes
    When we POST "/mdb/hadoop/1.0/console/clusters:estimate?folderId={{ template_params['folder_ext_id'] }}" with data
    """
    {
        "folderId": "{{ template_params['folder_ext_id'] }}",
        "configSpec": {
            "subclustersSpec": [
                {
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s2.micro",
                        "diskTypeId": "network-nvme",
                        "diskSize": 21474836480
                    },
                    "subnetId": "{{ template_params['subnet_id'] }}"
                },
                {
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s2.small",
                        "diskTypeId": "network-nvme",
                        "diskSize": 536870912000
                    },
                    "hostsCount": 2,
                    "subnetId": "{{ template_params['subnet_id'] }}"
                },
                {
                    "role": "COMPUTENODE",
                    "resources": {
                        "resourcePresetId": "s2.small",
                        "diskTypeId": "network-nvme",
                        "diskSize": 107374182400
                    },
                    "hostsCount": 3,
                    "subnetId": "{{ template_params['subnet_id'] }}"
                }
             ]
        },
        "zoneId": "myt"
    }
    """
    Then response should have status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 21474836480,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 4,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 17179869184,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 536870912000,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 4,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 17179869184,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 536870912000,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 4,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 17179869184,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 107374182400,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 4,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 17179869184,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 107374182400,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 4,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 17179869184,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 107374182400,
                    "type": "network-ssd"
                }
            }
        ]
    }
    """

  Scenario: Validate estimated billing with gpu nodes
    When we POST "/mdb/hadoop/1.0/console/clusters:estimate?folderId={{ template_params['folder_ext_id'] }}" with data
    """
    {
        "folderId": "{{ template_params['folder_ext_id'] }}",
        "configSpec": {
            "subclustersSpec": [
                {
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s2.micro",
                        "diskTypeId": "network-nvme",
                        "diskSize": 21474836480
                    },
                    "subnetId": "{{ template_params['subnet_id'] }}"
                },
                {
                    "role": "COMPUTENODE",
                    "resources": {
                        "resourcePresetId": "g1.small",
                        "diskTypeId": "network-nvme",
                        "diskSize": 21474836480
                    },
                    "hostsCount": 1,
                    "subnetId": "{{ template_params['subnet_id'] }}"
                }
             ]
        },
        "zoneId": "myt"
    }
    """
    Then response should have status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 21474836480,
                    "type": "network-ssd"
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 8,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 1,
                    "nvme_disks": 0,
                    "memory": 103079215104,
                    "platform_id": "gpu-standard-v1",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 21474836480,
                    "type": "network-ssd"
                }
            }
        ]
    }
    """

  Scenario: Validate estimated billing with different disk types
    When we POST "/mdb/hadoop/1.0/console/clusters:estimate?folderId={{ template_params['folder_ext_id'] }}" with data
    """
    {
        "folderId": "{{ template_params['folder_ext_id'] }}",
        "configSpec": {
            "subclustersSpec": [
                {
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s2.micro",
                        "diskTypeId": "network-hdd",
                        "diskSize": 21474836480
                    },
                    "subnetId": "{{ template_params['subnet_id'] }}"
                },
                {
                    "role": "COMPUTENODE",
                    "resources": {
                        "resourcePresetId": "s2.small",
                        "diskTypeId": "network-ssd-nonreplicated",
                        "diskSize": 99857989632
                    },
                    "hostsCount": 1,
                    "subnetId": "{{ template_params['subnet_id'] }}"
                }
             ]
        },
        "zoneId": "myt"
    }
    """
    Then response should have status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 8589934592,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 21474836480,
                    "type": "network-hdd"
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 4,
                    "core_fraction": 100,
                    "preemptible": false,
                    "gpus": 0,
                    "nvme_disks": 0,
                    "memory": 17179869184,
                    "platform_id": "standard-v2",
                    "product_ids": ["{{ template_params['product_id'] }}"],
                    "public_fips": 0,
                    "sockets": 1,
                    "boot_disk_product_ids": ["{{ template_params['product_id'] }}"]
                }
            },
            {
                "folder_id": "{{ template_params['folder_ext_id'] }}",
                "schema": "nbs.volume.allocated.v3",
                "tags": {
                    "size": 99857989632,
                    "type": "network-ssd-nonreplicated"
                }
            }
        ]
    }
    """

  Scenario: Get console config
    When we get console cluster config
    Then response should have status 200 and body contains
    """
    {
        "decommissionTimeout": {
            "min": 0,
            "max": 86400
        },
        "defaultResources": {
            "resourcePresetId": "s2.micro",
            "diskSize": 21474836480,
            "diskTypeId": "network-ssd"
        },
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
        "defaultVersion": "2.0"
    }
    """
    Then response should have status 200 and key versions list contains
    """
    ["1.4", "2.0", "2.1"]
    """
    Then response should have status 200 and key availableVersions list contains
    """
    [
        { "id": "1.4", "name": "1.4", "deprecated": false, "updatableTo": [] },
        { "id": "2.0", "name": "2.0", "deprecated": false, "updatableTo": [] },
        { "id": "2.1", "name": "2.1", "deprecated": false, "updatableTo": [] }
    ]
    """
    Then response should have status 200 and key images list contains
    """
    [
        {
            "serviceDependencies": [
                {"service": "YARN", "dependencies": ["HDFS"]},
                {"service": "MAPREDUCE", "dependencies": ["YARN"]},
                {"service": "TEZ", "dependencies": ["YARN"]},
                {"service": "HBASE", "dependencies": ["ZOOKEEPER", "HDFS", "YARN"]},
                {"service": "HIVE", "dependencies": ["YARN", "MAPREDUCE"]},
                {"service": "SPARK", "dependencies": ["YARN", "HDFS"]},
                {"service": "LIVY", "dependencies": ["SPARK"]}],
            "availableServices":
                    ["HDFS", "YARN", "MAPREDUCE", "TEZ", "ZOOKEEPER", "HBASE", "HIVE", "SQOOP", "FLUME", "SPARK", "ZEPPELIN", "OOZIE", "LIVY"],
            "minSize": 21474836480,
            "version": "1.4",
            "services": [
                    {"default": true, "version": "2.10.0", "service": "HDFS", "dependencies": []},
                    {"default": true, "version": "2.10.0", "service": "YARN", "dependencies": ["HDFS"]},
                    {"default": true, "version": "2.10.0", "service": "MAPREDUCE", "dependencies": ["YARN"]},
                    {"default": true, "version": "0.9.2", "service": "TEZ", "dependencies": ["YARN"]},
                    {"default": false, "version": "3.4.14", "service": "ZOOKEEPER", "dependencies": []},
                    {"default": false, "version": "1.3.5", "service": "HBASE", "dependencies": ["ZOOKEEPER", "HDFS", "YARN"]},
                    {"default": false, "version": "2.3.6", "service": "HIVE", "dependencies": ["YARN", "MAPREDUCE"]},
                    {"default": false, "version": "1.4.7", "service": "SQOOP", "dependencies": []},
                    {"default": false, "version": "1.9.0", "service": "FLUME", "dependencies": []},
                    {"default": true, "version": "2.4.6", "service": "SPARK", "dependencies": ["YARN", "HDFS"]},
                    {"default": false, "version": "0.8.2", "service": "ZEPPELIN", "dependencies": []},
                    {"default": false, "version": "5.2.0", "service": "OOZIE", "dependencies": []},
                    {"default": false, "version": "0.7.0", "service": "LIVY", "dependencies": ["SPARK"]}
            ]
        },
        {
            "serviceDependencies": [
                {"service": "MAPREDUCE", "dependencies": ["YARN", "HDFS"]},
                {"service": "TEZ", "dependencies": ["YARN"]},
                {"service": "HBASE", "dependencies": ["ZOOKEEPER", "HDFS", "YARN"]},
                {"service": "HIVE", "dependencies": ["YARN", "MAPREDUCE"]},
                {"service": "SPARK", "dependencies": ["YARN"]},
                {"service": "LIVY", "dependencies": ["SPARK"]},
                {"service": "OOZIE", "dependencies": ["HDFS"]}
            ],
            "availableServices":
                    ["HDFS", "YARN", "MAPREDUCE", "TEZ", "ZOOKEEPER", "HBASE", "HIVE", "SPARK", "ZEPPELIN", "LIVY", "OOZIE"],
            "minSize": 21474836480,
            "version": "2.0",
            "services": [
                    {"default": true, "version": "3.2.2", "service": "HDFS", "dependencies": []},
                    {"default": true, "version": "3.2.2", "service": "YARN", "dependencies": []},
                    {"default": true, "version": "3.2.2", "service": "MAPREDUCE", "dependencies": ["YARN", "HDFS"]},
                    {"default": true, "version": "0.10.0", "service": "TEZ", "dependencies": ["YARN"]},
                    {"default": false, "version": "3.4.14", "service": "ZOOKEEPER", "dependencies": []},
                    {"default": false, "version": "2.2.7", "service": "HBASE", "dependencies": ["ZOOKEEPER", "HDFS", "YARN"]},
                    {"default": false, "version": "3.1.2", "service": "HIVE", "dependencies": ["YARN", "MAPREDUCE"]},
                    {"default": true, "version": "3.0.2", "service": "SPARK", "dependencies": ["YARN"]},
                    {"default": false, "version": "0.9.0", "service": "ZEPPELIN", "dependencies": []},
                    {"default": false, "version": "0.8.0", "service": "LIVY", "dependencies": ["SPARK"]},
                    {"default": false, "version": "5.2.1", "service": "OOZIE", "dependencies": ["HDFS"]}
            ]
        }
    ]
    """
