@labels @MDB-7880
Feature: Common labels logic

    Background: Valid auth
        Given default headers

    Scenario: Invalid labels
        When we "POST" via REST at "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
        """
        {
           "name": "test",
           "environment": "PRESTABLE",
           "configSpec": {
               "version": "14",
               "resources": {
                   "resourcePresetId": "s1.porto.1",
                   "diskTypeId": "local-ssd",
                   "diskSize": 10737418240
               }
           },
           "databaseSpecs": [{
               "name": "testdb",
               "owner": "test"
           }],
           "userSpecs": [{
               "name": "test",
               "password": "test_password"
           }],
           "hostSpecs": [{
               "zoneId": "myt"
           }],
           "labels": {
                "//_-)": ""
           }
        }
        """
        Then we get response with status 422 and body contains
        """
        {
            "code": 3,
            "message": "The request is invalid.\nlabels: Symbol ')' not allowed in key."
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
          },
           "labels": {
                "//_-)": ""
           }
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "label key "//_-)" has invalid symbols"
