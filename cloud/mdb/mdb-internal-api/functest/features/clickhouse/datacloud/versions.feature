@grpc_api
Feature: Management of Datacloud ClickHouse versions

  Background:
    Given default headers

  Scenario: List versions works
    When we "List" via DataCloud at "datacloud.clickhouse.v1.VersionService" with data
    """
    {}
    """
    Then we get DataCloud response with body
    """
    {
        "versions": [
            {"id": "21.1",  "name": "21.1",     "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": true },
            {"id": "21.2",  "name": "21.2",     "updatable_to": [        "21.3", "21.4", "21.7", "21.8", "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.3",  "name": "21.3 LTS", "updatable_to": ["21.2",         "21.4", "21.7", "21.8", "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.4",  "name": "21.4",     "updatable_to": ["21.2", "21.3",         "21.7", "21.8", "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.7",  "name": "21.7",     "updatable_to": ["21.2", "21.3", "21.4",         "21.8", "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.8",  "name": "21.8 LTS", "updatable_to": ["21.2", "21.3", "21.4", "21.7",         "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.9",  "name": "21.9",     "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8",         "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.10", "name": "21.10",    "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9",          "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.11", "name": "21.11",    "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9", "21.10",          "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "22.1",  "name": "22.1",     "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9", "21.10", "21.11",         "22.3", "22.5"], "deprecated": false},
            {"id": "22.3",  "name": "22.3 LTS", "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9", "21.10", "21.11", "22.1",         "22.5"], "deprecated": false},
            {"id": "22.5",  "name": "22.5",     "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9", "21.10", "21.11", "22.1", "22.3"        ], "deprecated": false}
        ],
        "next_page": {
            "token": ""
        }
    }
    """

  Scenario: List versions with page size works
    When we "List" via DataCloud at "datacloud.clickhouse.v1.VersionService" with data
    """
    {
        "paging": {
            "page_size": 2
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "versions": [
            {"id": "21.1",  "name": "21.1",     "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": true },
            {"id": "21.2",  "name": "21.2",     "updatable_to": [        "21.3", "21.4", "21.7", "21.8", "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false}
        ],
        "next_page": {
            "token": "eyJPZmZzZXQiOjJ9"
        }
    }
    """

  Scenario: List versions with page token works
    When we "List" via DataCloud at "datacloud.clickhouse.v1.VersionService" with data
    """
    {
        "paging": {
            "page_token": "eyJPZmZzZXQiOjJ9"
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "versions": [
            {"id": "21.3",  "name": "21.3 LTS", "updatable_to": ["21.2",         "21.4", "21.7", "21.8", "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.4",  "name": "21.4",     "updatable_to": ["21.2", "21.3",         "21.7", "21.8", "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.7",  "name": "21.7",     "updatable_to": ["21.2", "21.3", "21.4",         "21.8", "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.8",  "name": "21.8 LTS", "updatable_to": ["21.2", "21.3", "21.4", "21.7",         "21.9", "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.9",  "name": "21.9",     "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8",         "21.10", "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.10", "name": "21.10",    "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9",          "21.11", "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "21.11", "name": "21.11",    "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9", "21.10",          "22.1", "22.3", "22.5"], "deprecated": false},
            {"id": "22.1",  "name": "22.1",     "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9", "21.10", "21.11",         "22.3", "22.5"], "deprecated": false},
            {"id": "22.3",  "name": "22.3 LTS", "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9", "21.10", "21.11", "22.1",         "22.5"], "deprecated": false},
            {"id": "22.5",  "name": "22.5",     "updatable_to": ["21.2", "21.3", "21.4", "21.7", "21.8", "21.9", "21.10", "21.11", "22.1", "22.3"        ], "deprecated": false}
        ],
        "next_page": {
            "token": ""
        }
    }
    """
