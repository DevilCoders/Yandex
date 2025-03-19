@sqlserver
@grpc_api
Feature: Microsoft SQLServer Resource Preset API
  Background:
    Given default headers

  Scenario: Resource preset service works
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ResourcePresetService"
    Then we get gRPC response with body
    """
    {
      "next_page_token": "",
      "resource_presets": [
          {
              "cores": "1",
              "id": "s1.compute.1",
              "memory": "4294967296",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "2",
              "id": "s1.compute.2",
              "memory": "8589934592",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "4",
              "id": "s1.compute.3",
              "memory": "17179869184",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "8",
              "id": "s1.compute.4",
              "memory": "34359738368",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "16",
              "id": "s1.compute.5",
              "memory": "68719476736",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "1",
              "id": "s1.porto.1",
              "memory": "4294967296",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "2",
              "id": "s1.porto.2",
              "memory": "8589934592",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "4",
              "id": "s1.porto.3",
              "memory": "17179869184",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "8",
              "id": "s1.porto.4",
              "memory": "34359738368",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "16",
              "id": "s1.porto.5",
              "memory": "68719476736",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "1",
              "id": "s2.compute.1",
              "memory": "4294967296",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "2",
              "id": "s2.compute.2",
              "memory": "8589934592",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "4",
              "id": "s2.compute.3",
              "memory": "17179869184",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "8",
              "id": "s2.compute.4",
              "memory": "34359738368",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "16",
              "id": "s2.compute.5",
              "memory": "68719476736",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "1",
              "id": "s2.porto.1",
              "memory": "4294967296",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "2",
              "id": "s2.porto.2",
              "memory": "8589934592",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "4",
              "id": "s2.porto.3",
              "memory": "17179869184",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "8",
              "id": "s2.porto.4",
              "memory": "34359738368",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          },
          {
              "cores": "16",
              "id": "s2.porto.5",
              "memory": "68719476736",
              "zone_ids": [
                  "iva",
                  "man",
                  "myt",
                  "sas",
                  "vla"
              ]
          }
      ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ResourcePresetService" with data
    """
    {
        "resource_preset_id": "s1.compute.1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cores": "1",
        "id": "s1.compute.1",
        "memory": "4294967296",
        "zone_ids": [
            "iva",
            "man",
            "myt",
            "sas",
            "vla"
        ]
    }
    """
