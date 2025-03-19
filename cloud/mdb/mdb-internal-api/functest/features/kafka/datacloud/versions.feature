@grpc_api
Feature: Management of Datacloud Kafka versions

  Background:
    Given default headers

  Scenario: List versions works
    When we "List" via DataCloud at "datacloud.kafka.v1.VersionService" with data
    """
    {}
    """
    Then we get DataCloud response with body
    """
    {
        "versions": [
            {"id": "3.1",  "name": "3.1",     "updatable_to": [],          "deprecated": false},
            {"id": "3.0",  "name": "3.0",     "updatable_to": ["3.1"],          "deprecated": false},
            {"id": "2.8",  "name": "2.8",     "updatable_to": ["3.0", "3.1"],          "deprecated": false}
        ],
        "next_page": {
            "token": ""
        }
    }
    """
