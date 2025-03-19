Feature: Populate table
  Background:
    Given default database

  Scenario: Populate without updates works
    Given fresh default database
    And populate datafile
    """
    [{"name": "mycoolflavor",
      "platform_id": "1",
      "cpu_guarantee": 1,
      "cpu_limit": 1,
      "memory_guarantee": 1073741824,
      "memory_limit": 1073741824,
      "network_guarantee": 1048576,
      "network_limit": 1048576,
      "io_limit": 5242880
     }]
    """
    When I run populate_table on table "dbaas.flavors"
    And I execute query
    """
    SELECT name, io_limit FROM dbaas.flavors
    """
    Then it returns one row matches
    """
    name: mycoolflavor
    io_limit: 5242880
    """

  Scenario: Populate with update works
    Given populate datafile
    """
    [{"name": "mycoolflavor",
      "platform_id": "1",
      "cpu_guarantee": 1,
      "cpu_limit": 1,
      "memory_guarantee": 1073741824,
      "memory_limit": 1073741824,
      "network_guarantee": 1048576,
      "network_limit": 1048576,
      "io_limit": 6242880
     }]
    """
    When I run populate_table on table "dbaas.flavors"
    And I execute query
    """
    SELECT name, io_limit FROM dbaas.flavors
    """
    Then it returns one row matches
    """
    name: mycoolflavor
    io_limit: 6242880
    """

  Scenario: Populate with jsonb value works
    Given fresh default database
    And populate datafile
    """
    [{"type": "postgresql_cluster",
      "value": {"a": 1, "b": [{"c": "%02d"}, 9]}}]
    """
    When I run populate_table on table "dbaas.cluster_type_pillar" with key "type"
    And I execute query
    """
    SELECT type, value FROM dbaas.cluster_type_pillar
    """
    Then it returns one row matches
    """
    type: "postgresql_cluster"
    value: {"a": 1, "b": [{"c": "%02d"}, 9]}
    """
