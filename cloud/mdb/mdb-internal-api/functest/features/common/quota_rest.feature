@quota
Feature: Quota v2 REST API

    Background: Create redis in cloud and use some quota
        Given feature flags
        """
        ["MDB_REDIS_62"]
        """
        And default headers
        When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "testRedis",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_2": {
                "password": "p@ssw#$rd!?"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
        Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
        When "worker_task_id1" acquired and finished by worker

    Scenario: Quota usage works
        Given default headers
        When we GET "/mdb/v2/quota/cloud1"
        Then we get response with status 200 and body contains
    """
    {
        "cloud_id": "cloud1",
        "metrics": [
          {
            "name": "mdb.hdd.size",
            "limit": 644245094400,
            "usage": 0
          },
          {
            "name": "mdb.ssd.size",
            "limit": 644245094400,
            "usage": 17179869184.0
          },
          {
            "name": "mdb.memory.size",
            "limit": 103079215104,
            "usage": 4294967296.0
          },
          {
            "name": "mdb.gpu.count",
            "limit": 0,
            "usage": 0
          },
          {
            "name": "mdb.cpu.count",
            "limit": 24.0,
            "usage": 1.0
          },
          {
            "name": "mdb.clusters.count",
            "limit": 2.0,
            "usage": 1.0
          }
        ]
    }
    """

    Scenario: Not existing cloud has default quota
        Given default headers
        When we GET "/mdb/v2/quota/cloud2"
        Then we get response with status 200 and body contains
    """
    {
        "cloud_id": "cloud2",
        "metrics": [
          {
            "name": "mdb.hdd.size",
            "limit": 644245094400,
            "usage": 0.0
          },
          {
            "name": "mdb.ssd.size",
            "limit": 644245094400,
            "usage": 0.0
          },
          {
            "name": "mdb.memory.size",
            "limit": 103079215104,
            "usage": 0.0
          },
          {
            "name": "mdb.gpu.count",
            "limit": 0,
            "usage": 0.0
          },
          {
            "name": "mdb.cpu.count",
            "limit": 24,
            "usage": 0.0
          },
          {
            "name": "mdb.clusters.count",
            "limit": 2,
            "usage": 0.0
          }
        ]
    }
    """

    Scenario: Default quota works
        Given headers
    """
    {
        "Accept": "application/json",
        "Content-Type": "application/json"
    }
    """
        When we GET "/mdb/v2/quota"
        Then we get response with status 200 and body contains
    """
    {
      "metrics": [
          {
              "name": "mdb.hdd.size",
              "limit": 644245094400
          },
          {
              "name": "mdb.ssd.size",
              "limit": 644245094400
          },
          {
              "name": "mdb.memory.size",
              "limit": 103079215104
          },
          {
              "name": "mdb.gpu.count",
              "limit": 0
          },
          {
              "name": "mdb.cpu.count",
              "limit": 24
          },
          {
              "name": "mdb.clusters.count",
              "limit": 2
          }
      ]
    }
    """

    Scenario: Update quota works
        Given default headers
        When we GET "/mdb/v1/quota/cloud1"
        Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersQuota": 2,
        "cpuQuota": 24.0,
        "memoryQuota": 103079215104,
        "ssdSpaceQuota": 644245094400,
        "hddSpaceQuota": 644245094400
    }
    """
        When we POST "/mdb/v2/quota" with data
    """
    {
      "cloud_id": "cloud1",
      "metrics":[
        {
          "name": "mdb.hdd.size",
          "limit": 1024
        },
        {
          "name": "mdb.ssd.size",
          "limit": 17179869184
        },
        {
          "name": "mdb.memory.size",
          "limit": 4294967296
        },
        {
          "name": "mdb.gpu.count",
          "limit": 1
        },
        {
          "name": "mdb.cpu.count",
          "limit": 2
        },
        {
          "name": "mdb.clusters.count",
          "limit": 5
        }
      ]
    }
    """
        Then we get response with status 200
        When we GET "/mdb/v1/quota/cloud1"
        Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersQuota": 5,
        "cpuQuota": 2.0,
        "memoryQuota": 4294967296,
        "ssdSpaceQuota": 17179869184,
        "hddSpaceQuota": 1024
    }
    """

    Scenario: Update single quota doesn't update other
        Given default headers
        When we GET "/mdb/v1/quota/cloud1"
        Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersQuota": 2,
        "cpuQuota": 24.0,
        "memoryQuota": 103079215104,
        "ssdSpaceQuota": 644245094400,
        "hddSpaceQuota": 644245094400
    }
    """
        When we POST "/mdb/v2/quota" with data
    """
    {
      "cloud_id": "cloud1",
      "metrics":[
        {
          "name": "mdb.cpu.count",
          "limit": 2
        }
      ]
    }
    """
        Then we get response with status 200
        When we GET "/mdb/v1/quota/cloud1"
        Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersQuota": 2,
        "cpuQuota": 2.0,
        "memoryQuota": 103079215104,
        "ssdSpaceQuota": 644245094400,
        "hddSpaceQuota": 644245094400
    }
    """

    Scenario: Set quota to 0
        Given default headers
        When we GET "/mdb/v1/quota/cloud1"
        Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "hddSpaceQuota": 644245094400
    }
    """
        When we POST "/mdb/v2/quota" with data
    """
    {
      "cloud_id": "cloud1",
      "metrics":[
        {
          "name": "mdb.hdd.size",
          "limit": 0
        }
      ]
    }
    """
        Then we get response with status 200
        When we GET "/mdb/v1/quota/cloud1"
        Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "hddSpaceQuota": 0
    }
    """

    Scenario: Setting quota below usage validation works
        Given default headers
        When we POST "/mdb/v2/quota" with data
    """
    {
      "cloud_id": "cloud1",
      "metrics":[
        {
          "name": "mdb.ssd.size",
          "limit": 1024
        }
      ]
    }
    """
        Then we get response with status 422

    Scenario: Not enough quota
        Given default headers
        When we POST "/mdb/v2/quota" with data
    """
    {
      "cloud_id": "cloud1",
      "metrics":[
        {
          "name": "mdb.clusters.count",
          "limit": 1
        },
        {
          "name": "mdb.cpu.count",
          "limit": 1
        }
      ]
    }
    """
        Then we get response with status 200
        When we "POST" via REST at "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "testRedis2",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_2": {
                "password": "p@ssw#$rd!?"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
        Then we get response with status 422 and body contains
    """
    {
      "code": 9,
      "message": "Quota limits exceeded, not enough clusters: 1 clusters, cpu: 1 cores",
      "details":
        [
            {
                "@type": "type.private-api.yandex-cloud.ru/quota.QuotaFailure",
                "cloud_id": "cloud1",
                "violations": [
                    {
                        "metric": {
                            "limit": 1,
                            "name": "mdb.clusters.count",
                            "usage": 1
                        },
                        "required": 2
                    },
                    {
                        "metric": {
                            "limit": 1,
                            "name": "mdb.cpu.count",
                            "usage": 1
                        },
                        "required": 2
                    }
                ]
            }
        ]
    }
    """
        When we add default feature flag "MDB_KAFKA_CLUSTER"
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s1.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
          }
        }
      }
    }
    """
        Then we get gRPC response error with code FAILED_PRECONDITION and message "Quota limits exceeded, not enough "cpu: 1, clusters: 1". Please contact support to request extra resource quota." and details contain
    """
    [
        {
            "@type": "type.private-api.yandex-cloud.ru/quota.QuotaFailure",
            "cloud_id": "cloud1",
            "violations": [
                {
                    "metric": {
                        "limit": "1",
                        "name": "mdb.cpu.count",
                        "usage": 1
                    },
                    "required": "2"
                },
                {
                    "metric": {
                        "limit": "1",
                        "name": "mdb.clusters.count",
                        "usage": 1
                    },
                    "required": "2"
                }
            ]
        }
    ]
    """

    Scenario: Not enough quota for burstable (cpu < 1)
        Given default headers
        When we POST "/mdb/v2/quota" with data
    """
    {
      "cloud_id": "cloud1",
      "metrics":[
        {
          "name": "mdb.cpu.count",
          "limit": 1
        }
      ]
    }
    """
        Then we get response with status 200
        When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "testRedis2",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_2": {
                "password": "p@ssw#$rd!?"
            },
            "resources": {
                "resourcePresetId": "b2.compute.2",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
        Then we get response with status 422 and body contains
    """
    {
      "code": 9,
      "message": "Quota limits exceeded, not enough cpu: 1 cores",
      "details":
        [
            {
                "@type": "type.private-api.yandex-cloud.ru/quota.QuotaFailure",
                "cloud_id": "cloud1",
                "violations": [
                    {
                        "metric": {
                            "limit": 1,
                            "name": "mdb.cpu.count",
                            "usage": 1
                        },
                        "required": 2
                    }
                ]
            }
        ]
    }
    """

    Scenario: Update quota of non-existent cloud works
        When we POST "/mdb/v2/quota" with data
    """
    {
      "cloud_id": "cloud2",
      "metrics":[
        {
          "name": "mdb.cpu.count",
          "limit": 64
        }
      ]
    }
    """
        Then we get response with status 200
        When we GET "/mdb/v1/quota/cloud2"
        Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud2",
        "cpuQuota": 64.0
    }
    """

    Scenario: Update quota with cloud_id in url works
        Given default headers
        When we GET "/mdb/v1/quota/cloud1"
        Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "ssdSpaceQuota": 644245094400,
        "hddSpaceQuota": 644245094400
    }
    """
        When we POST "/mdb/v2/quota/cloud1" with data
    """
    {
      "metrics":[
        {
          "name": "mdb.hdd.size",
          "limit": 1024
        },
        {
          "name": "mdb.ssd.size",
          "limit": 17179869184
        }
      ]
    }
    """
        Then we get response with status 200
        When we GET "/mdb/v1/quota/cloud1"
        Then we get response with status 200 and body contains
    """
    {
        "ssdSpaceQuota": 17179869184,
        "hddSpaceQuota": 1024
    }
    """
