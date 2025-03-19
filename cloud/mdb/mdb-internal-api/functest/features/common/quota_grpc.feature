@grpc_api
@quota
Feature: Quota v2 GRPC API

    Scenario: Empty cloud has default quota
        When we add cloud "cloud1"
        Given default headers
        When we "Get" via gRPC at "yandex.cloud.priv.mdb.v2.QuotaService" with data
    """
    {
        "cloud_id": "cloud1"
    }
    """
        Then we get gRPC response with body
    """
    {
        "cloud_id": "cloud1",
        "metrics": [
          {
            "name": "mdb.hdd.size",
            "limit": "644245094400",
            "usage": 0.0,
            "value": "0"
          },
          {
            "name": "mdb.clusters.count",
            "limit": "2",
            "usage": 0.0,
            "value": "0"
          },
          {
            "name": "mdb.memory.size",
            "limit": "103079215104",
            "usage": 0.0,
            "value": "0"
          },
          {
            "name": "mdb.cpu.count",
            "limit": "24",
            "usage": 0.0,
            "value": "0"
          },
          {
            "name": "mdb.gpu.count",
            "limit": "0",
            "usage": 0.0,
            "value": "0"
          },
          {
            "name": "mdb.ssd.size",
            "limit": "644245094400",
            "usage": 0.0,
            "value": "0"
          }
        ]
    }
    """
        When we "BatchUpdateMetric" via gRPC at "yandex.cloud.priv.mdb.v2.QuotaService" with data
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
        Then we get gRPC response with body
    """
    {}
    """
        When we "Get" via gRPC at "yandex.cloud.priv.mdb.v2.QuotaService" with data
    """
    {
        "cloud_id": "cloud1"
    }
    """
        Then we get gRPC response with body
    """
    {
        "cloud_id": "cloud1",
        "metrics": [
          {
            "name": "mdb.hdd.size",
            "limit": "1024",
            "usage": 0.0,
            "value": "0"
          },
          {
            "name": "mdb.clusters.count",
            "limit": "5",
            "usage": 0.0,
            "value": "0"
          },
          {
            "name": "mdb.memory.size",
            "limit": "4294967296",
            "usage": 0.0,
            "value": "0"
          },
          {
            "name": "mdb.cpu.count",
            "limit": "2",
            "usage": 0.0,
            "value": "0"
          },
          {
            "name": "mdb.gpu.count",
            "limit": "1",
            "usage": 0.0,
            "value": "0"
          },
          {
            "name": "mdb.ssd.size",
            "limit": "17179869184",
            "usage": 0.0,
            "value": "0"
          }
        ]
    }
    """
