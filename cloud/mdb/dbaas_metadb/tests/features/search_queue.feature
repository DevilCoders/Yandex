Feature: Search queue api

  Scenario: Add to search queue
    Given default database
    When I execute query
    """
    INSERT INTO dbaas.search_queue (doc)
    VALUES ('{"new": "doc"}')
    RETURNING *
    """
    Then it success
